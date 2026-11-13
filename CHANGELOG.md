# Changelog

Time Gate: Classic Reborn shares its engine with [Alone in the Dark: Classic Reborn](https://github.com/nayo186/AloneInTheDarkClassicReborn), built on [FITD](https://github.com/yaz0r/FITD) by yaz0r. The history of the engine is in those repositories; this one starts with the first release of *Time Gate*. Each fix is explained, with its cause, in `docs/baseline/BUGS.md` (letter G).

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
