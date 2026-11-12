///////////////////////////////////////////////////////////////////////////////
// Alone In The Dark Re-Haunted
// Copyright (C) 2026 Infogrames / Spacefarer Retro Remasters LLC
// Based on FITD by yaz0r, Re-haunted is released under GPL
// Author: Jake Jackson (jake@spacefarergames.com)
//
// OS abstraction layer interface
///////////////////////////////////////////////////////////////////////////////

#ifndef _OSYSTEM_H_
#define _OSYSTEM_H_

#ifdef __cplusplus
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include <cstddef>
#include <functional>
#include <string>
#include <vector>

#include "palette.h"

#endif

#define byte char
#define u8 unsigned char
#ifndef __cplusplus
#define true 1
#define false 0
#endif

    enum e_rendererType
    {
        OPENGL_OLD,
        OPENGL_ES,
        OPENGL_3_2,
    };

    extern enum e_rendererType g_rendererType;

	// Starts the system layer: window, graphics, sound, controller and ImGui
	void osystem_init();

	extern int osystem_mouseRight;
	extern int osystem_mouseLeft;

	// Draws the 320x200 background (or the player's picture) at the back of the scene
	void osystem_drawBackground();
	// Start of a game frame: waits for the render thread and opens the frame
	u32 osystem_startOfFrame();
	// End of a game frame: draws what is pending and the text layer, and shows the frame
	void osystem_endOfFrame();


	// Sets the game palette (256 RGB colours) and clears the screen fade
	void osystem_setPalette(unsigned byte * palette);
	// Sets the game palette (256 RGB colours) and clears the screen fade
	void osystem_setPalette(palette_t* palette);
	// Dark room (lightOff): covers the scene drawn so far with black, except
	// a lit circle around litX, litY (320x200) if lit; the text layer is
	// drawn after it and the glass of this frame is skipped
	void osystem_drawDarkness(bool lit, int litX, int litY);
	// Draws the 3D primitives still pending
	void osystem_flip(unsigned char *videoBuffer);
	// Sends a rectangle of the 320x200 screen to the graphics card
	void osystem_CopyBlockPhys(unsigned char* videoBuffer, int left, int top, int right, int bottom);
	// Plays a sound effect read from a file by its name
	void osystem_playSampleFromName(char* sampleName);
	// Plays a sound effect from memory (the game's VOC data)
	void osystem_playSample(char* samplePtr,int size);
	// Plays a sound effect in a loop (ambient sound; one at a time), at
	// `volume` times its level
	void osystem_playLoopingSample(char* samplePtr, int size, float volume = 1.0f);
	// Stops every sound effect
	void osystem_stopSample();
	// Stops the looping sound effects
	void osystem_stopLoopingSamples();
	// Stops the sound effect started last, if it is not a looping one
	void osystem_stopLastSample();
	// True while a sound effect is playing
	bool osystem_isSamplePlaying();
	// True while a looping sound effect is playing
	bool osystem_isLoopingSamplePlaying();
	// Time Gate's 8 sound channels (TIMEGATE.EXE 30BC0, 30C90): plays a
	// sound on a channel, once or in a loop, at `volume` times its level; the
	// one the channel had stops
	void osystem_playChannelSample(int channel, char* samplePtr, int size, bool looping, float volume = 1.0f);
	// Stops the sound of a channel (TIMEGATE.EXE 30DB0)
	void osystem_stopChannel(int channel);
	// The volume of a channel's sound, 0 to 1 (TIMEGATE.EXE 31050)
	void osystem_setChannelVolume(int channel, float volume);
	// True while a channel's sound plays
	bool osystem_isChannelPlaying(int channel);
	// Before a video of the scripts (Time Gate): stops the sound effects and
	// the voice, and pauses the looping effects and the music
	void osystem_pauseSoundsForVideo();
	// After it: the looping effects and the music go on where they were
	void osystem_resumeSoundsAfterVideo();
	// Time Gate's channel of chained sounds (its voices): plays a WAV in the
	// voices and effects volume, stopping the one before
	void osystem_playVoice(char* samplePtr, int size);
	// True while the sound of osystem_playVoice is playing
	bool osystem_isVoicePlaying();
	// Stops the sound of osystem_playVoice
	void osystem_stopVoice();

	// Plays a narrator voice by its file name (loose file or GAME.GOG)
	void osystem_playVO(const char* voFileName);
	// Plays the narrator voice of a book or message by its number
	void osystem_playVocByIndex(int vocIndex);
	// Plays the narrator voice of one page of a book
	void osystem_playVocPage(int vocIndex, int page);
	// Plays a CD track as the narrator voice for `seconds` (AITD2's books)
	void osystem_playVoiceTrack(int trackId, int seconds);
	// Stops the narrator voice
	void osystem_stopVO();

	// VOC pre-decode cache for seamless page turns during book reading
	void osystem_preDecodeVocPage(int vocIndex, int page);
	// Forgets the voices decoded in the background
	void osystem_clearVocCache();
	// The narrators with voices: the folders of Voices in the working folder
	std::vector<std::string> osystem_findNarrators();

	extern float g_voPitchMultiplier;
	extern bool g_playWorldActive;
	extern bool g_3dFlushedThisFrame;
	extern bool g_menuActive;
	//    void getMouseStatus(mouseStatusStruct * mouseData);

	// Makes the texture of a foreground mask from its 320x200 shape
	void osystem_createMask(const std::array<u8, 320 * 200>& mask, int roomId, int maskId, int actualRoomNumber, int maskX1, int maskY1, int maskX2, int maskY2);
	// Draws a foreground mask of the camera over the objects behind it
	void osystem_drawMask(int roomId, int maskId);
	// Draws a 2D drawing (AITD2's sprites) at the current depth slice: `layer`
	// is 320x200 pairs of colour and covered (1)
	void osystem_drawSpriteLayer(const u8* layer);

	// Opens a frame of the renderer (creates its resources the first time)
	void osystem_startFrame();
	// Empty: the renderer needs nothing at the end of a frame
	void osystem_stopFrame();

	// Sets the drawing clip rectangle (320x200 coordinates)
	void osystem_setClip(float left, float top, float right, float bottom);
	// Removes the drawing clip rectangle
	void osystem_clearClip();


	// Adds a filled polygon of a 3D model (plain, stone, ramp or glass by
	// polyType); modelPoints: its vertices on the model and the model's size of
	// a pixel of the game there (x, y, z, size each), for the grain of stone
	void osystem_fillPoly(float* buffer, int numPoint, unsigned char color,u8 polyType, const float* modelPoints);
	// Time Gate: a face with a texture (256 texels wide, palette colours, 0
	// transparent; textureId names it, texels/textureHeight its data, uv the
	// texel of each point) or, textureId -1, with one colour; shade is the
	// level of each point in the shade table (0 = as it is)
	void osystem_fillTexturedPoly(float* buffer, int numPoint, int textureId, const u8* texels, int textureHeight,
		const float* uv, const float* shade, unsigned char color);
	// Forgets a Time Gate texture made by osystem_fillTexturedPoly: the next
	// face that uses its number makes it again from its new texels
	void osystem_forgetTimeGateTexture(int textureId);
	// Adds a 3D line
	void osystem_draw3dLine(float x1, float y1, float z1, float x2, float y2, float z2, unsigned char color);
	// Adds the four edges of a 3D quad as lines
	void osystem_draw3dQuad(float x1, float y1, float z1, float x2, float y2, float z2, float x3, float y3, float z3, float x4, float y4, float z4, unsigned char color, int transparency);
	// Adds a sphere of a 3D model (a disc facing the camera)
	void osystem_drawSphere(float X, float Y, float Z, u8 color, u8 material, float size);
	// Adds a point of a 3D model (a small disc facing the camera)
	void osystem_drawPoint(float X, float Y, float Z, u8 color, u8 material, float size);
	// Sends the polygons, spheres and lines of the frame to the graphics card
	void osystem_flushPendingPrimitives();
	// Draws the text layer (uiLayer) over the scene
	void osystem_drawUILayer();



	// Plays a CD music track (loose file or GAME.GOG)
	int osystem_playTrack(int trackId);
	// Stops the CD music
	void osystem_stopTrack();
	// True while a CD music track (CD, loose file or GAME.GOG) is still playing
	int osystem_isTrackPlaying();
	// Starts the AdLib music output (Jack in the Dark)
	void osystem_playAdlib();
	// Time Gate: plays a standard MIDI file (core/music.cpp makes it from the
	// game's songs) with the General MIDI soundfont next to the program, once
	// or again and again; false if it cannot play
	bool osystem_playMidiMusic(const u8* midi, int size, bool loop);
	// Time Gate: the shade table its lit faces use (64 rows of 256 colours, row
	// 0 the colours as they are, the higher rows darker)
	void osystem_setShadeTable(const u8* table);
	// Stops the MIDI music
	void osystem_stopMidiMusic();
	// True while a MIDI song is still playing
	bool osystem_isMidiMusicPlaying();
	// Time Gate: how many soundfonts (.sf2) the player put in SoundFonts
	// (made if missing); the list is read again each time
	int osystem_listSoundFonts();
	// The file name of soundfont `index` of that list
	const char* osystem_soundFontName(int index);
	// Plays the music with soundfont `name` of SoundFonts, or with the one
	// that comes with the program if empty, the song going on where it was;
	// false if it cannot be read (the music keeps the one it had)
	bool osystem_setSoundFont(const char* name);

	// Scene preview capture for pause menu
	void osystem_updateSceneSnapshot();
	// Freezes the scene snapshot for the pause menu preview
	void osystem_captureScenePreview();
	// Marks the pause menu preview as ready
	void osystem_finalizeScenePreview();
	// The pixels of the pause menu preview, or nullptr
	unsigned char* osystem_getScenePreviewData();
	// Width of the pause menu preview (0 if none)
	int osystem_getScenePreviewWidth();
	// Height of the pause menu preview (0 if none)
	int osystem_getScenePreviewHeight();

	// Freeze/unfreeze scene snapshot for menu backgrounds
	void osystem_freezeSceneForMenu();
	// The scene snapshot follows the game again (menu closed)
	void osystem_unfreezeSceneForMenu();
	// Draws the frozen scene behind a menu
	void osystem_drawFrozenSceneBackground();

	// Painter's order (Original mode): the objects drawn next use depth slice
	// `slice` (0 = nearest); -1 = plain depth (BUGS.md V21)
	static const int OSYSTEM_DEPTH_SLICES = 128;
	// Window position (pixels) to the 320x200 game screen, through the scene
	// rectangle (graphics.aspect leaves black bars); false before the first frame
	bool osystem_windowToGame(float windowX, float windowY, float* gameX, float* gameY);
	// Rectangle of the window (pixels) where the play area is drawn: x, y, width, height
	void osystem_getSceneRect(int rect[4]);

	// Plain screens (title armadillo, intro screens, books) suspend the screen
	// effects without changing the player's options; calls nest (BUGS.md C15)
	void osystem_suspendScreenEffects(bool suspend);

	// Limits the depth drawn (-1: all), used to draw objects in front of or behind a mask
	void osystem_setDepthSlice(int slice);
	// True if a depth slice was used this frame
	bool osystem_depthSlicedThisFrame();
	// Draws the glass polygons of the frame (Original mode), after the scene
	void osystem_drawDeferredGlass();
	// True if the frozen image was drawn since the last call (screen effects
	// reuse its ambient occlusion)
	bool osystem_takeFrozenSceneDrawn();
	// True while the scene snapshot follows the game (not frozen by a menu)
	bool osystem_isSceneSnapshotLive();

	// Brightness of the final image, 0 (black) to 1 (normal). Fades use it so
	// the whole picture (background, models, masks) changes together.
	bool osystem_canFadeScreen();
	// Sets the screen fade (0 black, 1 normal)
	void osystem_setScreenFade(float level);
	// The screen fade now (0 black, 1 normal)
	float osystem_getScreenFade();
	// The cigar smoke's ellipse on the 320x200 screen for this frame: the image
	// turns grey inside it (core/sceneDraw.cpp, AffSmoke)
	void osystem_setSmokeArea(float centerX, float centerY, float radiusX, float radiusY);

	// Number of the 3D object being drawn (1..255, 0 = none): its masks cover
	// only its own pixels. See rendererBGFX.cpp.
	void osystem_setObjectStencil(int objectId);




#endif
