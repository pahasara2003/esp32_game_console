"""Prepare the three selected CC0 clips for the flash-resident IMA decoder."""
from pathlib import Path
import struct
import subprocess

root = Path(__file__).resolve().parents[1] / 'assets/sounds/endings'
for name in ('game_over', 'lose', 'win'):
    pcm = subprocess.check_output([
        'ffmpeg', '-v', 'error', '-i', str(root / (name + '.wav')),
        '-af', 'loudnorm=I=-18:TP=-6:LRA=11', '-ar', '22050', '-ac', '1',
        '-f', 's16le', '-'])
    output = root / (name + '_embedded.wav')
    subprocess.run([
        'ffmpeg', '-y', '-v', 'error', '-f', 's16le', '-ar', '22050', '-ac', '1',
        '-i', 'pipe:0', '-c:a', 'adpcm_ima_wav', '-block_size', '1024', str(output)],
        input=pcm, check=True)
    data = bytearray(output.read_bytes())
    # Keep the true length rather than the encoder's padded block length.
    fact = data.index(b'fact')
    chunk = data.index(b'data')
    capacity = struct.unpack_from('<I', data, chunk + 4)[0] // 1024 * 2041
    assert len(pcm) % 2 == 0 and len(pcm) // 2 <= capacity
    struct.pack_into('<I', data, fact + 8, len(pcm) // 2)
    output.write_bytes(data)
    print(f'{name}: {len(pcm) / 44100:.2f}s, {len(data)} bytes')
