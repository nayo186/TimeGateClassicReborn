#!/usr/bin/env python3
# Makes a language pack for Time Gate (TatouSource/Shared/languageFolder.h):
# one .language file with a language folder's files gathered one after the
# other without compression, so the game reads each one straight from it.
#   - The data files (FRANCAIS.PAK, SAMPLES.PAK, LISTLIFE.PAK, ITD_RESS.PAK...)
#     as they are; the edition's read-me files (.BAT, .DOC, .WRI, .TXT) are
#     left out.
#   - The videos (CD/TIMEGATE/ANn.MUX): when a video's pictures and the length
#     of its sound are those of the installed game's video, only its voice goes
#     in, as cd/TIMEGATE/ANn.ogg (the game shows the installed video with it);
#     otherwise the whole video, as cd/TIMEGATE/ANn.MUX.
# Needs ffmpeg for the voices. The pack goes in UserData\TimeGate\Languages,
# named as the folder would be (Spanish.language).
#
# Usage: python tools/makeTimeGateLanguagePack.py <language folder> <installed game folder> <pack.language>
#   e.g. python tools/makeTimeGateLanguagePack.py Languages/Spanish "C:/GOG Games/Time Gate" Spanish.language
import hashlib
import os
import struct
import subprocess
import sys
import tempfile
import zipfile

READ_ME = ('.BAT', '.DOC', '.WRI', '.TXT')


def parse_video(path):
    """Pictures and palettes (a fingerprint), the sound (unsigned 8-bit) and its frequency."""
    data = open(path, 'rb').read()
    if len(data) < 32 or struct.unpack_from('<I', data, 0)[0] != 0xAA07:
        return None
    header = struct.unpack_from('<H', data, 4)[0]
    frequency = struct.unpack_from('<H', data, 14)[0] or 11111
    pictures = hashlib.md5()
    sound = bytearray()
    pos = header
    while pos + 6 <= len(data):
        kind = data[pos]
        size = struct.unpack_from('<I', data, pos + 2)[0]
        if pos + 6 + size > len(data) or kind == 4:
            break
        chunk = data[pos + 6:pos + 6 + size]
        if kind in (6, 7):
            sound += chunk
        elif kind in (0x64, 0x6F, 0x65, 0x70, 5):
            pictures.update(bytes([kind]))
            pictures.update(chunk)
        pos += 6 + size
    return pictures.hexdigest(), bytes(sound), frequency


def find(folder, *parts):
    """A path inside folder whatever the case of its names; None if not there."""
    for part in parts:
        if not os.path.isdir(folder):
            return None
        names = {n.upper(): n for n in os.listdir(folder)}
        if part.upper() not in names:
            return None
        folder = os.path.join(folder, names[part.upper()])
    return folder


def main():
    if len(sys.argv) != 4:
        sys.exit('usage: makeTimeGateLanguagePack.py <language folder> <installed game folder> <pack.language>')
    folder, installed, output = sys.argv[1:4]
    names = [n for n in os.listdir(folder) if os.path.isfile(os.path.join(folder, n))]
    for needed in ('FRANCAIS.PAK', 'SAMPLES.PAK', 'LISTLIFE.PAK'):
        if needed not in (n.upper() for n in names):
            sys.exit(f'no {needed} in {folder}: the game would not use this language')
    videos = find(folder, 'CD', 'TIMEGATE')
    base_videos = find(installed, 'CD', 'TIMEGATE')
    voices = whole = 0
    with zipfile.ZipFile(output, 'w', zipfile.ZIP_STORED, allowZip64=False) as pack, tempfile.TemporaryDirectory() as tmp:
        for name in sorted(names):
            if name.upper().endswith(READ_ME):
                continue
            pack.write(os.path.join(folder, name), name.upper())
        for name in sorted(os.listdir(videos)) if videos else []:
            if not name.upper().endswith('.MUX'):
                continue
            path = os.path.join(videos, name)
            stem = os.path.splitext(name)[0].upper()
            mine = parse_video(path)
            base_path = find(base_videos, name) if base_videos else None
            base = parse_video(base_path) if base_path else None
            if mine and base and mine[0] == base[0] and len(mine[1]) == len(base[1]) and mine[2] == base[2]:
                raw = os.path.join(tmp, stem + '.raw')
                ogg = os.path.join(tmp, stem + '.ogg')
                open(raw, 'wb').write(mine[1])
                subprocess.run(['ffmpeg', '-v', 'error', '-y', '-f', 'u8', '-ar', str(mine[2]), '-ac', '1', '-i', raw,
                                '-c:a', 'libvorbis', '-q:a', '5', ogg], check=True)
                pack.write(ogg, f'cd/TIMEGATE/{stem}.ogg')
                voices += 1
                print(f'  {stem}: only its voice ({os.path.getsize(path) / 1e6:.1f} MB -> {os.path.getsize(ogg) / 1e3:.0f} KB)')
            else:
                pack.write(path, f'cd/TIMEGATE/{stem}.MUX')
                whole += 1
                print(f'  {stem}: the whole video (its pictures are its own, {os.path.getsize(path) / 1e6:.1f} MB)')
    print(f'{output}: {voices} video voices, {whole} whole videos, {os.path.getsize(output) / 1e6:.1f} MB')


if __name__ == '__main__':
    main()
