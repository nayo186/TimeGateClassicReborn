# TIMEGATE.EXE

What is known of Time Gate: Knight's Chase's executable (GOG edition, DOS/4GW, Watcom LE). Addresses are those of the program loaded in memory: code from 0x10000, data from 0x60000 (in DOSBox-X: data linear = address − 0x60000 + 0x21E000, code linear = address − 0x10000 + 0x1D5000).

All the addresses here are those of the **Spanish** GOG edition (`TIMEGATE.EXE` 697561 bytes, CRC 0xB22E46D3). The English one (CRC 0xCC1C1B8D) has the same size but its code and data are shifted by its texts. The two editions differ only in `FRANCAIS.PAK` (the texts, so named in every language), `SAMPLES.PAK` (voices), `LISTLIFE.PAK` (11 scripts: voice and text numbers), `ITD_RESS.PAK` (entries 2 and 44, the fonts: the Spanish one has accented letters), `PRIORITY.ITD`, `TEST.WAV`, the executable and 8 videos (AN0-AN3, AN12, AN20, AN28, AN43); the other 1820 files are the same (Shared/knownEditions.h). The German one also changes `DEFINES.ITD` (byte 1) and `VARS.ITD` (four starting values at 0x166), has its own `FRANCAIS.PAK`, `SAMPLES.PAK`, `LISTLIFE.PAK` (same 1158 scripts), `PRIORITY.ITD` (490 bytes, all zero: only AITD3 uses priorities here), executable and the videos AN0, AN12, AN20, AN28 and AN43, and keeps the English `ITD_RESS.PAK` (its fonts have ä, ö, ü, Ä, Ö, Ü and ß). The Italian one has the Spanish executable and fonts (`TIMEGATE.EXE` 0xB22E46D3, `ITD_RESS.PAK` 0xCE46A555: the addresses here are also its own), its own `FRANCAIS.PAK`, `SAMPLES.PAK`, `LISTLIFE.PAK`, `DEFINES.ITD`, `PRIORITY.ITD` and `TEST.WAV`, and the same 8 videos with voice as the Spanish one. The French CD ("Time Gate : Le Secret du Templier", not sold by GOG; `TIMEGATE.EXE` 0x6870D800) is an older version: besides its language it changes `LISTANIM.PAK`, `OBJETS.ITD`, `CAMSAL09.PAK`, `CAMSAL15.PAK`, `MK050800.PAK`, `DEFINES.ITD`, `VARS.ITD`, and has 17 light files `LMnnnnnn.GRD` that GOG's editions do not have.

## Main menu (19F80)

- Background `ITD_RESS` 1 (the seal, with the eight coins already drawn dark).
- The coins are boxes of 51 × 46 (hit test at 19E40). Arrows: previous / next coin; Enter or a click chooses it. 19B00 draws the chosen coin lit over the seal.
- Actions (jump table 0x19F54): 0 new game (1A34D), 1 save (1A3D6, calls 1F750), 2 load (1A453, calls 1FE10), 3 sound setup (1A4DF), 4 volume (1A56C), 5 intro video (1A58A), 6 difficulty, the tarot cards (1A5CF), 7 quit (1A5FF).
- Lit pictures (`ITD_RESS`): 102, 111, 93, 113, 115, 109, 95, 107. Each coin also has a dark one (the one before or after). 96 to 101 are the tarot cards (VIII, X, XV, each plain and coloured).
- Other pictures: 3 load screen, 4 save screen, 5 stained-glass inventory, 116 tarot, 117 volume, 119 museum plaque, 123 sound setup.
- Sprites keep the width (padded) and the height in two bytes each (BUGS.md G2).

## Saves

File name `SAVE<n>.ITD` (the code takes the string "SQSAVE" from its third letter, 0x60A4C, then the number and ".ITD" at 0x60A54; built in 1F882 / 1FF4A / 20A16). GOG keeps them in `cloud_saves` (DOSBox overlay mount). There are 8 slots, 0 to 7: the list (209D0) skips any other number. The save is written by 1FDA0 after 1E650 (which lets every object store its state).

The file starts with a table of 9 offsets (0x24 bytes); each block is written where the running offset is, and its offset goes into its slot:

| Slot | Written by | Contents |
| --- | --- | --- |
| 0x00 | 1F9A0 | 0x4180 bytes: thumbnail 160 × 100 (0x3E80) + palette (0x300), from `0x8ACC4` |
| 0x0C | 1F9F0 | difficulty: 3 words at 0x8B1E6 (size from the table, 6) |
| — | 1FCB0 | the blocks of the table at 0x61858, one after another, without a slot (see below) |
| 0x10 | 1FCB0 | word count at 0x8AE04, then that many bytes from `[0x8ACDC]` |
| 0x14 | 1FCB0 | 0x2774 bytes from `[0x8AE78]` |
| 0x18 | 1FA40 | 8 entries: 4 bytes at 0x83A4C + 21·i + 4, 1 byte at + 0x14 |
| 0x1C | 1FAD0 | 4 bytes at 0x8B344, 1 byte at 0x8B358 |
| 0x20 | 1FB30 | 2 bytes at 0x61770: whether the inventory can be opened (statusScreenAllowed; checked with Enter or I at 282F8) |

Slots 0x04 and 0x08 are not filled by 1FDA0 (still to be checked).

The table at 0x61858 (pointer, size), written from entry 1 until a null pointer (1F910 gives entries 7 to 10 itself):

| # | Address | Size | Notes |
| --- | --- | --- | --- |
| 1 | 0x8B33E | 2 | |
| 2 | 0x8B33C | 2 | |
| 3 | 0x8B338 | 2 | |
| 4 | 0x6176C | 2 | |
| 5 | 0x6176E | 2 | |
| 6 | 0x8B33A | 2 | number of objects looped over by 1E650 |
| 7 | `[0x8AE84]` + 4 | 2 | count A |
| 8 | `[0x8AE84]` + 6 | 2 | count B |
| 9 | `[[0x8AE84]` + 8] | A × 0x36 | list of 54-byte entries |
| 10 | `[[0x8AE84]` + 0xC] | B × 0x36 | list of 54-byte entries |
| 11 | 0x8B23E | 0xA6 | |
| 12 | 0x8B32E | 2 | |
| 13 | 0x8B334 | 2 | |
| 14 | 0x8AE14 | 0x64 | |
| 15 | 0x61772 | 2 | |
| 16 | 0x61774 | 2 | |
| 17 | 0x8B32A | 2 | |
| 18 | 0x71784 | 4 | the clock (timer) |
| 19 | 0x7178C | 4 | the frame counter (inc each frame); equal to the clock in the saves |
| 20 | 0x84A04 | 0x5208 | |

Still to do: name each variable (floor, room, camera, vars, objects...), the loader (202B0) and the list of saves (209D0 / 20390), and compare with a save of the original.

## Difficulty, the tarot cards (16B00)

Coin 6 of both coin menus (the start one, 29CE9, and the one in the game, 1A5CF) calls 16B00 and puts what it returns in the three words of the difficulty, 0x8B1E6, 0x8B1E8 and 0x8B1EA (always the same value; they are the first block of a save). The program starts them at 1 (297FA), not each new game; loading a save takes the save's.

- Screen: background `ITD_RESS` 116 (stained-glass frame with the three cards in grey). The chosen card is drawn over it in colour (16980): 0 VIII, the angel (easy), at x 41; 1 X, angel and demon (normal), at x 122; 2 XV, the demon (hard), at x 206; y 37, 72 x 121. Pictures: VIII 96 grey / 97 colour, X 98 / 99, XV 100 / 101.
- Left and right change the card (they wrap round); a click on a card (16A50, boxes of 72 x 121) chooses it; Enter, the mouse button and Esc all keep the card that is lit (there is no cancel).
- Effect, word 0 (0x8B1E6): the force of the scripts' HIT (opcode 0x10, 22950) and FIRE (0x33, 229E0) is multiplied by difficulty + 1 (x1, x2, x3). FIRE_UP_DOWN (0x63, 22A80) is not.
- Effect, words 1 and 2: when a hit or a projectile hands its force to the one hit (22F8B, 23289, 234DC, 23899): on the hero (0x8B258) word 2, on anyone else word 1; 0 takes 2 off, 2 adds 3, never under 1. The same rule as AITD3 (core/AITD3.cpp, difficultyHitForce).

## Sound setup (1A4DF, 32970)

Coin 3: the HMI sound card setup of 1996. It waits for the keys to be released, stops the sounds (30B30) and the music (30950), then 32970 shows lists over a plain stone background (`ITD_RESS` 123): the DIGITAL device (No Sound, Sound Blaster, Sound Blaster Pro, 16, AWE32, Ensoniq SoundScape, Microsoft Sound System, Gravis UltraSound, UltraSound Max, Roland RAP-10, Adlib Gold, Sound Galaxy NX Pro 16...) and the MIDI one, with port, DMA and IRQ, written to `HMISET.CFG` (`[DIGITAL]`, `[MIDI]`). Afterwards 34A10 / 34AB0 tell whether sound and music work (0x6184A, 0x6184B) and the music starts again. MUSIC.PAK 0 and 1 are the AdLib instrument banks for its FM drivers; the songs (2 on) are HMI MIDI.
