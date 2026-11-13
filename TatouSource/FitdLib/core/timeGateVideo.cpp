///////////////////////////////////////////////////////////////////////////////
// Alone in the Dark: Classic Reborn
// Based on FITD by yaz0r and AloneInTheDarkReHaunted, released under GPL
//
// Time Gate's videos (.MUX, TIMEGATE.EXE 1A7D0): the intro (AN0 and AN1 at
// the start, AN1 from its coin) and the scenes the scripts ask for
// (PLAY_SEQUENCE). The format, as the MultimediaWiki describes it ("MUX"):
// a header (signature 0xAA07, its size, the sound frequency at 14, the size
// at 28) and a table of frames, then chunks of 6 bytes (type, subtype, size)
// and their data: 06/07 unsigned 8-bit sound, 65/70 a palette of 8-bit
// colours, 64/6F a picture, 05 the end of a frame, 04 the end of the video.
// A picture: flags (0 whole, 2 changes only; 4 and 6 the same packed with a
// small LZ), where it starts, the size of its control bits and of its colours;
// the bits say how many times each colour goes on (or, in a picture of
// changes, how many points stay). Time Gate's 45 videos use 2, 4 and 6.
// The sound plays as one sample and the pictures follow it.
///////////////////////////////////////////////////////////////////////////////

#include "common.h"
#include "consoleLog.h"
#include "timeGateVideo.h"
#include "languagePack.h"
#include "coreHooks.h"
#include "gameFolder.h"
#include "settings.h"

#include <cstring>
#include <vector>

namespace
{
    const u32 kSignature = 0xAA07;
    const int kWidth = 320;
    const int kHeight = 200;
    const int kEscape = 0x1B;
    const int kEnter = 28;

    u32 read32(const u8* p) { u32 v; memcpy(&v, p, 4); return v; }
    u16 read16(const u8* p) { u16 v; memcpy(&v, p, 2); return v; }

    // The control bits: 32-bit words, the top bit first; bytes and nibbles
    // (the high one first, the low one kept for the next) from the same place
    struct Bits
    {
        const u8* data;
        size_t size;
        size_t pos = 0;
        u32 word = 0;
        int left = 0;
        int keptNibble = -1;

        bool ok(size_t n) const { return pos + n <= size; }
        int nextByte() { return ok(1) ? data[pos++] : 0; }
        int bit()
        {
            if (left == 0)
            {
                word = ok(4) ? read32(data + pos) : 0;
                pos += 4;
                left = 32;
            }
            const int b = (word >> 31) & 1;
            word <<= 1;
            left--;
            return b;
        }
        int nibble()
        {
            if (keptNibble >= 0)
            {
                const int v = keptNibble;
                keptNibble = -1;
                return v;
            }
            const int b = nextByte();
            keptNibble = b & 15;
            return b >> 4;
        }
    };

    // The small LZ of flags 4 and 6: 32 flag bits (the last 1 ends them), a
    // 1 copies nibble + 1 bytes, a 0 repeats nibble + 2 bytes from byte back
    std::vector<u8> unpack(const u8* src, size_t srcSize, size_t outSize)
    {
        std::vector<u8> out;
        out.reserve(outSize);
        size_t p = 0;
        int keptNibble = -1;
        auto nibble = [&]() -> int
        {
            if (keptNibble >= 0)
            {
                const int v = keptNibble;
                keptNibble = -1;
                return v;
            }
            const int b = p < srcSize ? src[p++] : 0;
            keptNibble = b & 15;
            return b >> 4;
        };
        while (p + 4 <= srcSize && out.size() < outSize)
        {
            u32 flags = read32(src + p);
            p += 4;
            for (;;)
            {
                const u32 bit = flags >> 31;
                flags <<= 1;
                if (!flags || out.size() >= outSize)
                    break;
                if (bit)
                {
                    const size_t n = (size_t)nibble() + 1;
                    for (size_t i = 0; i < n && p < srcSize; i++)
                        out.push_back(src[p++]);
                }
                else
                {
                    const size_t back = p < srcSize ? src[p++] : 0;
                    const int n = nibble() + 2;
                    for (int i = 0; i < n; i++)
                        out.push_back((back && back <= out.size()) ? out[out.size() - back] : 0);
                }
            }
        }
        out.resize(outSize, 0);
        return out;
    }

    // A picture into the frame (its points from `start` on)
    void decodePicture(const u8* data, size_t size, std::vector<u8>& frame)
    {
        if (size < 13)
            return;
        const int flags = data[0];
        const u32 start = read32(data + 1);
        const u32 bitsSize = read32(data + 5);
        const u32 coloursSize = read32(data + 9);
        if (flags != 0 && flags != 2 && flags != 4 && flags != 6)
        {
            printf(MAIN_WARN "Time Gate video: picture of kind %d not known" CON_RESET "\n", flags);
            return;
        }
        std::vector<u8> unpacked;
        const u8* bitsData = data + 13;
        if (flags == 4 || flags == 6)
        {
            unpacked = unpack(data + 13, size - 13, (size_t)bitsSize + coloursSize);
            bitsData = unpacked.data();
        }
        else if (13 + (size_t)bitsSize + coloursSize > size)
            return;
        const bool changes = (flags == 2 || flags == 6);
        Bits bits{ bitsData, bitsSize };
        const u8* colours = bitsData + bitsSize;
        size_t colour = 0;
        size_t pos = start;
        const size_t total = frame.size();
        while (pos < total && colour < coloursSize)
        {
            int n;
            if (bits.bit() == 0)
                n = 1;
            else if (bits.bit() == 0)
            {
                const int v = bits.nibble();
                n = v ? v + 4 : bits.nextByte() + 20;
            }
            else if (bits.bit() == 0)
                n = 2;
            else if (bits.bit() == 0)
                n = 3;
            else if (!changes || bits.bit() == 0)
                n = 4;
            else
            {
                pos += bits.nextByte();   // points that stay as they are
                continue;
            }
            const u8 c = colours[colour++];
            for (int i = 0; i < n && pos < total; i++)
                frame[pos++] = c;
        }
    }

    // The sound of the whole video as a WAV file in memory
    std::vector<u8> wavFile(const std::vector<u8>& pcm, int frequency)
    {
        std::vector<u8> wav(44);
        auto put32 = [&](size_t at, u32 v) { memcpy(&wav[at], &v, 4); };
        auto put16 = [&](size_t at, u16 v) { memcpy(&wav[at], &v, 2); };
        memcpy(&wav[0], "RIFF", 4); put32(4, (u32)(36 + pcm.size()));
        memcpy(&wav[8], "WAVEfmt ", 8); put32(16, 16); put16(20, 1); put16(22, 1);
        put32(24, (u32)frequency); put32(28, (u32)frequency); put16(32, 1); put16(34, 8);
        memcpy(&wav[36], "data", 4); put32(40, (u32)pcm.size());
        wav.insert(wav.end(), pcm.begin(), pcm.end());
        return wav;
    }
}

namespace
{
    // A video read whole: its data, where its chunks start, its sound and
    // where each frame ends in it
    struct Video
    {
        std::string name;
        std::vector<u8> data;
        size_t headerSize = 0;
        int frequency = 11111;
        std::vector<u8> pcm;
        std::vector<size_t> frameSound;
    };

    bool readVideo(int number, Video& video, bool quiet)
    {
        video.name = "cd/TIMEGATE/AN" + std::to_string(number) + ".MUX";
        DataFile file;
        if (!file.open(video.name.c_str()))
        {
            if (!quiet)
                printf(MAIN_WARN "Time Gate video %s not found" CON_RESET "\n", video.name.c_str());
            return false;
        }
        std::vector<u8>& data = video.data;
        data.resize((size_t)file.size());
        file.read(data.data(), data.size());
        if (data.size() < 32 || read32(data.data()) != kSignature)
        {
            printf(MAIN_WARN "Time Gate video %s: not a MUX video" CON_RESET "\n", video.name.c_str());
            return false;
        }
        video.headerSize = read16(&data[4]);
        video.frequency = read16(&data[14]) ? read16(&data[14]) : 11111;
        if (read16(&data[28]) != kWidth || read16(&data[30]) != kHeight || video.headerSize >= data.size())
            return false;

        // The sound, all of it, and where each frame ends in it
        for (size_t p = video.headerSize; p + 6 <= data.size();)
        {
            const int type = data[p];
            const u32 size = read32(&data[p + 2]);
            if (p + 6 + size > data.size() || type == 4)
                break;
            if (type == 6 || type == 7)
                video.pcm.insert(video.pcm.end(), &data[p + 6], &data[p + 6] + size);
            else if (type == 5)
                video.frameSound.push_back(video.pcm.size());
            p += 6 + size;
        }
        return true;
    }

    // The modder's sound for the video, with "Custom backgrounds" on:
    // Backgrounds/sequences/AN<n>/sound.wav (or .ogg, .mp3, .flac); empty if
    // there is none, and then the video's own sound plays (BUGS.md G52)
    std::vector<u8> customSound(int number)
    {
        std::vector<u8> sound;
        if (!g_settings.graphics.customBackgrounds)
            return sound;
        for (const char* extension : { "wav", "ogg", "mp3", "flac" })
        {
            const std::string path = std::string(customPicturesFolder()) + "sequences/AN" + std::to_string(number) + "/sound." + extension;
            FILE* file = fopen(path.c_str(), "rb");
            if (!file)
                continue;
            fseek(file, 0, SEEK_END);
            const long size = ftell(file);
            fseek(file, 0, SEEK_SET);
            sound.resize(size > 0 ? (size_t)size : 0);
            const size_t read = sound.empty() ? 0 : fread(sound.data(), 1, sound.size(), file);
            fclose(file);
            sound.resize(read);
            if (!sound.empty())
            {
                printf(MAIN_TAG "Time Gate video AN%d: the sound of %s\n", number, path.c_str());
                return sound;
            }
        }
        return sound;
    }
}

bool decodeTimeGateVideo(int number, const std::function<void(int frame, const unsigned char* pixels, const palette_t& colours)>& onFrame,
    std::vector<unsigned char>& wav)
{
    Video video;
    if (!readVideo(number, video, true))
        return false;
    std::vector<u8> frame(kWidth * kHeight, 0);
    palette_t palette = {};
    int shown = 0;
    const std::vector<u8>& data = video.data;
    for (size_t p = video.headerSize; p + 6 <= data.size();)
    {
        const int type = data[p];
        const u32 size = read32(&data[p + 2]);
        if (p + 6 + size > data.size() || type == 4)
            break;
        const u8* chunk = &data[p + 6];
        if ((type == 0x65 || type == 0x70) && size >= 768)
            copyPalette((u8*)chunk, palette);
        else if (type == 0x64 || type == 0x6F)
            decodePicture(chunk, size, frame);
        else if (type == 5)
            onFrame(shown++, frame.data(), palette);
        p += 6 + size;
    }
    wav = wavFile(video.pcm, video.frequency);
    return true;
}

bool playTimeGateVideo(int number)
{
    Video video;
    if (!readVideo(number, video, false))
        return false;
    const std::string& name = video.name;
    const std::vector<u8>& data = video.data;
    const size_t headerSize = video.headerSize;
    const int frequency = video.frequency;
    const std::vector<u8>& pcm = video.pcm;
    const std::vector<size_t>& frameSound = video.frameSound;
    const std::string pictures = "AN" + std::to_string(number);   // its folder of pictures, sequences/AN<n>
    printf(MAIN_OK "Time Gate video %s: %d frames, %.1f s\n", name.c_str(), (int)frameSound.size(), pcm.size() / (double)frequency);

    SaveTimerAnim();
    const palette_t gamePalette = currentGamePalette;
    // Its frames have no 3D: in the game the screen repainted the last
    // scene over them, only the sound came (the museum's painting, AN28),
    // unless it is a menu's picture, as the scripts' PICTURE (BUGS.md G20)
    const bool menuWasActive = g_menuActive;
    g_menuActive = true;
    // The modder's sound, else its own; the pictures keep the pace of its own
    // sound either way
    std::vector<u8> sound = customSound(number);
    if (sound.empty())
        sound = wavFile(pcm, frequency);
    osystem_playSample((char*)sound.data(), (int)sound.size());   // the mixer keeps a copy

    std::vector<u8> frame(kWidth * kHeight, 0);
    palette_t palette = {};
    bool newPalette = false;   // read, waiting for the frame that uses it
    const unsigned int start = timer;
    int shown = 0;
    bool skipped = false;
    bool waitRelease = key || JoyD || Click;
    for (size_t p = headerSize; p + 6 <= data.size() && !skipped;)
    {
        const int type = data[p];
        const u32 size = read32(&data[p + 2]);
        if (p + 6 + size > data.size() || type == 4)
            break;
        const u8* chunk = &data[p + 6];
        if (type == 0x65 || type == 0x70)
        {
            if (size >= 768)
            {
                // Set when its frame shows: set at once, the frame before
                // showed a moment in the next scene's colours (BUGS.md G21)
                copyPalette((u8*)chunk, palette);
                newPalette = true;
            }
        }
        else if (type == 0x64 || type == 0x6F)
            decodePicture(chunk, size, frame);
        else if (type == 5)
        {
            // Shown when the sound has played up to the start of this frame
            // (the first chunk of sound is that of frame 0); meanwhile the
            // one before stays on screen
            const size_t soundBefore = shown > 0 ? frameSound[shown - 1] : 0;
            const unsigned int due = (unsigned int)(soundBefore * 60 / (size_t)frequency);
            bool first = true;
            while (!skipped && (first || timer - start < due))
            {
                if (timer - start >= due)
                {
                    memcpy(logicalScreen, frame.data(), frame.size());
                    // The modder's picture of it (Backgrounds/sequences/AN<n>/
                    // 005.png...), as the other games' sequences (BUGS.md G52)
                    coreSequenceFrameShown(pictures.c_str(), shown, frame.data());
                    if (newPalette)
                    {
                        setPalette(palette);
                        newPalette = false;
                    }
                }
                first = timer - start < due;
                osystem_startFrame();
                osystem_CopyBlockPhys((unsigned char*)logicalScreen, 0, 0, kWidth, kHeight);
                osystem_stopFrame();
                osystem_flip(NULL);
                process_events();
                if (waitRelease)
                    waitRelease = key || JoyD || Click;
                else if (key == kEscape || key == kEnter || Click)
                    skipped = true;
            }
            shown++;
        }
        p += 6 + size;
    }

    // Only its own sound: the game's looping ones wait paused (BUGS.md G22)
    if (skipped)
        osystem_stopLastSample();
    while (key || JoyD || Click)
        process_events();
    currentGamePalette = gamePalette;
    setPalette(currentGamePalette);
    g_menuActive = menuWasActive;
    RestoreTimerAnim();
    printf(MAIN_OK "Time Gate video %s %s\n", name.c_str(), skipped ? "skipped" : "ended");
    return true;
}
