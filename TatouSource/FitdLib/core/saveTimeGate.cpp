///////////////////////////////////////////////////////////////////////////////
// Alone in the Dark: Classic Reborn
// Based on FITD by yaz0r and AloneInTheDarkReHaunted, released under GPL
//
// Time Gate's saves, in the format of the original so that GOG's saves load
// here and ours there (TIMEGATE.EXE 1FDA0, docs/TIMEGATE_EXE.md). The file
// starts with 9 offsets (little-endian, 4 bytes each):
//   0x00 the picture: 160 x 100 palette indexes, then the palette (8-bit)
//   0x0C the game: difficulty, room, floor, camera, the world objects (two
//        lists, as AITD3), the 83 CVars, the inventory, the clock and a
//        block the engine does not use (kept as read)
//   0x10 the variables (VARS.ITD), with their size
//   0x14 the 50 objects: AITD3's fields and 10 more values each
//   0x18 the 8 sound channels (kept as read); 0x1C the music: its number
//        in MUSIC.PAK and whether it plays; 0x20 whether the inventory can
//        be opened (statusScreenAllowed)
// AITD3's save code (save.cpp) is not touched: the same order of fields is
// written once here, for reading and writing alike (transfer).
///////////////////////////////////////////////////////////////////////////////

#include "common.h"
#include "consoleLog.h"
#include "gameFolder.h"
#include "gameTime.h"
#include "saveTimeGate.h"
#include "vibration.h"

#include <array>
#include <cstring>
#include <filesystem>
#include <vector>

namespace
{
    const u32 kHeaderSize = 0x24;
    const int kPictureWidth = 160;
    const int kPictureHeight = 100;
    const u32 kPictureSize = kPictureWidth * kPictureHeight + 768;   // 0x4180
    const int kFirstWorldList = 351;     // the first of the two lists of world objects
    const int kFirstSharedColour = 16;   // the colours of a save's picture
    const int kLastSharedColour = 143;
    const int kInventorySize = 50;
    const size_t kUnusedDataSize = 20994;   // the block after the clock (0x84A04)
    const size_t kTailSize = 40 + 5 + 2;    // the blocks at 0x18, 0x1C and 0x20
    const size_t kMusicAt = 40;             // in the tail: the block at 0x1C
    // The block at 0x18: the original's 8 sound channels (0x83A4C, 21 bytes
    // each), their sample (+4) and how many times it plays (+0x14, -1 for
    // ever), 5 bytes each (1FA7F); loading starts again those that play for
    // ever (30B70)
    const int kSoundChannels = 8;
    const size_t kChannelSize = 5;
    const u8 kForever = 0xFF;

    // Values of each object that the engine does not use: two after
    // newAnimInfo, one after hotPointID, two after hardMat, then ten more
    // (Time Gate's own). Kept as read; a new game writes what AITD3 writes
    using ObjectExtras = std::array<s16, 15>;
    ObjectExtras newObjectExtras()
    {
        ObjectExtras extras = {};
        extras[1] = -1;
        return extras;
    }

    // What the engine does not have, kept from the last save loaded
    struct Kept
    {
        bool fromSave = false;
        std::vector<u8> unusedData;
        std::vector<u8> tail;
        std::array<ObjectExtras, NUM_MAX_OBJECT> objectExtras;
    };
    Kept s_kept;

    void keepDefaults()
    {
        s_kept = Kept();
        s_kept.unusedData.assign(kUnusedDataSize, 0);
        // 8 entries of 4 bytes -1 and a 0, then 4 bytes and a 1, then a 1
        for (int i = 0; i < 8; i++)
            s_kept.tail.insert(s_kept.tail.end(), { 0xFF, 0xFF, 0xFF, 0xFF, 0x00 });
        s_kept.tail.insert(s_kept.tail.end(), { 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x00 });
        s_kept.objectExtras.fill(newObjectExtras());
    }

    // Reads from or appends to a buffer, in the file's order
    struct Stream
    {
        bool writing;
        std::vector<u8>& data;
        size_t pos = 0;
        bool ok = true;

        void bytes(void* p, size_t n)
        {
            if (writing)
            {
                const u8* b = (const u8*)p;
                data.insert(data.end(), b, b + n);
                return;
            }
            if (pos + n > data.size())
            {
                ok = false;
                memset(p, 0, n);
                return;
            }
            memcpy(p, &data[pos], n);
            pos += n;
        }
        template <class T> void v16(T& value)
        {
            s16 v = (s16)value;
            bytes(&v, 2);
            if (!writing)
                value = (T)v;
        }
        void v32(unsigned int& value)
        {
            u32 v = value;
            bytes(&v, 4);
            if (!writing)
                value = v;
        }
        // A changing value with its time: only its low 16 bits are kept, the
        // same time ago from the restored clock (timer is read before)
        void real(RealValue& r)
        {
            v16(r.startValue);
            v16(r.endValue);
            v16(r.numSteps);
            u16 memo = (u16)r.memoTicks;
            bytes(&memo, 2);
            if (!writing)
                r.memoTicks = timer - (u16)((u16)timer - memo);
        }
        size_t at() const { return writing ? data.size() : pos; }
    };

    void transferWorldObject(Stream& s, tWorldObject& o)
    {
        s.v16(o.objIndex); s.v16(o.body); s.v16(o.flags); s.v16(o.typeZV);
        s.v16(o.foundBody); s.v16(o.foundName); s.v16(o.foundFlag); s.v16(o.foundLife);
        s.v16(o.x); s.v16(o.y); s.v16(o.z);
        s.v16(o.alpha); s.v16(o.beta); s.v16(o.gamma);
        s.v16(o.stage); s.v16(o.room);
        s.v16(o.lifeMode); s.v16(o.life); s.v16(o.floorLife);
        s.v16(o.anim); s.v16(o.frame); s.v16(o.animType); s.v16(o.animInfo);
        s.v16(o.trackMode); s.v16(o.trackNumber); s.v16(o.positionInTrack);
        s.v16(o.mark);
    }

    // 202 bytes, the original's object in memory
    void transferObject(Stream& s, tObject& o, ObjectExtras& x)
    {
        s.v16(o.indexInWorld); s.v16(o.bodyNum); s.v16(o.objectType); s.v16(o.dynFlags);
        s.v16(o.zv.ZVX1); s.v16(o.zv.ZVX2); s.v16(o.zv.ZVY1); s.v16(o.zv.ZVY2); s.v16(o.zv.ZVZ1); s.v16(o.zv.ZVZ2);
        s.v16(o.screenXMin); s.v16(o.screenYMin); s.v16(o.screenXMax); s.v16(o.screenYMax);
        s.v16(o.roomX); s.v16(o.roomY); s.v16(o.roomZ);
        s.v16(o.worldX); s.v16(o.worldY); s.v16(o.worldZ);
        s.v16(o.alpha); s.v16(o.beta); s.v16(o.gamma);
        s.v16(o.stage); s.v16(o.room);
        s.v16(o.lifeMode); s.v16(o.life);
        s.v32(o.CHRONO); s.v32(o.ROOM_CHRONO);
        s.v16(o.ANIM); s.v16(o.animType); s.v16(o.animInfo);
        s.v16(o.newAnim); s.v16(o.newAnimType); s.v16(o.newAnimInfo);
        s.v16(x[0]); s.v16(x[1]);
        s.v16(o.frame); s.v16(o.numOfFrames); s.v16(o.END_FRAME); s.v16(o.flagEndAnim);
        unsigned int memoTicks = o.memoTicks;
        s.v32(memoTicks);
        o.memoTicks = memoTicks;
        s.v16(o.trackMode); s.v16(o.trackNumber); s.v16(o.MARK); s.v16(o.positionInTrack);
        s.v16(o.stepX); s.v16(o.stepY); s.v16(o.stepZ);
        s.v16(o.animNegX); s.v16(o.animNegY); s.v16(o.animNegZ);
        s.real(o.YHandler);
        s.v16(o.falling);
        s.real(o.rotate);
        s.v16(o.direction); s.v16(o.speed);
        s.real(o.speedChange);
        s.v16(o.COL[0]); s.v16(o.COL[1]); s.v16(o.COL[2]); s.v16(o.COL_BY);
        s.v16(o.HARD_DEC); s.v16(o.HARD_COL); s.v16(o.HIT); s.v16(o.HIT_BY);
        s.v16(o.animActionType); s.v16(o.animActionANIM); s.v16(o.animActionFRAME); s.v16(o.animActionParam);
        s.v16(o.hitForce); s.v16(o.hotPointID);
        s.v16(x[2]);
        s.v16(o.hotPoint.x); s.v16(o.hotPoint.y); s.v16(o.hotPoint.z);
        s.v16(o.hardMat);
        for (int i = 3; i < (int)x.size(); i++)
            s.v16(x[i]);
    }

    bool readFile(int slot, std::vector<u8>& data)
    {
        FILE* file = nullptr;
        fopen_s(&file, saveFilePath(slot, ".ITD").c_str(), "rb");
        if (!file)
            return false;
        fseek(file, 0, SEEK_END);
        const long size = ftell(file);
        fseek(file, 0, SEEK_SET);
        data.resize(size > 0 ? (size_t)size : 0);
        const bool read = !data.empty() && fread(data.data(), 1, data.size(), file) == data.size();
        fclose(file);
        return read;
    }

    u32 slotOf(const std::vector<u8>& data, int index)
    {
        u32 v;
        memcpy(&v, &data[index * 4], 4);
        return v;
    }

    // A Time Gate save: the picture right after the header, the game after it
    bool isTimeGateSave(const std::vector<u8>& data)
    {
        if (data.size() < kHeaderSize + kPictureSize)
            return false;
        const u32 offsets[6] = { slotOf(data, 0), slotOf(data, 3), slotOf(data, 4), slotOf(data, 5), slotOf(data, 6), slotOf(data, 8) };
        return offsets[0] == kHeaderSize && offsets[1] == kHeaderSize + kPictureSize &&
            offsets[1] < offsets[2] && offsets[2] < offsets[3] && offsets[3] < offsets[4] &&
            offsets[4] < offsets[5] && offsets[5] <= data.size();
    }
}

namespace
{
    // The shared colour nearest to (r, g, b) in the game's palette
    u8 sharedColour(int r, int g, int b)
    {
        int best = kFirstSharedColour;
        int bestDistance = 1 << 30;
        for (int s = kFirstSharedColour; s <= kLastSharedColour; s++)
        {
            const int dr = currentGamePalette[s][0] - r, dg = currentGamePalette[s][1] - g, db = currentGamePalette[s][2] - b;
            const int distance = dr * dr + dg * dg + db * db;
            if (distance < bestDistance)
            {
                best = s;
                bestDistance = distance;
            }
        }
        return (u8)best;
    }

    // The picture of a save: the scene as it was drawn, the 3D objects too
    // (the copy taken when Esc opened the menu, its colours averaged over
    // each point of the picture); without it, the camera's background
    void makePicture(std::vector<u8>& picture)
    {
        const u8* scene = osystem_getScenePreviewData();
        const int width = osystem_getScenePreviewWidth();
        const int height = osystem_getScenePreviewHeight();
        if (scene && width >= kPictureWidth && height >= kPictureHeight)
        {
            for (int y = 0; y < kPictureHeight; y++)
                for (int x = 0; x < kPictureWidth; x++)
                {
                    const int x1 = x * width / kPictureWidth, x2 = (x + 1) * width / kPictureWidth;
                    const int y1 = y * height / kPictureHeight, y2 = (y + 1) * height / kPictureHeight;
                    int sum[3] = {}, count = 0;
                    for (int sy = y1; sy < y2; sy++)
                        for (int sx = x1; sx < x2; sx++, count++)
                            for (int c = 0; c < 3; c++)
                                sum[c] += scene[((size_t)sy * width + sx) * 4 + c];
                    picture[y * kPictureWidth + x] = sharedColour(sum[0] / count, sum[1] / count, sum[2] / count);
                }
            return;
        }
        if (!aux)
            return;
        u8 colours[256];
        for (int c = 0; c < 256; c++)
            colours[c] = sharedColour(currentGamePalette[c][0], currentGamePalette[c][1], currentGamePalette[c][2]);
        for (int y = 0; y < kPictureHeight; y++)
            for (int x = 0; x < kPictureWidth; x++)
                picture[y * kPictureWidth + x] = colours[(u8)aux[(y * 2) * 320 + x * 2]];
    }
}

void forgetTimeGateSaveExtras()
{
    keepDefaults();
}

bool readTimeGateSavePicture(int slot, unsigned char picture[160 * 100], unsigned char palette[768])
{
    std::vector<u8> data;
    if (!readFile(slot, data) || !isTimeGateSave(data))
        return false;
    memcpy(picture, &data[kHeaderSize], kPictureWidth * kPictureHeight);
    memcpy(palette, &data[kHeaderSize + kPictureWidth * kPictureHeight], 768);
    return true;
}

bool loadTimeGateSave(int slot)
{
    std::vector<u8> data;
    if (!readFile(slot, data) || !isTimeGateSave(data))
    {
        printf(MAIN_WARN "Save %d: not a Time Gate save" CON_RESET "\n", slot);
        return false;
    }

    forgetPlayerLife();   // the life it brings is not a hit (vibration.cpp)
    LoadWorld();
    initVars();
    keepDefaults();

    Stream s{ false, data };
    s.pos = slotOf(data, 3);
    s.v16(gameDifficulty[0]); s.v16(gameDifficulty[1]); s.v16(gameDifficulty[2]);
    s.v16(currentRoom); s.v16(g_currentFloor); s.v16(NumCamera);
    s.v16(currentWorldTarget); s.v16(currentCameraTargetActor); s.v16(maxObjects);
    s16 lists[2] = {};
    s.v16(lists[0]); s.v16(lists[1]);
    const int worldObjects = (std::max)(0, lists[0] + lists[1]);
    if (worldObjects > (int)ListWorldObjets.size())
        ListWorldObjets.resize(worldObjects);
    for (int i = 0; i < worldObjects; i++)
        transferWorldObject(s, ListWorldObjets[i]);
    for (s16& cvar : CVars)
        s.v16(cvar);

    // One inventory: the second stays empty
    s.v16(inHandTable[0]); s.v16(numObjInInventoryTable[0]);
    for (int i = 0; i < kInventorySize; i++)
        s.v16(inventoryTable[0][i]);
    if (numObjInInventoryTable[0] < 0 || numObjInInventoryTable[0] > kInventorySize)
        numObjInInventoryTable[0] = 0;
    for (int inventoryId = 1; inventoryId < NUM_MAX_INVENTORY; inventoryId++)
    {
        inHandTable[inventoryId] = -1;
        numObjInInventoryTable[inventoryId] = 0;
    }
    currentInventory = 0;

    s.v16(FlagGameOver); s.v16(lightOff); s.v16(shakingAmplitude);
    s.v32(timer);
    // The clock itself, not only its copy (BUGS.md C29)
    setGameClock(timer);
    resetFoundObjectChronos();
    // The original's frame counter (0x7178C), as many as the clock in its
    // saves: the engine counts with the clock alone
    unsigned int frames = 0;
    s.v32(frames);
    const u32 varsOffset = slotOf(data, 4);
    if (!s.ok || s.pos > varsOffset)
    {
        printf(MAIN_WARN "Save %d: its game block does not fit" CON_RESET "\n", slot);
        return false;
    }
    s_kept.unusedData.assign(data.begin() + s.pos, data.begin() + varsOffset);

    const int camera = NumCamera;
    LoadEtage(g_currentFloor);
    NumCamera = -1;
    ChangeSalle(currentRoom);

    // The variables, as many as the game has
    s.pos = varsOffset;
    u16 savedVarSize = 0;
    s.v16(savedVarSize);
    std::vector<u8> savedVars(savedVarSize);
    if (savedVarSize)
        s.bytes(savedVars.data(), savedVarSize);
    memcpy(vars, savedVars.data(), (std::min<int>)(savedVarSize, varSize));

    s.pos = slotOf(data, 5);
    for (int i = 0; i < NUM_MAX_OBJECT; i++)
    {
        transferObject(s, ListObjets[i], s_kept.objectExtras[i]);
        // Not saved: its script sets it again on its next pass
        ListObjets[i].zvFollowsAnim = false;
    }
    s_kept.tail.assign(data.begin() + slotOf(data, 6), data.end());
    // Its last 2 bytes (0x61770): whether the inventory can be opened
    if (s_kept.tail.size() == kTailSize)
        statusScreenAllowed = (s16)(s_kept.tail[kTailSize - 2] | (s_kept.tail[kTailSize - 1] << 8));
    s_kept.fromSave = true;

    // The music that was playing starts again, as the original after loading
    // (TIMEGATE.EXE 30970: 0x8B344 the song, 0x8B358 whether it plays;
    // BUGS.md G13)
    if (s_kept.tail.size() == kTailSize)
    {
        const u8* music = s_kept.tail.data() + kMusicAt;
        const s32 song = (s32)(music[0] | (music[1] << 8) | (music[2] << 16) | ((u32)music[3] << 24));
        currentMusic = -1;
        if (music[4] == 1 && song >= 0)
            playMusic(song);
        else
            stopMusic();
    }

    // The sounds that played for ever, each on its channel (the lasers' hum,
    // which their script starts once when the hero comes in, BUGS.md G23;
    // every channel, BUGS.md G33)
    if (s_kept.tail.size() == kTailSize)
        for (int channel = 0; channel < kSoundChannels; channel++)
        {
            const u8* entry = s_kept.tail.data() + channel * kChannelSize;
            const s32 sample = (s32)(entry[0] | (entry[1] << 8) | (entry[2] << 16) | ((u32)entry[3] << 24));
            if (sample >= 0 && entry[4] == kForever)
                playChannelSound(sample, channel, true);
        }

    for (int i = 0; i < NUM_MAX_OBJECT; i++)
    {
        if (ListObjets[i].indexInWorld != -1 && ListObjets[i].bodyNum != -1 && ListObjets[i].ANIM != -1)
        {
            sBody* body = HQR_Get(HQ_Bodys, ListObjets[i].bodyNum);
            sAnimation* anim = HQR_Get(HQ_Anims, ListObjets[i].ANIM);
            SetAnimObjet(ListObjets[i].frame, anim, body);
        }
    }
    NewNumCamera = camera;

    printf(MAIN_OK "Save %d loaded (Time Gate): floor %d, room %d, camera %d%s\n", slot, g_currentFloor, currentRoom, camera,
        s.ok ? "" : ", but the file was shorter than expected");
    return s.ok;
}

bool makeTimeGateSave(int slot)
{
    if (s_kept.unusedData.size() != kUnusedDataSize || s_kept.tail.size() != kTailSize)
        keepDefaults();

    // Special objects (smoke, sparks...) are not saved (BUGS.md A7)
    for (int i = 0; i < NUM_MAX_OBJECT; i++)
        if (ListObjets[i].indexInWorld == -2)
            DeleteObjet(i);

    std::vector<u8> data(kHeaderSize, 0);
    Stream s{ true, data };
    u32 offsets[9] = {};
    offsets[0] = kHeaderSize;

    // The picture, with the colours every room shares (16 to 143), as GOG's
    // saves: the save screen has other colours from 144 on, and the original
    // showed those pixels purple on its drum
    std::vector<u8> picture(kPictureWidth * kPictureHeight, 0);
    makePicture(picture);
    s.bytes(picture.data(), picture.size());
    for (int i = 0; i < 256; i++)
        s.bytes(currentGamePalette[i].data(), 3);

    offsets[3] = (u32)s.at();
    s.v16(gameDifficulty[0]); s.v16(gameDifficulty[1]); s.v16(gameDifficulty[2]);
    s.v16(currentRoom); s.v16(g_currentFloor); s.v16(NumCamera);
    s.v16(currentWorldTarget); s.v16(currentCameraTargetActor); s.v16(maxObjects);
    s16 lists[2] = { (s16)(std::min<int>)(maxObjects, kFirstWorldList), 0 };
    lists[1] = (s16)(maxObjects - lists[0]);
    s.v16(lists[0]); s.v16(lists[1]);
    for (int i = 0; i < maxObjects && i < (int)ListWorldObjets.size(); i++)
        transferWorldObject(s, ListWorldObjets[i]);
    for (s16& cvar : CVars)
        s.v16(cvar);
    s.v16(inHandTable[0]); s.v16(numObjInInventoryTable[0]);
    for (int i = 0; i < kInventorySize; i++)
        s.v16(inventoryTable[0][i]);
    s.v16(FlagGameOver); s.v16(lightOff); s.v16(shakingAmplitude);
    // The time the game stopped at, not the one running inside the menu
    unsigned int clock = gameClockForSave();
    s.v32(clock);
    // The frame counter as the clock, as the original's saves have them: with
    // an older count the original waited for it and seemed to hang (BUGS.md G4)
    s.v32(clock);
    s.bytes(s_kept.unusedData.data(), s_kept.unusedData.size());

    offsets[4] = (u32)s.at();
    u16 savedVarSize = (u16)varSize;
    s.v16(savedVarSize);
    s.bytes(vars, varSize);

    offsets[5] = (u32)s.at();
    for (int i = 0; i < NUM_MAX_OBJECT; i++)
        transferObject(s, ListObjets[i], s_kept.objectExtras[i]);

    offsets[6] = (u32)s.at();
    offsets[7] = offsets[6] + 40;
    offsets[8] = offsets[7] + 5;
    // The sounds now: each channel's loop, as the original (BUGS.md G33)
    for (int channel = 0; channel < kSoundChannels; channel++)
    {
        u8* entry = s_kept.tail.data() + channel * kChannelSize;
        const s32 sample = channelLoopSample(channel);
        for (int i = 0; i < 4; i++)
            entry[i] = (u8)((u32)sample >> (8 * i));
        entry[4] = sample >= 0 ? kForever : 0;
    }
    // The music now: its song (-1 none) and whether it plays
    const s32 song = currentMusic;
    for (int i = 0; i < 4; i++)
        s_kept.tail[kMusicAt + i] = (u8)((u32)song >> (8 * i));
    s_kept.tail[kMusicAt + 4] = song >= 0 ? 1 : 0;
    s_kept.tail[kTailSize - 2] = (u8)statusScreenAllowed;
    s_kept.tail[kTailSize - 1] = (u8)(statusScreenAllowed >> 8);
    s.bytes(s_kept.tail.data(), s_kept.tail.size());
    memcpy(data.data(), offsets, sizeof(offsets));

    std::error_code folderError;
    if (savesFolder()[0])
        std::filesystem::create_directories(savesFolder(), folderError);
    FILE* file = nullptr;
    fopen_s(&file, saveFilePath(slot, ".ITD").c_str(), "wb");
    if (!file)
    {
        printf(MAIN_WARN "Save %d: the file cannot be written" CON_RESET "\n", slot);
        return false;
    }
    const bool written = fwrite(data.data(), 1, data.size(), file) == data.size();
    fclose(file);
    printf(MAIN_OK "Save %d written (Time Gate): %d bytes, floor %d, room %d\n", slot, (int)data.size(), g_currentFloor, currentRoom);
    return written;
}
