///////////////////////////////////////////////////////////////////////////////
// Time Gate: Classic Reborn - launcher
// Based on FITD by yaz0r, released under GPL
//
// The improved backgrounds' texture pack (texturePackDownload.h)
///////////////////////////////////////////////////////////////////////////////

#include "texturePackDownload.h"
#include "launcherSettings.h"
#include "updater.h"
#include "userFolder.h"

#include <atomic>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <thread>

namespace fs = std::filesystem;

// The pack published: file name (game and version), where it is and its exact
// size. A new version: change these three; the old file is removed when the
// new one arrives (every file of the same prefix, "nayo_TimeGate*.texture")
static const char* const kPackName = "nayo_TimeGateV1.texture";
static const char* const kPackPrefix = "nayo_TimeGate";
static const char* const kPackUrl =
    "https://drive.usercontent.google.com/download?id=1Seh7AYT9Wb6NTa1jlj9M1gAecXqDlgu2&export=download&confirm=t";
static const uint64_t kPackSize = 330283266;

static std::atomic<int> s_state{ -1 };   // -1: look at UserData
static std::atomic<bool> s_running{ false };

static fs::path userData()
{
    return fs::u8path(launcherFolder()) / kUserDataFolder;
}

static fs::path packPath()
{
    return userData() / kPackName;
}

static fs::path partPath()
{
    return userData() / (std::string(kPackName) + ".part");
}

// The file arrived whole: its size, and a ZIP (a page of the server would not be)
static bool wholePack(const fs::path& path)
{
    std::error_code error;
    if (fs::file_size(path, error) != kPackSize || error)
        return false;
    std::ifstream file(path, std::ios::binary);
    char magic[4] = {};
    file.read(magic, 4);
    return file && magic[0] == 'P' && magic[1] == 'K' && magic[2] == 3 && magic[3] == 4;
}

static void finish(PackState state)
{
    s_state = (int)state;
    s_running = false;
}

static void downloadThread()
{
    std::error_code error;
    fs::create_directories(userData(), error);
    fs::remove(partPath(), error);
    logLine("Texture pack: downloading %s", kPackName);
    const bool downloaded = launcherDownload(kPackUrl, partPath(), 3600);
    if (!downloaded || !wholePack(partPath()))
    {
        logLine("Texture pack: ERROR: %s failed or is not whole (%llu of %llu bytes)", kPackName,
            (unsigned long long)fs::file_size(partPath(), error), (unsigned long long)kPackSize);
        fs::remove(partPath(), error);
        finish(PackState::Failed);
        return;
    }
    fs::rename(partPath(), packPath(), error);
    if (error)
    {
        logLine("Texture pack: ERROR: %s could not be renamed: %s", kPackName, error.message().c_str());
        fs::remove(partPath(), error);
        finish(PackState::Failed);
        return;
    }
    // Older versions of the pack
    for (fs::directory_iterator it(userData(), error), end; !error && it != end; it.increment(error))
    {
        const std::string name = it->path().filename().u8string();
        if (name != kPackName && name.rfind(kPackPrefix, 0) == 0 && it->path().extension() == ".texture")
        {
            std::error_code removeError;
            fs::remove(it->path(), removeError);
            logLine("Texture pack: older version %s removed", name.c_str());
        }
    }
    logLine("Texture pack: %s installed", kPackName);
    finish(PackState::Installed);
}

PackState texturePackState()
{
    if (s_running)
        return PackState::Downloading;
    if (s_state == (int)PackState::Failed)
        return PackState::Failed;
    std::error_code error;
    return fs::exists(packPath(), error) ? PackState::Installed : PackState::Missing;
}

int texturePackProgress()
{
    std::error_code error;
    const uint64_t size = fs::file_size(partPath(), error);
    if (error)
        return 0;
    return (int)((size * 100) / kPackSize);
}

int texturePackMegabytes()
{
    return (int)((kPackSize + 500000) / 1000000);
}

void texturePackStartDownload()
{
    bool expected = false;
    if (!s_running.compare_exchange_strong(expected, true))
        return;
    s_state = (int)PackState::Downloading;
    std::thread(downloadThread).detach();
}

void texturePackRemove()
{
    if (s_running)
        return;
    std::error_code error;
    fs::remove(packPath(), error);
    s_state = -1;
    logLine("Texture pack: %s removed", kPackName);
}
