# Changelog

Time Gate: Classic Reborn shares its engine with [Alone in the Dark: Classic Reborn](https://github.com/nayo186/AloneInTheDarkClassicReborn), built on [FITD](https://github.com/yaz0r/FITD) by yaz0r. The history of the engine is in those repositories; this one starts with the first release of *Time Gate*. Each fix is explained, with its cause, in `docs/baseline/BUGS.md` (letter G).

## v1.0.2 (2026-11-22)

Improved backgrounds for every room, downloaded from the launcher, and a macOS fix.

**New**

- **Improved backgrounds**: every background and mask of the game, repainted at high resolution with the same colours, in one file the launcher downloads: **Options** → **Improved backgrounds** → **Download**; **Remove** deletes it (G69).
- **Texture packs**: backgrounds, pictures and masks can be a single `.texture` file in `UserData`, as in *Alone in the Dark: Classic Reborn*; a loose file still comes first. The modding guide explains how to make one (G69).

**macOS**

- The game could close as soon as it started: the game's thread moved the window, which macOS only allows from the main thread (G68; fixed in *Alone in the Dark: Classic Reborn* by iNdioNicarao).

## v1.0.1 (2026-11-21)

The new font, the widescreen view and languages in one file, from *Alone in the Dark: Classic Reborn*; lighter screen effects, and several Android fixes.

**New**

- With **Enhanced text**, a new smooth font in every text (captions, subtitles, inventory, menus, the newspaper), in every language; the German Ö shows again (G63).
- **Widescreen view**: **16:9 (zoomed)** and **16:10 (zoomed)** in **Aspect ratio** fill a 16:9 screen with no stretching and no black bars; the view follows the character, and **Look up / Look down** move it. Menus, the inventory and the newspaper stay whole (G64).
- **Languages in one file**: another language of the game can be a single `.language` file, with its texts, voices and dubbed videos; the modding guide explains how to make one (G65).
- A new size in **Resolution**, **800**, the size by default on Android (G66).

**Faster**

- **Edge smoothing (FXAA)** and **Ambient occlusion (SSAO)** work only around the 3D models, and the ambient occlusion at half size; with a zoomed aspect ratio the effects work only on the part of the scene seen on screen (G66).
- Custom masks are prepared together when a camera is shown, are not read again after a menu, and keep only the part that covers something: a shorter stop at a change of camera (G67).

**Android**

- Coming back to the game after a call or another app left the screen black, and with Vulkan the game closed: it now carries on where it was (G66).
- The game is always fullscreen; the **Fullscreen** option made the system bars appear over the touch buttons and is no longer shown (G66).
- The menus' **scroll bar** can be dragged with a finger, and a flicked list keeps going and slows down (G66).
- The main menu says **Exit** in the language of the game instead of "Return to Linux" (G66).

**Removed**

- The **Animation smoothing** option: the game already blends the poses in every frame, and the difference could hardly be seen (G66).

## v1.0.0 (2026-11-17)

The first full release: *Time Gate: Knight's Chase* has been played from start to end.

**The game**

- The fountain's water, with an enhanced background, looked like a grey patch you could see through: only its ripples are drawn now, over the background (G59).
- A character's voice went on over a save loaded from the menu: now it pauses while the menu is open, goes on when you go back to the game, and stops when you load a save, start a new game or quit (G60).
- The monks of the graveyard came out of their graves with their whole body showing: as in the original, every object is cut at the height of the ground, so they rise from it (G61).
- After putting the cross on the statue, the altar's lid opening was not seen: the picture of the cross stayed on screen during those seconds (G62).

## v0.1.0-alpha.5 (2026-11-17)

Three fixes found while playing.

**The game**

- An object you have put down can be taken again, as in the original: a book left on a table could not be taken back, and the action in front of the table did nothing (G57).
- After using an object from the inventory, its name stayed written over the video that follows (the sheepskin in the bell tower) (G56).
- The invisible zones of a room are read as the original reads them: a zone with nothing for an object no longer hides the zones after it (where you stand, the floor under your feet, the way to the next room) (G58).

## v0.1.0-alpha.4 (2026-11-17)

Two fixes found while playing.

**The game**

- The sheep of the farm charges again: once you had stood next to it, it went on "touching" you even after you walked away, and every charge was a blow at once, without running. An object that stands still now forgets what it touched, as the original does (G55).
- On the save and load screen, the up and down arrows turned the drum the opposite way to what you would expect: up now brings the next board and down the one before, and the mouse wheel does the same. Left and right are as before (G54).

## v0.1.0-alpha.3 (2026-11-13)

A choice of graphics engine, fixes on Android, and a guide for modders.

**Options**

- New **Graphics engine** option in the launcher (**Options**) and in F1: Direct3D 11 or Vulkan on Windows, Vulkan or OpenGL ES on Android. It applies the next time the game starts (C78).

**The game**

- Loading a game while blood or sparks were on screen lost for good part of the small block of memory that the game keeps for those effects. After many loads they could stop showing (C76).

**Android (experimental)**

- Bigger text in the launcher and F1 menus, easier to read on a phone (C77).
- A touch on the on-screen buttons (Start, A, B, the D-pad...) also moved the menus' pointer to where the button is, and could choose the wrong entry of a menu. Now a touch on them only presses the button (C79).

**Documents**

- A **modding guide** ([MODDING.md](MODDING.md)): how to export and replace the backgrounds, the second pictures of some cameras, the full-screen pictures, the masks and the videos (picture by picture, and their sound).
- The README has a picture of the launcher, the table of options and a Modding section.

## v0.1.0-alpha.2 (2026-11-13)

Time Gate on Android, enemies that find their way round obstacles, and fixes found while playing the prison after the journey back in time.

**The game**

- Enemies that chase you find their way round walls and furniture, as the original does: each floor's obstacles are read from the game's `PURSUIT.PAK` and the shortest way is searched through their corners. The knight of the prison corridor now keeps to his round instead of wandering into your cell, and the scripts that ask how far the way is left work again (G49).
- The jailer of the cells takes a blow from behind while he looks at the other prisoner, and if he goes away the prisoner bangs on the bars again (G50).
- The sound of a character's steps is only heard in your room, and quieter the farther it is, as in the original: no more endless steps from the guard of the next room (G48).
- The scripts that take your objects away (when you are imprisoned) now empty the inventory, as the original (G47).

**Languages**

- A language can also be the original CD as it came, an image (`.iso`, `.bin`, `.img`) in its folder of `Languages`: its texts, voices and videos are read from the image (G46).

**Android (experimental)**

- Time Gate: Classic Reborn for Android: its own app with its name and icon, the music's soundfont inside, import of the game folder with its languages, saves and soundfonts with a progress window (G44, G45; see the README).
- On the phone, the **Controller** tab of **Controls** is a list, easier to use with a finger than the picture (G53).
- With Vulkan, some places where a small animation is drawn on the background came out grey or striped (C75, shared with Alone in the Dark: Classic Reborn).

**For modders**

- **Export backgrounds** also writes the second pictures of the cameras (`CYMERAnn`) and the full-screen pictures (menu, save wall, inventory, volume, tarot, sound setup), and those screens can be replaced with your pictures. **Export masks** works with Time Gate (G51).
- **Export sequences** writes each video as numbered pictures and its sound (`sequences\ANn\000.png`..., `sound.wav`); pictures (PNG or JPG) and a sound (`sound.wav`, `.ogg`, `.mp3`, `.flac`) put in `Backgrounds\sequences\ANn` replace them (G52).

**Known limitations of this alpha**

- The rest of the game, after the prison, is still being played through.

## v0.1.0-alpha.1 (2026-11-11)

The first alpha: the first part of the game, in the museum, is playable up to the journey back in time.

**The game**

- *Time Gate*'s own engine parts, compared with `TIMEGATE.EXE`: textured models and their lighting, masks, MIDI music, its eight sound channels, the videos (`AN*.MUX`), the stained-glass inventory, the "you found an object" window, the documents and the newspaper, the menu of coins (G1–G15).
- Saves in the format of the GOG edition, in both directions (G4).
- Difficulty: the tarot coin chooses it, and fights use it as the original (G14).
- The sound coin chooses the soundfont of the music: the one that comes with the game (TimGM6mb) or any `.sf2` added to `UserData\TimeGate\SoundFonts` (G15).
- Every script order read as the original reads it, including `IF_IN`/`IF_OUT` and the music orders (G36, G37).
- Fixes found while playing the first part: the museum's board, street noise, invisible posts, the lasers' room (its beams, its hum, its alarms switched off with the remote catapult, the second picture of the room without the beams), the sword on the floor, the courtyard's well, the enemies that chase you, sounds that stopped or stayed, colours of the showcase, collisions by the wall plaque (G16–G43).

**Editions and languages**

- Known editions: GOG in English, Spanish, German and Italian, and the French CD (*Time Gate : Le Secret du Templier*); the launcher checks the files of your folder against them (G25, G28, G29, G31).
- Play in another language you own: put its files in `UserData\TimeGate\Languages\<language>` and choose it in **Game language** (G32).
- Texts measured as the original measures them, so the German newspaper fits (G27).

**The launcher**

- Its own screen with the cover of the game, **Play**, and windows for the options, the controls (with a picture of the controller) and the audio; the menus in English, French, German, Italian and Spanish.

**Known limitations of this alpha**

- Enemies that chase you go straight towards you; the original finds a way around obstacles, so one may get stuck behind a column or a piece of furniture (G42; found in alpha 2, G49).
- There is no Android version yet (added in alpha 2).
