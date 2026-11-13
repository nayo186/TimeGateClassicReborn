///////////////////////////////////////////////////////////////////////////////
// Alone in the Dark: Classic Reborn
// Based on FITD by yaz0r and AloneInTheDarkReHaunted, released under GPL
//
// Time Gate's music: its songs are MIDI (core/music.cpp turns them into a
// standard MIDI file), which the original sent to a General MIDI synthesizer
// (the GOG version, DOSBox's MPU-401). Here TinySoundFont plays them with the
// General MIDI soundfont next to the program (TimGM6mb.sf2, GPL v2), or one
// the player put in SoundFonts (audio.soundFont, chosen with the sound coin),
// as one more source of the music group (osystemAL_playMusicSource): its
// volume is the music volume of F1 and the launcher.
///////////////////////////////////////////////////////////////////////////////

#include "soloud.h"

#include "common.h"
#include "osystemAL.h"
#include "consoleLog.h"
#include "settings.h"
#include "userFolder.h"

#include <SDL3/SDL.h>

#include <algorithm>
#include <filesystem>
#include <mutex>
#include <string>
#include <vector>

#define TSF_IMPLEMENTATION
#include "tsf.h"
#define TML_IMPLEMENTATION
#include "tml.h"

extern SoLoud::Soloud* gSoloud;
SoLoud::handle osystemAL_playMusicSource(SoLoud::AudioSource& source);   // osystemAL.cpp

static const char* kSoundFontName = "TimGM6mb.sf2";

// The song playing, shared by the game thread (which starts and stops it)
// and the mixer thread (which plays it)
static std::mutex s_songMutex;
static tsf* s_synth = nullptr;
static tml_message* s_song = nullptr;        // the whole song (tml_free)
static tml_message* s_nextMessage = nullptr; // the next one to play
static double s_songTime = 0.0;              // milliseconds since it started
static bool s_loop = false;
static bool s_playing = false;

// Puts every channel back as at the start of a song: no notes, the default
// instrument, drums on channel 10
static void resetChannels()
{
    tsf_reset(s_synth);
    for (int channel = 0; channel < 16; channel++)
        tsf_channel_set_presetnumber(s_synth, channel, 0, channel == 9);
}

// Plays one message of the song; without its notes, only what it changes
// in the channels (instrument, controls, pitch)
static void playMessage(const tml_message& message, bool notes)
{
    switch (message.type)
    {
    case TML_PROGRAM_CHANGE:
        tsf_channel_set_presetnumber(s_synth, message.channel, message.program, message.channel == 9);
        break;
    case TML_NOTE_ON:
        if (notes)
            tsf_channel_note_on(s_synth, message.channel, message.key, message.velocity / 127.0f);
        break;
    case TML_NOTE_OFF:
        if (notes)
            tsf_channel_note_off(s_synth, message.channel, message.key);
        break;
    case TML_PITCH_BEND:
        tsf_channel_set_pitchwheel(s_synth, message.channel, message.pitch_bend);
        break;
    case TML_CONTROL_CHANGE:
        tsf_channel_midi_control(s_synth, message.channel, message.control, message.control_value);
        break;
    default:
        break;
    }
}

// Plays the messages due up to the song time
static void playMessagesDue()
{
    for (; s_nextMessage && s_songTime >= s_nextMessage->time; s_nextMessage = s_nextMessage->next)
        playMessage(*s_nextMessage, true);
}

class MidiInstance : public SoLoud::AudioSourceInstance
{
public:
    unsigned int getAudio(float* buffer, unsigned int samplesToRead, unsigned int bufferSize) override
    {
        m_interleaved.assign(samplesToRead * 2, 0.0f);
        {
            std::lock_guard<std::mutex> lock(s_songMutex);
            if (s_synth && s_playing)
            {
                const double sampleMs = 1000.0 / mBaseSamplerate;
                for (unsigned int done = 0; done < samplesToRead;)
                {
                    const unsigned int block = (std::min)((unsigned int)TSF_RENDER_EFFECTSAMPLEBLOCK, samplesToRead - done);
                    s_songTime += block * sampleMs;
                    playMessagesDue();
                    tsf_render_float(s_synth, &m_interleaved[done * 2], (int)block, 0);
                    done += block;
                    // The end: once more from the start, or silence
                    if (!s_nextMessage)
                    {
                        if (s_loop && s_song)
                        {
                            resetChannels();
                            s_nextMessage = s_song;
                            s_songTime = 0.0;
                        }
                        else if (!tsf_active_voice_count(s_synth))
                        {
                            s_playing = false;
                        }
                    }
                }
            }
        }
        for (unsigned int i = 0; i < samplesToRead; i++)
        {
            buffer[i] = m_interleaved[i * 2];
            buffer[bufferSize + i] = m_interleaved[i * 2 + 1];
        }
        return samplesToRead;
    }

    bool hasEnded() override
    {
        return false;   // a stream that lasts the whole session
    }

private:
    std::vector<float> m_interleaved;
};

class MidiSource : public SoLoud::AudioSource
{
public:
    MidiSource()
    {
        mChannels = 2;
        mBaseSamplerate = (float)osystemAL_getSampleRate();
    }

    SoLoud::AudioSourceInstance* createInstance() override
    {
        return new MidiInstance();
    }
};

static MidiSource* s_source = nullptr;
static SoLoud::handle s_handle = 0;

// A synthesizer with soundfont `name` of SoundFonts, or with the one next to
// the program if empty; nullptr if it cannot be read
static tsf* loadSoundFont(const char* name)
{
    std::string path;
    tsf* synth = nullptr;
    if (name && name[0])
    {
        path = std::string(kSoundFontsFolder) + "/" + name;
        synth = tsf_load_filename(path.c_str());
    }
    else
    {
#ifdef __ANDROID__
        // Android: inside the app (its assets, android/app/build.gradle),
        // which SDL reads by its plain name
        path = kSoundFontName;
        size_t size = 0;
        if (void* data = SDL_LoadFile(kSoundFontName, &size))
        {
            synth = tsf_load_memory(data, (int)size);
            SDL_free(data);
        }
#else
        const char* base = SDL_GetBasePath();
        path = std::string(base ? base : "") + kSoundFontName;
        synth = tsf_load_filename(path.c_str());
#endif
    }
    if (!synth)
    {
        printf(MUSIC_WARN "The soundfont %s could not be read" CON_RESET "\n", path.c_str());
        return nullptr;
    }
    tsf_set_output(synth, TSF_STEREO_INTERLEAVED, osystemAL_getSampleRate(), 0.0f);
    printf(MUSIC_OK "MIDI music with the soundfont %s (%d Hz)\n", path.c_str(), osystemAL_getSampleRate());
    return synth;
}

// Puts `synth` in place of the one playing (and starts the stream the first
// time): the song goes on where it was, its channels as the song left them
static void useSynth(tsf* synth)
{
    tsf* old;
    {
        std::lock_guard<std::mutex> lock(s_songMutex);
        old = s_synth;
        s_synth = synth;
        resetChannels();
        for (const tml_message* message = s_song; message && message != s_nextMessage; message = message->next)
            playMessage(*message, false);
    }
    if (old)
        tsf_close(old);
    if (!s_source)
    {
        s_source = new MidiSource();
        s_handle = osystemAL_playMusicSource(*s_source);
    }
}

// The synthesizer and its stream, made the first time a song plays: with
// the player's soundfont, or the one that comes with the program if it
// cannot be read
static bool startSynth()
{
    if (s_synth)
        return true;
    if (!gSoloud)
        return false;
    tsf* synth = loadSoundFont(g_settings.audio.soundFont);
    if (!synth && g_settings.audio.soundFont[0])
        synth = loadSoundFont("");
    if (!synth)
    {
        printf(MUSIC_WARN "No General MIDI soundfont: the music is silent" CON_RESET "\n");
        return false;
    }
    useSynth(synth);
    return true;
}

static std::vector<std::string> s_soundFonts;   // the last list read

int osystem_listSoundFonts()
{
    namespace fs = std::filesystem;
    s_soundFonts.clear();
    std::error_code error;
    fs::create_directories(kSoundFontsFolder, error);
    for (fs::directory_iterator it(kSoundFontsFolder, error), end; !error && it != end; it.increment(error))
    {
        std::string extension = it->path().extension().string();
        std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char c) { return (char)tolower(c); });
        if (extension == ".sf2" && it->is_regular_file(error))
            s_soundFonts.push_back((const char*)it->path().filename().u8string().c_str());
    }
    std::sort(s_soundFonts.begin(), s_soundFonts.end());
    return (int)s_soundFonts.size();
}

const char* osystem_soundFontName(int index)
{
    return (index >= 0 && index < (int)s_soundFonts.size()) ? s_soundFonts[index].c_str() : "";
}

bool osystem_setSoundFont(const char* name)
{
    if (!gSoloud)
        return false;
    tsf* synth = loadSoundFont(name);
    if (!synth)
        return false;
    useSynth(synth);
    return true;
}

bool osystem_playMidiMusic(const u8* midi, int size, bool loop)
{
    if (!startSynth())
        return false;
    tml_message* song = tml_load_memory(midi, size);
    if (!song)
    {
        printf(MUSIC_WARN "MIDI song could not be read (%d bytes)" CON_RESET "\n", size);
        return false;
    }
    tml_message* old;
    {
        std::lock_guard<std::mutex> lock(s_songMutex);
        old = s_song;
        resetChannels();
        s_song = song;
        s_nextMessage = song;
        s_songTime = 0.0;
        s_loop = loop;
        s_playing = true;
    }
    tml_free(old);
    return true;
}

void osystem_stopMidiMusic()
{
    tml_message* old;
    {
        std::lock_guard<std::mutex> lock(s_songMutex);
        old = s_song;
        s_song = nullptr;
        s_nextMessage = nullptr;
        s_playing = false;
        if (s_synth)
            tsf_note_off_all(s_synth);
    }
    tml_free(old);
}

bool osystem_isMidiMusicPlaying()
{
    std::lock_guard<std::mutex> lock(s_songMutex);
    return s_playing;
}

// Before the mixer shuts down (osystemAL.cpp)
void osystemAL_midiStop()
{
    if (gSoloud && s_source)
        gSoloud->stop(s_handle);
    delete s_source;
    s_source = nullptr;
    s_handle = 0;
    std::lock_guard<std::mutex> lock(s_songMutex);
    tml_free(s_song);
    s_song = nullptr;
    s_nextMessage = nullptr;
    s_playing = false;
    if (s_synth)
        tsf_close(s_synth);
    s_synth = nullptr;
}
