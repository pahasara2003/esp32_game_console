#!/usr/bin/env python3
"""Listen to source songs and save independent start/end points for each shared track."""
import argparse
import hashlib
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
import json
from pathlib import Path
import re
import secrets
import webbrowser

from prepare_game_music import GAMES, SELECTIONS, MUSIC_BUDGET, duration, validate_selection, estimate_bytes

PAGE = Path(__file__).with_name('game_music_selector.html').read_text()


def load_tracks(folder, selections):
    groups = {}
    for game in GAMES:
        path = folder / (game + '.mp3')
        digest = hashlib.sha256(path.read_bytes()).hexdigest()
        if digest not in groups:
            seconds = duration(path)
            groups[digest] = {'hash': digest, 'path': path, 'games': [],
                              'duration': seconds, **validate_selection(selections.get(digest, 0), seconds)}
        groups[digest]['games'].append(game.replace('_', ' ').capitalize())
    return list(groups.values())


def handler(tracks, selection_path, token):
    class Handler(BaseHTTPRequestHandler):
        def allowed_host(self):
            return self.headers.get('Host') == f'127.0.0.1:{self.server.server_port}'

        def reply(self, code, data, kind='application/json'):
            self.send_response(code)
            self.send_header('Content-Type', kind)
            self.send_header('Content-Length', str(len(data)))
            self.send_header('Cache-Control', 'no-store')
            self.end_headers()
            self.wfile.write(data)

        def do_GET(self):
            if not self.allowed_host():
                self.reply(403, b'{}'); return
            if self.path == '/':
                self.reply(200, PAGE.replace('TOKEN', json.dumps(token)).encode(), 'text/html; charset=utf-8')
            elif self.path == '/tracks':
                public = [{k: v for k, v in t.items() if k != 'path'} for t in tracks]
                self.reply(200, json.dumps({'tracks': public, 'budget': MUSIC_BUDGET}).encode())
            elif re.fullmatch(r'/audio/\d+', self.path):
                index = int(self.path.rsplit('/', 1)[1])
                if index >= len(tracks):
                    self.reply(404, b'{}'); return
                try:
                    with tracks[index]['path'].open('rb') as source:
                        size = tracks[index]['path'].stat().st_size
                        start, end = 0, size - 1
                        requested = self.headers.get('Range')
                        if requested:
                            match = re.fullmatch(r'bytes=(\d+)-(\d*)', requested)
                            if not match:
                                self.reply(416, b'{}'); return
                            start = int(match[1]); end = min(int(match[2]) if match[2] else end, end)
                            if start > end:
                                self.reply(416, b'{}'); return
                        self.send_response(206 if requested else 200)
                        self.send_header('Content-Type', 'audio/mpeg')
                        self.send_header('Accept-Ranges', 'bytes')
                        self.send_header('Content-Length', str(end - start + 1))
                        if requested:
                            self.send_header('Content-Range', f'bytes {start}-{end}/{size}')
                        self.end_headers(); source.seek(start)
                        remaining = end - start + 1
                        while remaining:
                            block = source.read(min(65536, remaining))
                            if not block: break
                            self.wfile.write(block); remaining -= len(block)
                except (BrokenPipeError, ConnectionResetError):
                    pass  # Seeking cancels the previous browser request.
                except OSError:
                    self.close_connection = True
            else:
                self.reply(404, b'{}')

        def do_POST(self):
            if not self.allowed_host() or self.path != '/save' or self.headers.get('X-Token') != token:
                self.reply(403, b'{}'); return
            try:
                length = int(self.headers.get('Content-Length', '0'))
                if not 0 < length <= 16384:
                    raise ValueError('Invalid request size')
                choices = json.loads(self.rfile.read(length))
                if not isinstance(choices, dict) or set(choices) != {t['hash'] for t in tracks}:
                    raise ValueError('Selections must include every song')
                choices = {t['hash']: validate_selection(choices[t['hash']], t['duration']) for t in tracks}
                if estimate_bytes(choices.values()) > MUSIC_BUDGET:
                    raise ValueError('Selections exceed available flash. Shorten some loops.')
                temporary = selection_path.with_suffix('.tmp')
                temporary.write_text(json.dumps(choices, indent=2) + '\n')
                temporary.replace(selection_path)
                for track in tracks: track.update(choices[track['hash']])
                self.reply(200, b'{"saved":true}')
            except (ValueError, OSError) as error:
                self.reply(400, json.dumps({'error': str(error)}).encode())

        def log_message(self, *args):
            pass
    return Handler


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('folder', type=Path, help='SD card GameMusic folder')
    args = parser.parse_args()
    choices = json.loads(SELECTIONS.read_text()) if SELECTIONS.exists() else {}
    tracks = load_tracks(args.folder, choices)
    server = ThreadingHTTPServer(('127.0.0.1', 0), handler(tracks, SELECTIONS, secrets.token_urlsafe(24)))
    url = f'http://127.0.0.1:{server.server_port}/'
    print(f'Loop selector: {url}', flush=True)
    webbrowser.open(url)
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass
    finally:
        server.server_close()


if __name__ == '__main__':
    main()
