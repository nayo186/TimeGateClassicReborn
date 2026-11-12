# Changelog

Time Gate: Classic Reborn shares its engine with [Alone in the Dark: Classic Reborn](https://github.com/nayo186/AloneInTheDarkClassicReborn), which started from [Alone in the Dark: Re-Haunted](https://github.com/spacefarergames/AloneInTheDarkReHaunted) by Spacefarer Retro Remasters and [FITD](https://github.com/yaz0r/FITD) by yaz0r. The history of the engine is in those repositories; this one starts with the first release of *Time Gate*. Each fix is explained, with its cause, in `docs/baseline/BUGS.md` (letter G).

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

- Enemies that chase you go straight towards you; the original finds a way around obstacles, so one may get stuck behind a column or a piece of furniture (G42).
- There is no Android version yet.
