///////////////////////////////////////////////////////////////////////////////
// Alone in the Dark: Classic Reborn
// Based on FITD by yaz0r and AloneInTheDarkReHaunted, released under GPL
//
// Time Gate's chained sounds (TIMEGATE.EXE 30F30, 30FB0, 31030, and the main
// loop at 27F3A): the scripts give a list of sounds of SAMPLES.PAK and the
// game plays them one after another, the next as soon as one ends. That is
// how its characters talk (the man at the museum's door), with their voices
// and no subtitles. The life opcode 0x61 plays a list once and marks when it
// has ended (evalVar 43); 0x79 plays one again and again; 0x7A stops it.
///////////////////////////////////////////////////////////////////////////////

#include "common.h"
#include "consoleLog.h"
#include "timeGateSound.h"

#include <cstdlib>

namespace
{
    struct SoundList
    {
        std::vector<int> samples;
        size_t next = 0;
        bool looping = false;
        bool active = false;
    };
    SoundList s_list;
    bool s_ended = true;

    void playNext()
    {
        if (s_list.next >= s_list.samples.size())
            return;
        const int sample = s_list.samples[s_list.next++];
        const int size = getPakSize("SAMPLES", sample);
        char* data = size > 0 ? loadPak("SAMPLES", sample) : nullptr;
        if (!data)
            return;
        osystem_playVoice(data, size);   // the mixer keeps a copy
        free(data);
        printf(MAIN_TAG "[SOUND] Time Gate chained sound %d (%d of %d)%s\n", sample, (int)s_list.next,
            (int)s_list.samples.size(), s_list.looping ? ", looping" : "");
    }
}

void timeGatePlaySounds(const std::vector<int>& samples, bool looping)
{
    osystem_stopVoice();
    s_list = SoundList();
    s_list.samples = samples;
    s_list.looping = looping;
    s_list.active = !samples.empty();
    if (!looping)
        s_ended = samples.empty();
    playNext();
}

void timeGateStopSounds()
{
    if (!s_list.active)
        return;
    osystem_stopVoice();
    s_list = SoundList();
    s_ended = true;
}

void updateTimeGateSounds()
{
    if (!s_list.active || osystem_isVoicePlaying())
        return;
    if (s_list.next >= s_list.samples.size())
    {
        if (!s_list.looping)
        {
            s_list.active = false;
            s_ended = true;
            return;
        }
        s_list.next = 0;
    }
    playNext();
}

bool timeGateSoundsEnded()
{
    return s_ended;
}
