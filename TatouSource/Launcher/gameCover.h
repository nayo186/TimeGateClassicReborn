///////////////////////////////////////////////////////////////////////////////
// Alone in the Dark: Classic Reborn - launcher
// Based on FITD by yaz0r and AloneInTheDarkReHaunted, released under GPL
//
// Game covers: the 3:4 picture of each game of the edition (AITD1, AITD2,
// AITD3, Jack in the Dark belonging to AITD1; or Time Gate). The pictures are
// part of the project (Launcher/assets/covers) and are compiled into
// Launcher.exe by embedCovers.cmake, so they cannot be replaced from outside.
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <SDL3/SDL.h>

struct GameCover
{
    SDL_Texture* colour = nullptr;
    SDL_Texture* grey = nullptr;   // shown while the game has no data folder
#ifdef CLASSIC_REBORN_EDITION_TIMEGATE
    SDL_Texture* background = nullptr;   // blurred and darkened, behind the whole window
#endif
    int width = 0;
    int height = 0;
};

// game: 0 = AITD1, 1 = AITD2, 2 = AITD3 (Alone's edition); 3 = Time Gate
GameCover loadGameCover(SDL_Renderer* renderer, int game);

// Frees the textures of a cover
void freeGameCover(GameCover& cover);

#ifdef CLASSIC_REBORN_EDITION_TIMEGATE
// Time Gate's controller drawing (Shared/ui/controllerPicture.h) as a
// texture, and its height / width; nullptr if it could not be read
SDL_Texture* loadControllerDrawing(SDL_Renderer* renderer, float& aspect);
#endif
