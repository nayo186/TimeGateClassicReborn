#pragma once

// Time Gate's way finder, for the objects that chase another (movement mode
// 5, TIMEGATE.EXE 17BFB; the finder 2F720-30700 and 40664-40970). Each floor
// has, in PURSUIT.PAK, its obstacles as boxes, some zones and points where no
// way may pass, all on a grid of 8 map units (BUGS.md G49). The way goes
// through the corners of the obstacles: the shortest of at most 16 legs, each
// leg a straight line that crosses no obstacle

// Reads the floor's obstacles (30700, when a floor is loaded; 2F720)
void timeGatePathLoadFloor(int floor);

// The way from a point to another of the grid (304B0): the number of its
// legs, 0 if there is none; `nextX`, `nextZ` are where the first leg ends
int timeGateFindPath(int startX, int startZ, int goalX, int goalZ, int& nextX, int& nextZ);
