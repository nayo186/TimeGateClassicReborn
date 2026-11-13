#pragma once

// Plays Time Gate's video AN<number>.MUX (the CD's TIMEGATE folder): its
// pictures, its palette and its sound (in the voices and effects volume).
// Esc, Enter or a button skips it. False if the video is missing
bool playTimeGateVideo(int number);

#include "palette.h"
#include <functional>
#include <vector>

// The export of a video for modders (customBackgrounds.cpp; BUGS.md G52):
// each picture of AN<number>.MUX with its colours, in order, and its sound as
// a WAV file. False if the video is missing
bool decodeTimeGateVideo(int number, const std::function<void(int frame, const unsigned char* pixels, const palette_t& colours)>& onFrame,
    std::vector<unsigned char>& wav);
