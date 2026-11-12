#pragma once

// Time Gate's saves, in the format of the original (GOG): SAVE<n>.ITD with
// a table of 9 offsets, the picture and the game's blocks (docs/TIMEGATE_EXE.md)

// Loads SAVE<slot>.ITD; false if it is missing or not a Time Gate save
bool loadTimeGateSave(int slot);
// Writes the game as SAVE<slot>.ITD; false if the file cannot be written
bool makeTimeGateSave(int slot);
// The picture of a save (160 x 100 palette indexes) and its palette (768
// bytes, 8-bit colours); false if the save is missing or not Time Gate's
bool readTimeGateSavePicture(int slot, unsigned char picture[160 * 100], unsigned char palette[768]);
// Forgets the blocks kept from the last save loaded (a new game)
void forgetTimeGateSaveExtras();
