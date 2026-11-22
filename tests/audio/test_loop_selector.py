"""Check seeking, saving, shared sources and loop-start validation."""
from pathlib import Path
from array import array
import hashlib
import json
import math
import sys
import subprocess
import tempfile
import threading
from urllib.error import HTTPError
from urllib.request import Request, urlopen

root = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(root / 'tools'))
from prepare_game_music import validate_start, validate_selection, estimate_bytes, MUSIC_BUDGET
from select_game_music import ThreadingHTTPServer, handler
import prepare_game_music as converter

assert validate_start(0, 10) == 0
assert validate_start(60, 90) == 60
for value in [-1, 60.1, True, '5', math.nan, math.inf]:
    try:
        validate_start(value, 90)
    except ValueError:
        pass
    else:
        raise AssertionError(value)
assert validate_selection(15.5, 90) == {'start': 15.5, 'end': 45.5}
assert validate_selection({'start': 5, 'end': 80}, 90) == {'start': 5, 'end': 80}
for choice in [{'start': 5, 'end': 5}, {'start': -1, 'end': 30},
               {'start': 10, 'end': 91}, {'start': 0, 'end': math.inf},
               {'start': True, 'end': 30}, {'start': 0}]:
    try: validate_selection(choice, 90)
    except ValueError: pass
    else: raise AssertionError(choice)
assert estimate_bytes([{'start': 0, 'end': 400}]) > MUSIC_BUDGET

with tempfile.TemporaryDirectory() as directory:
    work = Path(directory)
    song = work / 'song.mp3'
    song.write_bytes(bytes(range(256)) * 1000)
    saved = work / 'selection.json'
    tracks = [{'hash': 'abc', 'path': song, 'games': ['Snake', 'Tic tac toe'],
               'duration': 90, 'start': 0, 'end': 30}]
    server = ThreadingHTTPServer(('127.0.0.1', 0), handler(tracks, saved, 'test-token'))
    worker = threading.Thread(target=server.serve_forever)
    worker.start()
    base = f'http://127.0.0.1:{server.server_port}'
    try:
        with urlopen(base + '/tracks') as response:
            result = json.load(response)
            assert result['budget'] == MUSIC_BUDGET
            data = result['tracks']
            assert len(data) == 1 and len(data[0]['games']) == 2 and 'path' not in data[0]
        with urlopen(Request(base + '/audio/0', headers={'Range': 'bytes=123-456'})) as response:
            assert response.status == 206
            assert response.headers['Content-Range'] == f'bytes 123-456/{song.stat().st_size}'
            assert response.read() == song.read_bytes()[123:457]
        for path, headers, expected in [('/audio/9', {}, 404),
                                        ('/audio/0', {'Range': 'bytes=999999-'}, 416),
                                        ('/tracks', {'Host': 'foreign.example'}, 403)]:
            try:
                urlopen(Request(base + path, headers=headers))
            except HTTPError as error:
                assert error.code == expected
            else:
                raise AssertionError(path)
        for choices, token, expected in [({'abc': 15.5}, 'test-token', 200),
                                          ({'abc': {'start': 5, 'end': 80}}, 'test-token', 200),
                                          ({'abc': {'start': 10, 'end': 9}}, 'test-token', 400),
                                          ({'abc': 61}, 'test-token', 400),
                                          ({'abc': 0}, 'wrong-token', 403),
                                          ({}, 'test-token', 400)]:
            request = Request(base + '/save', data=json.dumps(choices).encode(),
                              headers={'Content-Type': 'application/json', 'X-Token': token})
            try:
                with urlopen(request) as response: assert response.status == expected
            except HTTPError as error:
                assert error.code == expected
        assert json.loads(saved.read_text()) == {'abc': {'start': 5, 'end': 80}}
        assert tracks[0]['start'] == 5 and tracks[0]['end'] == 80
        tracks[0]['duration'] = 500
        request = Request(base + '/save', data=json.dumps({'abc': {'start': 0, 'end': 400}}).encode(),
                          headers={'Content-Type': 'application/json', 'X-Token': 'test-token'})
        try: urlopen(request)
        except HTTPError as error: assert error.code == 400
        else: raise AssertionError('Over-budget save was accepted')
        assert json.loads(saved.read_text()) == {'abc': {'start': 5, 'end': 80}}
    finally:
        server.shutdown(); worker.join(); server.server_close()

    # Conversion must use the chosen section, not the beginning of the song.
    (work / 'assets').mkdir(); (work / 'src').mkdir()
    original = work / 'tetris.mp3'
    subprocess.run(['ffmpeg', '-v', 'error', '-f', 'lavfi', '-i',
                    'sine=frequency=220:sample_rate=22050:duration=20',
                    '-f', 'lavfi', '-i', 'sine=frequency=880:sample_rate=22050:duration=40',
                    '-filter_complex', '[0:a][1:a]concat=n=2:v=0:a=1',
                    '-c:a', 'libmp3lame', '-b:a', '32k', str(original)], check=True)
    digest = hashlib.sha256(original.read_bytes()).hexdigest()
    saved.write_text(json.dumps({digest: {'start': 25, 'end': 37}}))
    converter.ROOT, converter.SELECTIONS, converter.GAMES = work, saved, ['tetris']
    sys.argv = ['prepare_game_music.py', str(work)]
    converter.main()
    manifest = json.loads((work / 'assets/game_music.json').read_text())
    assert manifest['tracks'][0]['start'] == 25
    assert manifest['tracks'][0]['end'] == 37
    assert 12 <= manifest['tracks'][0]['duration'] < 12.2
    pcm = array('h', subprocess.check_output([
        'ffmpeg', '-v', 'error', '-i', str(work / 'assets/game_music.bin'),
        '-f', 's16le', '-ac', '1', '-ar', '22050', '-']))
    segment = pcm[11025:33075]
    crossings = sum((a < 0) != (b < 0) for a, b in zip(segment, segment[1:]))
    assert 870 < crossings / 2 < 890, crossings / 2
print('PASS: browser seeking, shared song list, saved starts and invalid request handling')
