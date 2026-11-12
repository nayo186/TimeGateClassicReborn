#pragma once

#include <vector>

// Time Gate's chained sounds of SAMPLES.PAK (its voices): plays them one
// after another, at once (TIMEGATE.EXE 30F30, life 0x61); `looping` starts
// again from the first when the last ends, until timeGateStopSounds
// (30FB0 and 31030, life 0x79 and 0x7A). One channel, as the original: a new
// list stops the one playing
void timeGatePlaySounds(const std::vector<int>& samples, bool looping);
// Stops the list playing
void timeGateStopSounds();
// Every frame: the next sound when one ends
void updateTimeGateSounds();
// True when the last list that is not looping has ended (evalVar 43, 0x61986)
bool timeGateSoundsEnded();
