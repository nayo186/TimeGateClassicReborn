# Modding guide

This guide is for anyone who wants to change how *Time Gate: Knight's Chase* looks or sounds: new backgrounds, redrawn scenes, remade videos, other music. Everything you add goes into the **`UserData\TimeGate`** folder, next to `Launcher.exe`. The game files are never changed: to go back to the original, remove your files.

## What you can change

| What | Where it goes | Section |
| --- | --- | --- |
| Backgrounds (the painted scenes behind the characters), and the second picture of some cameras | `UserData\TimeGate\Backgrounds` | [Backgrounds](#backgrounds) |
| Full-screen pictures (menu of coins, save wall, inventory, tarot...) | `UserData\TimeGate\Backgrounds` | [Full-screen pictures](#full-screen-pictures) |
| Masks (the parts of a background that pass in front of the characters) | `UserData\TimeGate\Masks` | [Masks](#masks) |
| Videos, picture by picture, and their sound | `UserData\TimeGate\Backgrounds\sequences` | [Videos](#videos) |
| Music (the soundfont that plays it) | `UserData\TimeGate\SoundFonts` | [Music](#music) |
| Languages | `UserData\TimeGate\Languages` | [Languages](#languages) |

**Not yet:** the 3D models and their textures, the sound effects and the texts.

## Where your files go

```
UserData/
└── TimeGate/
    ├── Backgrounds/             backgrounds and full-screen pictures (PNG or JPG)
    │   └── sequences/AN<n>/     videos: pictures and sound
    ├── Masks/                   masks (PNG)
    ├── Exports/                 what F1 exports: the originals, to use as a base
    ├── SoundFonts/              soundfonts for the music (.sf2)
    └── Languages/<language>/    other languages
```

The launcher creates these folders the first time it opens. A mod is shared as these same folders: whoever uses it copies them into their own `UserData` (see [Sharing a mod](#sharing-a-mod)).

## Before you start

1. Switch on **Custom backgrounds** in **F1** (or in the launcher's **Options**). Backgrounds, full-screen pictures, masks and videos are only used while it is on.
2. In F1, use **Export backgrounds**, **Export masks** and **Export sequences** (the videos). They write the game's own pictures, at their original 320×200, to `UserData\TimeGate\Exports`: they are your starting point, and their names are the names your files must have.
3. Keep F1's export running until it says it is done: it goes through every floor of the game.

While the game runs, a picture you save is shown at once, no restart needed. `tatou.log` (in `UserData\TimeGate`) names each of your pictures when it is used, for example `Custom background CAMERA00_000.png (2880x2160, ...)`, and warns about any it cannot use.

## Backgrounds

**Name:** `CAMERAnn_iii`, as exported: `nn` is the floor (the game's `CAMERAnn.PAK` file) and `iii` the picture in it, one per camera. Example: `CAMERA00_000.png`.

**Second pictures:** some cameras change their picture later in the game, for example the museum room once the alarm beams are switched off. Those pictures are `CYMERAnn_iii`, after the game's `CYMERAnn.PAK` files, with the same numbers as the camera they replace: `CYMERA01_001` is the second picture of `CAMERA01_001`. **Export backgrounds** writes only the cameras that have one, and they are replaced the same way.

**Format:** PNG or JPG (`.png`, `.jpg`, `.jpeg`). JPG is much lighter for big pictures.

**Size:** any, as long as it is not narrower than 4:3:

- **16:10**, the shape of the original 320×200 (1280×800, 2560×1600...), or **4:3** (1600×1200...): the whole picture is the play area;
- **wider than 4:3** (16:9...): its 4:3 centre is the play area and the sides fill the black bars in fullscreen.

A picture narrower than 4:3 is not used (`tatou.log` says so).

**Example:** to repaint the first room of the museum, take `Exports\CAMERA00_000.png`, enlarge or repaint it, and save it as `UserData\TimeGate\Backgrounds\CAMERA00_000.png` (or `.jpg`).

**Keep the layout.** The characters walk on the floor of the original picture, and the objects stand where they stood: your picture must keep the walls, floor and furniture in the same places, or the characters will float or walk through things.

## Full-screen pictures

The pictures that fill the screen (the menu of coins, the save and load wall, the inventory's stained glass, the volume screen, the tarot of the difficulty...) are named `ITD_RESS_nnn`, after the game's `ITD_RESS.PAK` file and their number in it. F1's **Export backgrounds** writes them with the backgrounds. They follow the same rules: same name in `Backgrounds`, PNG or JPG, any size from 4:3.

## Masks

A mask says which part of a background passes in front of the characters: a column, a railing, a door frame. The game keeps the masks of each camera in its own file (`MKnncc00.PAK`, and `NKnncc00.PAK` for the second pictures); when your background moves or redraws such an object, its mask must follow it.

**Name:** `CAMERAnn_iii_mask_r_m.png` (or `CYMERAnn_iii_mask_r_m.png` for a second picture), as exported: the background it belongs to, then two numbers the game gives each mask (the room seen from that camera and the mask in that room). Use the exported names as they are.

**What you get from the export:** in `Exports\masks`, each mask as a black and white picture; in `Exports\masks_guides`, the same mask over its background, to see which part it is.

**How it works:**

- **White** is the part that covers the characters; black lets them be seen. The edge is where the grey is halfway, and the game smooths it by one pixel of the screen.
- **PNG only** (a mask's edges must be exact; JPG would blur them).
- **Size:** the shape of the original (16:10, like 320×200, e.g. 1280×800) places it over the scene as the original mask; the shape of your background (e.g. the same 16:9 size as your picture) places it over your background. The simplest is to make it the same size as your background.

**Example:** your new `CAMERA00_000.png` moved a column. Look in `Exports\masks_guides` among the masks whose name starts with `CAMERA00_000_mask_` for the one of that column, take the same file from `Exports\masks`, paint the column white where it now is (and black where it no longer is) at your background's size, and save it with the same name in `UserData\TimeGate\Masks`.

Masks only matter when your picture moves or redraws an object in front of the characters: with the same layout, the game's own masks keep working.

## Videos

The intro and the cut scenes of *Time Gate* are videos (the `AN<n>.MUX` files of the game's `cd/TIMEGATE` folder). Each one can be redrawn picture by picture, and its sound replaced.

**Export:** F1's **Export sequences** writes each video to `Exports\sequences\AN<n>\`: one picture per frame (`000.png`, `001.png`...) and its sound as `sound.wav`.

**Replace the pictures:** save your frame with the same name in `Backgrounds\sequences\AN<n>\` (PNG or JPG, any size from 4:3). Example: `UserData\TimeGate\Backgrounds\sequences\AN3\005.png` replaces the sixth frame of video `AN3`. Frames you do not replace keep the original, so you can redraw only some of them.

**Replace the sound:** put a file named `sound` in the same folder, as `.wav`, `.ogg`, `.mp3` or `.flac`: `Backgrounds\sequences\AN3\sound.ogg`. It plays instead of the video's own sound, from the start of the video; without it, the original sound plays. The pictures keep the pace and the length of the original video, so make your sound as long as the exported `sound.wav`. `tatou.log` says when a video uses your sound.

The videos with voices are in the language of your game, so the exported sound is the one you play with. A sound of yours plays in every language.

## Music

*Time Gate*'s music is MIDI, played with a General MIDI soundfont (`.sf2`). Put any soundfont in `UserData\TimeGate\SoundFonts` and choose it in the game's menu (**Esc**, the **Sound** coin): see [Music](README.md#music) in the README.

## Languages

The game can be played in another language you own from a folder of `UserData\TimeGate\Languages`, with the texts, voices and videos of that language: see [Languages](README.md#languages) in the README.

## Sharing a mod

- **Share only your own work:** pictures you painted or made, recordings you made. **Never** the files of the game, its pictures or videos exported as they are, or its voices: everyone needs their own copy of the game.
- **Share the folders as they go in `UserData`**, so a player can copy them as they are. For example, a pack of backgrounds:

  ```
  My HD backgrounds/
  └── TimeGate/
      ├── Backgrounds/   CAMERA00_000.jpg, CAMERA00_001.jpg...
      └── Masks/         CAMERA00_000_mask_0_1.png...
  ```

- **Say which edition** it is made for (GOG and its language, or the French CD): the backgrounds are the same in all of them, but the videos with voices are not, and a player will want to know.
- A short text with what it changes and how to remove it (delete the files) helps players.
