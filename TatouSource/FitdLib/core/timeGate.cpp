///////////////////////////////////////////////////////////////////////////////
// Alone in the Dark: Classic Reborn
// Based on FITD by yaz0r and AloneInTheDarkReHaunted, released under GPL
//
// Time Gate: Knight's Chase. Its menu of eight coins around the seal
// (TIMEGATE.EXE 19F80), at the start and, with Esc, during the game (Esc
// again goes back to it), with the mouse as in the original, and with the
// keyboard and the controller: the arrows go to the previous or next coin,
// Enter or the action button chooses it. Loading and saving show the
// original's drum (1FE10, 1F750, 20390): a 3D drum of eight wooden boards,
// each with the picture of a save, that turns to the one chosen; here it
// turns at once instead of slowly, and also with the controller and the
// mouse wheel. Saves in the original's format (saveTimeGate.cpp). The
// difficulty is chosen with three tarot cards (16B00). The sound coin chooses
// the instruments of the music (the original set up the sound card there).
///////////////////////////////////////////////////////////////////////////////

#include "common.h"
#include "coreHooks.h"
#include "consoleLog.h"
#include "input.h"
#include "menuMouse.h"
#include "timeGate.h"
#include "saveTimeGate.h"
#include "timeGateVideo.h"
#include "timeGateSound.h"
#include "hybrid.h"
#include "sprite.h"
#include "settings.h"
#include "music.h"

#include <array>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <vector>

namespace
{
    // The coins, in the original's order (TIMEGATE.EXE 19E40): from the top
    // left down the left side, then up the right side. Each is a box of
    // 51 x 46 from its corner. The seal already has them all, dark; the
    // chosen one is drawn over it lit, a hybrid of ITD_RESS (19B00)
    struct Coin
    {
        int x, y;
        int litPicture;  // ITD_RESS entry
        const char* name;
    };
    const Coin kCoins[8] = {
        { 39, 1, 102, "new game" },
        { 28, 52, 111, "save" },
        { 43, 109, 93, "load" },
        { 96, 143, 113, "sound setup" },
        { 175, 143, 115, "volume" },
        { 227, 109, 109, "intro" },
        { 239, 52, 95, "difficulty" },
        { 230, 1, 107, "quit" },
    };
    // A pressed coin sinks this much, and stays pressed at least this long
    // (ticks of 1/60 s) so that a quick tap is seen too. The original has no
    // pressed picture: it only lights the coin while it is held
    const int kPressOffset = 1;
    const unsigned int kMinPressTicks = 8;
    const int kCoinWidth = 51;
    const int kCoinHeight = 46;
    const int kMenuPicture = 1;   // the seal
    const int kEscape = 0x1B;     // Esc, as the input layer gives it
    const int kEnter = 28;

    const int kNewGame = 0;
    const int kSave = 1;   // 1A3D6 calls 1F750, which writes
    const int kLoad = 2;   // 1A453 calls 1FE10, which loads
    const int kSound = 3;  // 1A4DF calls 32970, the sound card setup
    const int kVolume = 4;  // 1A56C calls 16DA0
    const int kIntro = 5;   // 1A58A plays AN1
    const int kDifficulty = 6;  // 1A5CF calls 16B00
    // The intro: AN0 and AN1 at the start (29915), AN1 from its coin
    const int kIntroVideos[] = { 0, 1 };
    const int kIntroCoinVideo = 1;
    const int kQuit = 7;

    // The save screen (1FE10 loads, 1F750 saves): the stone wall (ITD_RESS 3
    // to load, 4 to save, with the screen's palette), the drum (ITD_RESS 52)
    // and its two textures, the boards (57) and the iron (58); the picture of
    // save n goes on board n (rows 10 to 89 of it, at x 44 and the board's
    // row, 20940). The drum turns 1/8 of a turn per save (128 of 1024)
    const int kLoadPicture = 3;
    const int kSavePicture = 4;
    const int kDrumBody = 52;
    const int kBoardsTexture = 57;
    const int kIronTexture = 58;
    const int kTurnSound = 55;    // while it turns
    const int kStopSound = 56;    // when it stops on a save
    const int kSlots = 8;         // SAVE0 to SAVE7: the original lists no other
    const int kBoardRows[kSlots] = { 7, 104, 204, 299, 395, 492, 587, 680 };
    const int kBoardX = 44;
    const int kPictureWidth = 160;
    const int kPictureHeight = 100;
    const int kPictureTop = 10;    // the rows of the picture on a board
    const int kPictureBottom = 90;
    // Numbers of the drum's textures for the renderer (TEXTURES.PAK has far fewer)
    const int kDrumBoards = 0x1000;
    const int kDrumIron = 0x1001;
    const int kTurnPerSlot = 1024 / kSlots;
    const int kTurnSpeed = 24;     // per tick: a save in about 0.1 s (the original took about a second)

    // During the game: a coin asked for a new game or to quit
    bool s_newGameAsked = false;
    bool s_quitAsked = false;

    std::vector<u8> loadResource(int entry)
    {
        std::vector<u8> data;
        const int size = getPakSize("ITD_RESS", entry);
        if (char* raw = size > 0 ? loadPak("ITD_RESS", entry) : nullptr)
        {
            data.assign((u8*)raw, (u8*)raw + size);
            free(raw);
        }
        return data;
    }

    std::unique_ptr<sHybrid> loadHybrid(int entry)
    {
        std::vector<u8> data = loadResource(entry);
        if (data.size() <= 24)
            return nullptr;
        return std::unique_ptr<sHybrid>(new sHybrid(data.data(), (int)data.size()));
    }

    // Where a drawing went on the 320 x 200 screen (empty: x1 > x2)
    struct Box
    {
        int x1 = 320, y1 = 200, x2 = -1, y2 = -1;
        bool contains(float x, float y) const { return x >= x1 && x <= x2 && y >= y1 && y <= y2; }
    };

    // The first entity of a hybrid, its sprites drawn into the 320 x 200
    // picture. AffSpr draws on a layer of two bytes per point (colour, then
    // whether it is covered): drawn there first, then copied. Its box
    Box drawHybrid(sHybrid* hybrid, int x, int y, u8* picture)
    {
        Box box;
        if (!hybrid || hybrid->entities.empty())
            return box;
        static std::array<u8, 320 * 200 * 2> layer;
        layer.fill(0);
        for (auto& part : hybrid->entities[0].parts)
        {
            if (part.id != 1 || part.XY.empty())
                continue;
            const int sprite = part.cout + (((int)part.cin) << 8);
            AffSpr(sprite, part.XY[0][0] + x, part.XY[0][1] + y + 1, layer.data(), hybrid->sprites);
        }
        for (int i = 0; i < 320 * 200; i++)
            if (layer[i * 2 + 1])
            {
                picture[i] = layer[i * 2];
                const int px = i % 320, py = i / 320;
                box.x1 = (std::min)(box.x1, px); box.x2 = (std::max)(box.x2, px);
                box.y1 = (std::min)(box.y1, py); box.y2 = (std::max)(box.y2, py);
            }
        return box;
    }

    int coinAt(float x, float y)
    {
        for (int i = 0; i < 8; i++)
            if (x >= kCoins[i].x && x <= kCoins[i].x + kCoinWidth && y >= kCoins[i].y && y <= kCoins[i].y + kCoinHeight)
                return i;
        return -1;
    }

    void playResourceSound(int entry)
    {
        std::vector<u8> sound = loadResource(entry);
        if (!sound.empty())
            osystem_playSample((char*)sound.data(), (int)sound.size());   // the mixer keeps a copy
    }

    // The menu until a coin is chosen: its number; -1 for Esc during the game
    int coinMenu(bool inGame)
    {
        std::vector<u8> sealData = loadResource(kMenuPicture);
        if (sealData.size() < 64000 + 768)
            return inGame ? -1 : kQuit;
        palette_t palette;
        copyPalette(sealData.data() + 64000, palette);
        convertPaletteIfRequired(palette);
        copyPalette(palette, currentGamePalette);
        setPalette(palette);

        std::array<std::unique_ptr<sHybrid>, 8> litCoins;
        for (int i = 0; i < 8; i++)
            litCoins[i] = loadHybrid(kCoins[i].litPicture);

        // Three looks: dark (as in the seal), lit (the selected one, where the
        // mouse or the controller is) and pressed (lit and sunk, while the
        // click, Enter or the button is held: it is chosen on release)
        int selected = kNewGame;
        int chosen = -2;
        int pressed = -1;
        bool pressedByMouse = false;
        unsigned int pressedAt = 0;
        bool waitRelease = true;   // the key that opened the menu
        bool firstFrame = true;
        ImVec2 lastMouse = { -1.0f, -1.0f };
        while (chosen == -2)
        {
            memcpy(logicalScreen, sealData.data(), 64000);
            const int offset = (pressed == selected) ? kPressOffset : 0;
            drawHybrid(litCoins[selected].get(), kCoins[selected].x + offset, kCoins[selected].y + offset, (u8*)logicalScreen);

            osystem_startFrame();
            osystem_CopyBlockPhys((unsigned char*)logicalScreen, 0, 0, 320, 200);
            osystem_stopFrame();
            osystem_flip(NULL);
            if (firstFrame)
            {
                FadeInPhys(16, 0);
                firstFrame = false;
            }

            process_events();
            const ImVec2 mouse = menuGetGameMouse();

            // Held down: chosen when let go, and not before it has been seen
            if (pressed >= 0)
            {
                const bool held = pressedByMouse ? ImGui::GetIO().MouseDown[0] : (key == kEnter || Click);
                if (!held && timer - pressedAt >= kMinPressTicks)
                {
                    // The mouse let go outside the coin: nothing is chosen
                    if (!pressedByMouse || coinAt(mouse.x, mouse.y) == pressed)
                        chosen = pressed;
                    pressed = -1;
                    waitRelease = true;
                }
                continue;
            }

            // The mouse: pointing lights a coin, a click presses it
            if (menuMouseMoved(lastMouse, JoyD || key))
            {
                const int hit = coinAt(mouse.x, mouse.y);
                if (hit >= 0)
                    selected = hit;
            }
            if (menuMouseClicked())
            {
                const int hit = coinAt(mouse.x, mouse.y);
                if (hit >= 0)
                {
                    selected = pressed = hit;
                    pressedByMouse = true;
                    pressedAt = timer;
                }
                continue;
            }

            if (waitRelease)
            {
                if (!JoyD && !key && !Click)
                    waitRelease = false;
                continue;
            }
            if (inGame && key == kEscape)   // back to the game
                chosen = -1;
            else if (JoyD & (1 | 4))   // up or left: the previous coin
            {
                selected = (selected + 7) % 8;
                waitRelease = true;
            }
            else if (JoyD & (2 | 8))   // down or right: the next one
            {
                selected = (selected + 1) % 8;
                waitRelease = true;
            }
            else if (key == kEnter || Click)
            {
                pressed = selected;
                pressedByMouse = false;
                pressedAt = timer;
            }
        }

        // What chose it must not reach the game
        while (JoyD || key || Click)
            process_events();
        printf(MAIN_OK "Time Gate menu: %s\n", chosen >= 0 ? kCoins[chosen].name : "back to the game");
        return chosen;
    }

    // The colour of `palette` nearest to (r, g, b)
    u8 nearestColour(const palette_t& palette, int r, int g, int b)
    {
        int best = 0;
        int bestDistance = 1 << 30;
        for (int i = 0; i < 256; i++)
        {
            const int dr = palette[i][0] - r, dg = palette[i][1] - g, db = palette[i][2] - b;
            const int distance = dr * dr + dg * dg + db * db;
            if (distance < bestDistance)
            {
                best = i;
                bestDistance = distance;
            }
        }
        return (u8)best;
    }

    // The number of a board (1 to 8; the file is one less, SAVE0 to SAVE7),
    // in the top left corner of where its picture goes, light on a dark box,
    // in digits of 3 x 5 enlarged twice, as those of the dates of the saves
    // list (game.cpp). The original has no numbers
    void drawBoardNumber(std::vector<u8>& boards, int slot, u8 colour, u8 boxColour)
    {
        static const unsigned char kDigits[10][5] = {
            { 7, 5, 5, 5, 7 }, { 2, 6, 2, 2, 7 }, { 7, 1, 7, 4, 7 }, { 7, 1, 7, 1, 7 }, { 5, 5, 7, 1, 1 },
            { 7, 4, 7, 1, 7 }, { 7, 4, 7, 5, 7 }, { 7, 1, 2, 2, 2 }, { 7, 5, 7, 5, 7 }, { 7, 5, 7, 1, 7 } };
        const int scale = 2;
        const int margin = 2;
        const int left = kBoardX + 6;
        const int top = kBoardRows[slot] + 3;
        const unsigned char* glyph = kDigits[slot + 1];
        for (int row = -margin; row < 5 * scale + margin; row++)
            for (int column = -margin; column < 3 * scale + margin; column++)
            {
                const bool inside = row >= 0 && row < 5 * scale && column >= 0 && column < 3 * scale;
                const bool lit = inside && ((glyph[row / scale] >> (2 - column / scale)) & 1);
                const size_t at = (size_t)(top + row) * 256 + left + column;
                if (at < boards.size())
                    boards[at] = lit ? colour : boxColour;
            }
    }

    // The boards with the number and the picture of each save, in the
    // screen's colours (a picture keeps those of its room); which saves there are
    std::vector<u8> boardsWithPictures(const palette_t& screenPalette, std::array<bool, kSlots>& exists)
    {
        std::vector<u8> boards = loadResource(kBoardsTexture);
        std::vector<u8> picture(kPictureWidth * kPictureHeight);
        std::vector<u8> palette(768);
        for (int slot = 0; slot < kSlots; slot++)
        {
            exists[slot] = readTimeGateSavePicture(slot, picture.data(), palette.data());
            if (!exists[slot])
                continue;
            u8 colours[256];
            for (int c = 0; c < 256; c++)
                colours[c] = nearestColour(screenPalette, palette[c * 3], palette[c * 3 + 1], palette[c * 3 + 2]);
            for (int y = kPictureTop; y < kPictureBottom; y++)
            {
                const size_t row = (size_t)(kBoardRows[slot] + y - kPictureTop) * 256 + kBoardX;
                if (row + kPictureWidth > boards.size())
                    break;
                for (int x = 0; x < kPictureWidth; x++)
                    boards[row + x] = colours[picture[y * kPictureWidth + x]];
            }
        }
        const u8 numberColour = nearestColour(screenPalette, 235, 215, 160);
        const u8 boxColour = nearestColour(screenPalette, 30, 20, 10);
        for (int slot = 0; slot < kSlots; slot++)
            drawBoardNumber(boards, slot, numberColour, boxColour);
        return boards;
    }

    // The save screen: chooses one of the 8 saves with the drum, then loads
    // it or writes the game there. True when a save was loaded or written
    bool saveScreen(bool saving)
    {
        std::vector<u8> wall = loadResource(saving ? kSavePicture : kLoadPicture);
        std::vector<u8> drumData = loadResource(kDrumBody);
        if (wall.size() < 64000 + 768 || drumData.empty())
            return false;
        palette_t screenPalette;
        copyPalette(wall.data() + 64000, screenPalette);
        convertPaletteIfRequired(screenPalette);
        setPalette(screenPalette);

        std::array<bool, kSlots> exists = {};
        setTimeGateTexture(kDrumBoards, boardsWithPictures(screenPalette, exists));
        setTimeGateTexture(kDrumIron, loadResource(kIronTexture));

        // The drum, its two textures (the last bytes of its scratch, as
        // hqr.cpp reads them) replaced by the boards and the iron
        std::unique_ptr<sBody> drum(createBodyFromPtr(drumData.data()));
        if (drum->m_scratchBuffer.size() >= 10)
        {
            u8* scratch = drum->m_scratchBuffer.data() + drum->m_scratchBuffer.size() - 10;
            const s16 first = (s16)READ_LE_U16(scratch + 6);
            for (sPrimitive& primitive : drum->m_primitives)
                if (primitive.m_texture >= 0)
                    primitive.m_texture = (primitive.m_texture == first) ? kDrumBoards : kDrumIron;
        }

        int slot = 0;
        // Where the drum is (0-1023): it opens with the first save in front,
        // then turns towards the one chosen
        int angle = 0;
        bool turning = false;
        bool waitRelease = true;
        int chosen = -2;
        unsigned int lastTick = timer;
        while (chosen == -2)
        {
            memcpy(logicalScreen, wall.data(), 64000);
            osystem_CopyBlockPhys((unsigned char*)logicalScreen, 0, 0, 320, 200);
            process_events();
            osystem_drawBackground();

            // Turned by the clock, the same at any frame rate
            const int target = slot * kTurnPerSlot;   // board n in front at n / 8 of a turn
            for (unsigned int ticks = timer - lastTick; ticks > 0 && angle != target; ticks--)
            {
                int difference = ((target - angle) % 1024 + 1024 + 512) % 1024 - 512;   // the short way round
                const int step = (std::min)(kTurnSpeed, std::abs(difference));
                angle = (angle + (difference > 0 ? step : -step) + 1024) % 1024;
            }
            lastTick = timer;
            if (turning && angle == target)
            {
                turning = false;
                playResourceSound(kStopSound);
            }

            SetProjection(160, 100, 128, 500, 490);
            SetAngleCamera(0, 0, 0);
            SetPosCamera(0, 0, -8000);
            setTimeGateLightRoom(-1, 0, 0, 0);   // no room's light: as it is
            AffObjet(-430, 0, -1520, angle, 0, 0, drum.get());

            // Its input: up and down (or left and right, or the wheel) turn
            // it, Enter or a click chooses, Esc leaves
            int move = 0;
            const float wheel = ImGui::GetIO().MouseWheel;
            if (wheel > 0.0f)
                move = -1;
            else if (wheel < 0.0f)
                move = 1;
            if (waitRelease)
            {
                if (!JoyD && !key && !Click)
                    waitRelease = false;
            }
            else if (key == kEscape)
                chosen = -1;
            else if (JoyD & (1 | 4))
                move = -1;
            else if (JoyD & (2 | 8))
                move = 1;
            else if (key == kEnter || Click || menuMouseClicked())
            {
                if (saving || exists[slot])
                    chosen = slot;
                else
                    playResourceSound(kStopSound);   // nothing to load there
                waitRelease = true;
            }
            if (move)
            {
                slot = (slot + move + kSlots) % kSlots;
                turning = true;
                waitRelease = true;
                playResourceSound(kTurnSound);
            }
        }
        while (JoyD || key || Click)
            process_events();

        if (chosen < 0)
            return false;
        printf(MAIN_OK "Time Gate: %s save %d\n", saving ? "writing" : "loading", chosen);
        return saving ? makeTimeGateSave(chosen) : loadTimeGateSave(chosen);
    }

    // The game, new or loaded, until it ends or a coin stops it
    void play(bool newGame)
    {
        timeGateStopSounds();
        if (newGame)
            startGame(0, 5, 1);
        else
        {
            FlagInitView = 2;
            InitView();
            PlayWorld(1, 1);
        }
    }
}

namespace
{
    // The inventory (TIMEGATE.EXE 103D0): the stained glass (ITD_RESS 5), the
    // object chosen turning in its circle with its name below (10010), and
    // the actions it has, each in its own place of the side panels: blue, red
    // when chosen (10A67, 10B8E, 10CC8); the empty places drawn first. Its
    // actions are the bits of its foundFlag, as in AITD2 and AITD3; bits 5
    // and 7 have no place (Time Gate does not use them)
    const int kInventoryPicture = 5;
    struct ActionPlace
    {
        int bit;
        int x, y;
        int picture, chosenPicture, emptyPicture;
    };
    const ActionPlace kActionPlaces[] = {
        { 0, 28, 38, 38, 37, 39 },     // tool
        { 1, 18, 62, 26, 25, 27 },     // dish and cup
        { 2, 14, 86, 23, 22, 24 },     // book
        { 3, 14, 110, 17, 16, 18 },    // cart
        { 9, 17, 134, 20, 19, 21 },    // throw
        { 10, 25, 158, 29, 28, 30 },   // hand
        { 4, 251, 62, 11, 10, 12 },    // fight
        { 6, 261, 86, 14, 13, 15 },    // search, take
        { 8, 264, 110, 32, 31, 33 },   // push
    };
    const int kPreviousPicture = 41, kPreviousX = 102, kPreviousY = 156;
    const int kNextPicture = 40, kNextX = 205, kNextY = 156;
    const int kRosePicture = 42, kRoseX = 132, kRoseY = 1;
    const int kMaxActions = 6;
    const int kNameColour = 64;
    const int kNameY = 184;
    // The circle and its 3D object (103D0: projection 159, 102)
    const int kCircleLeft = 98, kCircleTop = 34, kCircleRight = 220, kCircleBottom = 152;

    struct InventoryPictures
    {
        std::vector<u8> glass;
        palette_t palette;
        std::array<std::unique_ptr<sHybrid>, 64> hybrids;   // by ITD_RESS entry
        sHybrid* get(int entry)
        {
            if (!hybrids[entry])
                hybrids[entry] = loadHybrid(entry);
            return hybrids[entry].get();
        }
    };
}

void timeGateInventory()
{
    if (!numObjInInventoryTable[currentInventory])
        return;

    InventoryPictures pictures;
    pictures.glass = loadResource(kInventoryPicture);
    if (pictures.glass.size() < 64000 + 768)
        return;
    copyPalette(pictures.glass.data() + 64000, pictures.palette);
    convertPaletteIfRequired(pictures.palette);
    setPalette(pictures.palette);

    SaveTimerAnim();
    const s16 savedShakingAmplitude = shakingAmplitude;
    const s16 savedShakeVar1 = shakeVar1;
    pauseShaking();

    int object = 0;           // place in the inventory
    int chosenAction = 0;     // place in the object's list of actions
    int actions[kMaxActions];
    int actionCount = 0;
    int lastObject = -1;
    sBody* body = nullptr;
    int beta = 0;
    unsigned int lastTick = timer;
    bool waitRelease = true;
    bool done = false;
    bool act = false;
    ImVec2 lastMouse = { -1.0f, -1.0f };
    while (!done)
    {
        const int worldObject = inventoryTable[currentInventory][object];
        if (object != lastObject)
        {
            // Its actions: the bits of its foundFlag, at most 6 (10902)
            const tWorldObject& world = ListWorldObjets[worldObject];
            actionCount = 0;
            for (int bit = 0; bit < 11 && actionCount < kMaxActions; bit++)
                if (world.foundFlag & (1 << bit))
                    actions[actionCount++] = bit;
            chosenAction = 0;
            body = HQR_Get(HQ_Bodys, world.foundBody);
            lastObject = object;
        }

        // The glass, its empty places, the actions of the object and its name
        memcpy(logicalScreen, pictures.glass.data(), 64000);
        u8* screen = (u8*)logicalScreen;
        drawHybrid(pictures.get(kRosePicture), kRoseX, kRoseY, screen);
        Box actionBoxes[kMaxActions];
        for (const ActionPlace& place : kActionPlaces)
            drawHybrid(pictures.get(place.emptyPicture), place.x, place.y, screen);
        for (int i = 0; i < actionCount; i++)
            for (const ActionPlace& place : kActionPlaces)
                if (place.bit == actions[i])
                    actionBoxes[i] = drawHybrid(pictures.get(i == chosenAction ? place.chosenPicture : place.picture),
                        place.x, place.y, screen);
        const Box previousBox = numObjInInventoryTable[currentInventory] > 1
            ? drawHybrid(pictures.get(kPreviousPicture), kPreviousX, kPreviousY, screen) : Box();
        const Box nextBox = numObjInInventoryTable[currentInventory] > 1
            ? drawHybrid(pictures.get(kNextPicture), kNextX, kNextY, screen) : Box();
        // Texts go to their own layer: emptied first, or each name stayed
        // under the next one (BUGS.md G12)
        uiLayer.fill(0);
        SimpleMessage(160, kNameY, ListWorldObjets[worldObject].foundName, kNameColour);

        osystem_CopyBlockPhys((unsigned char*)logicalScreen, 0, 0, 320, 200);
        process_events();
        osystem_drawBackground();

        // The object turns by the clock, the same at any frame rate
        const unsigned int ticks = (std::min)(timer - lastTick, 15u);
        lastTick = timer;
        beta = (beta - 8 * (int)ticks) & 1023;
        SetClip(kCircleLeft, kCircleTop, kCircleRight, kCircleBottom);
        SetProjection(159, 102, 128, 300, 300);
        setCameraTarget(0, 0, 0, 60, beta, 0, 24000);
        setTimeGateLightRoom(-1, 0, 0, 0);   // no room's light: as it is
        if (body)
            AffObjet(0, 0, 0, 0, 0, 0, body);
        SetClip(0, 0, 319, 199);

        // The mouse: an action is chosen and done, an arrow changes object
        const ImVec2 mouse = menuGetGameMouse();
        if (menuMouseMoved(lastMouse, JoyD || key))
            for (int i = 0; i < actionCount; i++)
                if (actionBoxes[i].contains(mouse.x, mouse.y))
                    chosenAction = i;
        int move = 0;
        if (menuMouseClicked())
        {
            for (int i = 0; i < actionCount; i++)
                if (actionBoxes[i].contains(mouse.x, mouse.y))
                {
                    chosenAction = i;
                    act = done = true;
                }
            if (previousBox.contains(mouse.x, mouse.y))
                move = -1;
            else if (nextBox.contains(mouse.x, mouse.y))
                move = 1;
            waitRelease = true;
        }
        else if (waitRelease)
        {
            if (!JoyD && !key && !Click)
                waitRelease = false;
        }
        else if (key == kEscape)
            done = true;
        else if (JoyD & 4)   // left: the previous object
            move = -1;
        else if (JoyD & 8)   // right: the next one
            move = 1;
        else if (JoyD & 1)   // up: the previous action
        {
            chosenAction = (chosenAction + actionCount - 1) % (std::max)(actionCount, 1);
            waitRelease = true;
        }
        else if (JoyD & 2)   // down: the next one
        {
            chosenAction = (chosenAction + 1) % (std::max)(actionCount, 1);
            waitRelease = true;
        }
        else if (key == kEnter || Click)
        {
            act = actionCount > 0;
            done = true;
        }
        if (move)
        {
            const int count = numObjInInventoryTable[currentInventory];
            object = (object + move + count) % count;
            waitRelease = true;
        }
    }

    while (key || JoyD || Click)
        process_events();
    RestoreTimerAnim();
    setPalette(currentGamePalette);
    FlagInitView = 1;
    shakingAmplitude = savedShakingAmplitude;
    shakeVar1 = savedShakeVar1;
    updateShaking();

    const int worldObject = inventoryTable[currentInventory][object];
    printf(MAIN_OK "Time Gate inventory: %s\n", act ? "action chosen" : "closed");
    if (act && actionCount > 0)
    {
        action = 1 << actions[chosenAction];
        executeFoundLife(worldObject);
    }
}

namespace
{
    // The volumes (TIMEGATE.EXE 16DA0): the lute (ITD_RESS 117), music on the
    // lutes' bar and sound effects (with the voices) on the horns' one, a
    // mark of 3 x 8 (colour 240) from x 81 to 236 on a black bar; X cancels
    // and the tick accepts (16C80: boxes of 51 x 46 at 1 and 270, 154). Here
    // they are the engine's two volumes (audio.musicVolume, audio.soundVolume,
    // the same as F1's Audio window), heard at once, with the keyboard and
    // the controller too: up and down choose the bar, left and right move it
    const int kVolumePicture = 117;
    const int kBarLeft = 81, kBarRight = 236;
    const int kBarTop[2] = { 64, 118 };
    const int kBarHeight = 8;
    const int kMarkWidth = 3;
    const int kMarkColour = 240;
    const int kButtonY = 154, kCancelX = 1, kAcceptX = 270;
    const int kCancelLitPicture = 91, kAcceptLitPicture = 105;
    const int kVolumeStep = 5;           // per press, then per few ticks while held
    const unsigned int kRepeatTicks = 6;

    int volumeToX(int volume) { return kBarLeft + volume * (kBarRight - kBarLeft) / 100; }
    int xToVolume(float x)
    {
        const int volume = (int)((x - kBarLeft) * 100.0f / (kBarRight - kBarLeft) + 0.5f);
        return (std::max)(0, (std::min)(100, volume));
    }

    // The coin under (x, y): 0 cancel, 1 accept, -1 none
    int volumeButtonAt(float x, float y)
    {
        if (y >= kButtonY && y <= kButtonY + kCoinHeight)
        {
            if (x >= kCancelX && x <= kCancelX + kCoinWidth)
                return 0;
            if (x >= kAcceptX && x <= kAcceptX + kCoinWidth)
                return 1;
        }
        return -1;
    }

    // The bar under (x, y), with a margin: 0 music, 1 sound, -1 none
    int volumeBarAt(float x, float y)
    {
        for (int i = 0; i < 2; i++)
            if (x >= kBarLeft - 4 && x <= kBarRight + 4 && y >= kBarTop[i] - 4 && y <= kBarTop[i] + kBarHeight + 4)
                return i;
        return -1;
    }

    void volumeScreen()
    {
        std::vector<u8> lute = loadResource(kVolumePicture);
        if (lute.size() < 64000 + 768)
            return;
        palette_t palette;
        copyPalette(lute.data() + 64000, palette);
        convertPaletteIfRequired(palette);
        setPalette(palette);
        std::unique_ptr<sHybrid> cancelLit = loadHybrid(kCancelLitPicture);
        std::unique_ptr<sHybrid> acceptLit = loadHybrid(kAcceptLitPicture);

        int* volumes[2] = { &g_settings.audio.musicVolume, &g_settings.audio.soundVolume };
        const int before[2] = { *volumes[0], *volumes[1] };
        int bar = 0;              // chosen with the keyboard or the controller
        int dragging = -1;        // the bar the mouse holds
        int pressed = -1;         // the coin held: 0 cancel, 1 accept
        int hovered = -1;         // the coin under the mouse
        bool pressedByMouse = false;
        bool waitRelease = true;
        unsigned int lastStep = 0;
        int result = -1;          // 0 cancel, 1 accept
        ImVec2 lastMouse = { -1.0f, -1.0f };
        while (result < 0)
        {
            memcpy(logicalScreen, lute.data(), 64000);
            u8* screen = (u8*)logicalScreen;
            for (int i = 0; i < 2; i++)
            {
                for (int y = kBarTop[i]; y < kBarTop[i] + kBarHeight; y++)
                    memset(screen + y * 320 + kBarLeft, 0, kBarRight + kMarkWidth - kBarLeft);
                // The chosen bar has its mark lit, the other one darker
                const u8 colour = (u8)(i == bar ? kMarkColour : kMarkColour - 6);
                const int x = volumeToX(*volumes[i]);
                for (int y = kBarTop[i]; y < kBarTop[i] + kBarHeight; y++)
                    memset(screen + y * 320 + x, colour, kMarkWidth);
            }
            const int lit = pressed >= 0 ? pressed : hovered;
            if (lit == 0)
                drawHybrid(cancelLit.get(), kCancelX, kButtonY, screen);
            else if (lit == 1)
                drawHybrid(acceptLit.get(), kAcceptX, kButtonY, screen);

            osystem_startFrame();
            osystem_CopyBlockPhys((unsigned char*)logicalScreen, 0, 0, 320, 200);
            osystem_stopFrame();
            osystem_flip(NULL);
            process_events();

            // The mouse: a click or a drag on a bar sets it; a coin answers
            // when let go over it
            const ImVec2 mouse = menuGetGameMouse();
            const bool mouseDown = ImGui::GetIO().MouseDown[0];
            if (menuMouseMoved(lastMouse, JoyD || key))
                hovered = volumeButtonAt(mouse.x, mouse.y);
            if (menuMouseClicked())
            {
                dragging = volumeBarAt(mouse.x, mouse.y);
                if (dragging >= 0)
                    bar = dragging;
                pressed = volumeButtonAt(mouse.x, mouse.y);
                pressedByMouse = pressed >= 0;
            }
            if (dragging >= 0)
            {
                if (mouseDown)
                    *volumes[dragging] = xToVolume(mouse.x);
                else
                    dragging = -1;
                continue;
            }
            if (pressed >= 0)
            {
                const bool held = pressedByMouse ? mouseDown : (key == kEnter || key == kEscape || Click);
                if (!held)
                {
                    if (!pressedByMouse || volumeButtonAt(mouse.x, mouse.y) == pressed)
                        result = pressed;
                    pressed = -1;
                }
                continue;
            }

            // The keyboard and the controller
            if (waitRelease)
            {
                if (!JoyD && !key && !Click)
                    waitRelease = false;
                continue;
            }
            if (key == kEscape)
            {
                pressed = 0;
                pressedByMouse = false;
            }
            else if (key == kEnter || Click)
            {
                pressed = 1;
                pressedByMouse = false;
            }
            else if (JoyD & (1 | 2))
            {
                bar = 1 - bar;
                waitRelease = true;
            }
            else if (JoyD & (4 | 8))
            {
                // A step at once, then one every few ticks while it is held
                if (lastStep == 0 || timer - lastStep >= kRepeatTicks)
                {
                    const int step = (JoyD & 4) ? -kVolumeStep : kVolumeStep;
                    *volumes[bar] = (std::max)(0, (std::min)(100, *volumes[bar] + step));
                    lastStep = timer ? timer : 1;
                }
                continue;
            }
            lastStep = 0;
        }
        while (JoyD || key || Click)
            process_events();

        if (result == 1)
        {
            saveSettings();
            printf(MAIN_OK "Time Gate volumes: music %d, sound %d\n", *volumes[0], *volumes[1]);
        }
        else
        {
            *volumes[0] = before[0];
            *volumes[1] = before[1];
        }
    }
}

namespace
{
    // The difficulty (TIMEGATE.EXE 16B00): the stained glass with three cards
    // (ITD_RESS 116), all drawn grey and the chosen one in colour (16980):
    // VIII the angel (easy), X the angel and the demon (normal), XV the demon
    // (hard). Left and right change the card, round; a click on a card
    // chooses it; Enter, the action button and Esc keep the lit one (the
    // original has no cancel). Here pointing at a card lights it too. The
    // three values of the difficulty take the card's (AITD3.h)
    const int kTarotPicture = 116;
    struct TarotCard
    {
        int x;
        int greyPicture, colourPicture;
    };
    const TarotCard kTarotCards[3] = { { 41, 96, 97 }, { 122, 98, 99 }, { 206, 100, 101 } };
    const int kTarotY = 37;
    const int kTarotWidth = 72, kTarotHeight = 121;
    const char* const kTarotNames[3] = { "easy", "normal", "hard" };

    int tarotCardAt(float x, float y)
    {
        for (int i = 0; i < 3; i++)
            if (x >= kTarotCards[i].x && x < kTarotCards[i].x + kTarotWidth && y >= kTarotY && y < kTarotY + kTarotHeight)
                return i;
        return -1;
    }

    void tarotScreen()
    {
        std::vector<u8> glass = loadResource(kTarotPicture);
        if (glass.size() < 64000 + 768)
            return;
        palette_t palette;
        copyPalette(glass.data() + 64000, palette);
        convertPaletteIfRequired(palette);
        setPalette(palette);
        std::unique_ptr<sHybrid> grey[3], colour[3];
        for (int i = 0; i < 3; i++)
        {
            grey[i] = loadHybrid(kTarotCards[i].greyPicture);
            colour[i] = loadHybrid(kTarotCards[i].colourPicture);
        }

        int card = (std::max)(0, (std::min)(2, (int)gameDifficulty[0]));
        bool waitRelease = true;
        bool done = false;
        ImVec2 lastMouse = { -1.0f, -1.0f };
        while (!done)
        {
            memcpy(logicalScreen, glass.data(), 64000);
            u8* screen = (u8*)logicalScreen;
            for (int i = 0; i < 3; i++)
                drawHybrid(grey[i].get(), kTarotCards[i].x, kTarotY, screen);
            drawHybrid(colour[card].get(), kTarotCards[card].x, kTarotY, screen);

            osystem_startFrame();
            osystem_CopyBlockPhys((unsigned char*)logicalScreen, 0, 0, 320, 200);
            osystem_stopFrame();
            osystem_flip(NULL);
            process_events();

            const ImVec2 mouse = menuGetGameMouse();
            if (menuMouseMoved(lastMouse, JoyD || key))
            {
                const int hit = tarotCardAt(mouse.x, mouse.y);
                if (hit >= 0)
                    card = hit;
            }
            if (menuMouseClicked())
            {
                const int hit = tarotCardAt(mouse.x, mouse.y);
                if (hit >= 0)
                {
                    card = hit;
                    done = true;
                }
                continue;
            }
            if (waitRelease)
            {
                if (!JoyD && !key && !Click)
                    waitRelease = false;
                continue;
            }
            if (key == kEnter || key == kEscape || Click)
                done = true;
            else if (JoyD & 4)   // left: the previous card
            {
                card = (card + 2) % 3;
                waitRelease = true;
            }
            else if (JoyD & 8)   // right: the next one
            {
                card = (card + 1) % 3;
                waitRelease = true;
            }
        }
        while (JoyD || key || Click || ImGui::GetIO().MouseDown[0])
            process_events();

        for (s16& value : gameDifficulty)
            value = (s16)card;
        printf(MAIN_OK "Time Gate difficulty: %s\n", kTarotNames[card]);
    }
}

namespace
{
    // The sound coin. The original (TIMEGATE.EXE 1A4DF, 32970) set up the
    // sound card of 1996 there, over a plain stone (ITD_RESS 123): lists of
    // devices with their port, IRQ and DMA, written to HMISET.CFG. Today the
    // sound always goes out through the computer, so here, over the same
    // stone, it chooses the instruments of the music: the General MIDI
    // soundfont that comes with the program or one the player put in
    // SoundFonts (platform/midiMusic.cpp). Up and down move, Enter, the
    // action button or a click play the music with it (heard at once), Esc
    // leaves. If no music plays, a song plays while the screen is open.
    // The title is the game's own text "Sound options", in its language
    const int kStonePicture = 123;
    const int kSoundTitleText = 40;
    const int kPreviewSong = 2;          // MUSIC.PAK 0 and 1 are AdLib banks
    const int kPanelLeft = 36, kPanelRight = 283, kPanelTop = 12, kPanelBottom = 187;
    const int kTitleY = 17;
    const int kListTop = 40;
    const int kRowHeight = 14;           // the lines of its text reader (fontHeight)
    const int kVisibleRows = 9;
    const int kHintY = 171;
    const char* const kBuiltInSoundFont = "TimGM6mb";
    const char* const kSoundFontsHint = "SoundFonts   .sf2";

    // A soundfont's name as the game's font can write it: without .sf2, its
    // letters outside ASCII as '?', cut with "..." to fit `width`
    std::string soundFontLabel(const char* fileName, int width)
    {
        std::string label = fileName;
        if (label.size() > 4)
            label.resize(label.size() - 4);
        for (char& c : label)
            if ((unsigned char)c >= 0x80)
                c = '?';
        if (ExtGetSizeFont((u8*)label.c_str()) <= width)
            return label;
        while (!label.empty() && ExtGetSizeFont((u8*)(label + "...").c_str()) > width)
            label.pop_back();
        return label + "...";
    }

    void fillBox(u8* screen, int x1, int y1, int x2, int y2, u8 colour)
    {
        for (int y = y1; y <= y2; y++)
            memset(screen + y * 320 + x1, colour, x2 - x1 + 1);
    }

    void soundScreen()
    {
        std::vector<u8> stone = loadResource(kStonePicture);
        if (stone.size() < 64000 + 768)
            return;
        palette_t palette;
        copyPalette(stone.data() + 64000, palette);
        convertPaletteIfRequired(palette);
        setPalette(palette);
        const u8 panelColour = nearestColour(palette, 24, 24, 28);
        const u8 borderColour = nearestColour(palette, 170, 140, 80);
        const u8 barColour = nearestColour(palette, 95, 62, 30);
        const u8 usedColour = nearestColour(palette, 245, 205, 95);
        const u8 textColour = nearestColour(palette, 205, 205, 205);
        const u8 hintColour = nearestColour(palette, 130, 130, 130);

        // Row 0 is the one that comes with the program, then the player's
        const int count = 1 + osystem_listSoundFonts();
        auto fileOf = [](int row) { return row == 0 ? "" : osystem_soundFontName(row - 1); };
        int used = 0;
        for (int row = 1; row < count; row++)
            if (strcmp(fileOf(row), g_settings.audio.soundFont) == 0)
                used = row;
        int selected = used;
        int first = 0;   // the first row shown

        const bool preview = !osystem_isMidiMusicPlaying();
        const s16 musicBefore = currentMusic;
        if (preview)
            playTimeGateMusicLoop(kPreviewSong);

        bool waitRelease = true;
        bool done = false;
        ImVec2 lastMouse = { -1.0f, -1.0f };
        while (!done)
        {
            if (selected < first)
                first = selected;
            else if (selected >= first + kVisibleRows)
                first = selected - kVisibleRows + 1;

            memcpy(logicalScreen, stone.data(), 64000);
            u8* screen = (u8*)logicalScreen;
            fillBox(screen, kPanelLeft, kPanelTop, kPanelRight, kPanelBottom, borderColour);
            fillBox(screen, kPanelLeft + 1, kPanelTop + 1, kPanelRight - 1, kPanelBottom - 1, panelColour);
            fillBox(screen, kPanelLeft + 8, kListTop - 5, kPanelRight - 8, kListTop - 5, borderColour);
            uiLayer.fill(0);
            SimpleMessage(160, kTitleY, kSoundTitleText, usedColour);
            for (int i = 0; i < kVisibleRows && first + i < count; i++)
            {
                const int row = first + i;
                const int y = kListTop + i * kRowHeight;
                if (row == selected)
                    fillBox(screen, kPanelLeft + 6, y - 1, kPanelRight - 6, y + kRowHeight - 3, barColour);
                SetFont(PtrFont, row == used ? usedColour : textColour);
                const std::string label = row == 0 ? std::string(kBuiltInSoundFont)
                    : soundFontLabel(fileOf(row), kPanelRight - kPanelLeft - 24);
                PrintFont(160 - ExtGetSizeFont((u8*)label.c_str()) / 2, y, (char*)logicalScreen, (u8*)label.c_str());
            }
            SetFont(PtrFont, hintColour);
            PrintFont(160 - ExtGetSizeFont((u8*)kSoundFontsHint) / 2, kHintY, (char*)logicalScreen, (u8*)kSoundFontsHint);

            osystem_startFrame();
            osystem_CopyBlockPhys((unsigned char*)logicalScreen, 0, 0, 320, 200);
            osystem_stopFrame();
            osystem_flip(NULL);
            process_events();

            // The row under the mouse, if any
            const ImVec2 mouse = menuGetGameMouse();
            int hit = -1;
            if (mouse.x >= kPanelLeft && mouse.x <= kPanelRight && mouse.y >= kListTop - 1)
            {
                const int i = (int)(mouse.y - (kListTop - 1)) / kRowHeight;
                if (i < kVisibleRows && first + i < count)
                    hit = first + i;
            }
            if (menuMouseMoved(lastMouse, JoyD || key) && hit >= 0)
                selected = hit;
            const float wheel = ImGui::GetIO().MouseWheel;
            if (wheel != 0.0f)
                selected = (std::max)(0, (std::min)(count - 1, selected + (wheel > 0.0f ? -1 : 1)));

            bool use = false;
            if (menuMouseClicked())
            {
                if (hit >= 0)
                {
                    selected = hit;
                    use = true;
                }
                waitRelease = true;
            }
            else if (waitRelease)
            {
                if (!JoyD && !key && !Click)
                    waitRelease = false;
                continue;
            }
            else if (key == kEscape)
                done = true;
            else if (key == kEnter || Click)
            {
                use = true;
                waitRelease = true;
            }
            else if (JoyD & 1)   // up
            {
                selected = (selected + count - 1) % count;
                waitRelease = true;
            }
            else if (JoyD & 2)   // down
            {
                selected = (selected + 1) % count;
                waitRelease = true;
            }

            if (use && selected != used && osystem_setSoundFont(fileOf(selected)))
            {
                used = selected;
                snprintf(g_settings.audio.soundFont, sizeof(g_settings.audio.soundFont), "%s", fileOf(used));
                saveSettings();
            }
        }
        while (JoyD || key || Click || ImGui::GetIO().MouseDown[0])
            process_events();
        uiLayer.fill(0);
        if (preview)
        {
            stopMusic();
            currentMusic = musicBefore;
        }
        printf(MAIN_OK "Time Gate soundfont: %s\n", used == 0 ? kBuiltInSoundFont : g_settings.audio.soundFont);
    }
}

void timeGatePauseMenu()
{
    osystem_freezeSceneForMenu();
    SaveTimerAnim();
    // The game's colours: the menus change them, and a save keeps them with its picture
    const palette_t gamePalette = currentGamePalette;

    bool loaded = false;
    bool done = false;
    while (!done)
    {
        const int coin = coinMenu(true);
        switch (coin)
        {
        case -1:
            done = true;
            break;
        case kNewGame:
            s_newGameAsked = true;
            FlagGameOver = 1;
            done = true;
            break;
        case kLoad:
            loaded = saveScreen(false);
            done = loaded;
            break;
        case kSave:
            // The copy of the scene that Esc asked for (mainLoop.cpp) is
            // ready after a few frames: the coin menu has drawn far more.
            // It is the picture of the save
            osystem_finalizeScenePreview();
            currentGamePalette = gamePalette;
            done = saveScreen(true);
            break;
        case kQuit:
            s_quitAsked = true;
            FlagGameOver = 1;
            done = true;
            break;
        case kVolume:
            volumeScreen();
            break;
        case kIntro:
            playTimeGateVideo(kIntroCoinVideo);
            break;
        case kDifficulty:
            tarotScreen();
            break;
        case kSound:
            soundScreen();
            break;
        default:
            break;
        }
    }

    if (!loaded)
    {
        currentGamePalette = gamePalette;
        setPalette(currentGamePalette);
    }
    osystem_unfreezeSceneForMenu();
    RestoreTimerAnim();
    FlagInitView = 2;
    while (key || JoyD || Click)
        process_events();
}

void startTimeGate()
{
    // Lines every 14 pixels, as its text reader (TIMEGATE.EXE 1C79C)
    fontHeight = 14;
    for (int video : kIntroVideos)
        playTimeGateVideo(video);
    while (true)
    {
        int coin = s_newGameAsked ? kNewGame : coinMenu(false);
        s_newGameAsked = false;
        if (coin == kQuit)
        {
            FadeOutPhys(16, 0);
            return;
        }
        if (coin == kNewGame)
        {
            FadeOutPhys(16, 0);
            play(true);
        }
        else if (coin == kLoad && saveScreen(false))
            play(false);
        else if (coin == kVolume)
            volumeScreen();
        else if (coin == kIntro)
            playTimeGateVideo(kIntroCoinVideo);
        else if (coin == kDifficulty)
            tarotScreen();
        else if (coin == kSound)
            soundScreen();
        // Saving needs a game
        if (s_quitAsked)
            return;
    }
}
