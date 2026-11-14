#!/usr/bin/env python3
"""
ESP32-C3 Music Player - PC song controller.

Talks to the firmware over USB serial using its built-in commands:
    list, play <n>, next, prev, status, help

Two ways to use it:

1) Interactive menu (run with no arguments):
       python3 song_controller.py

       Then type a song number to play it, or n/p for next/prev.

2) One-shot commands (handy for scripts / hotkeys):
       python3 song_controller.py list
       python3 song_controller.py play 12
       python3 song_controller.py next
       python3 song_controller.py prev
       python3 song_controller.py status

Notes:
- Opening the serial port makes the ESP32-C3 reboot once (USB CDC). The
  script waits for it to come back before sending anything.
- The board is auto-detected; pass --port to override, e.g.
       python3 song_controller.py --port /dev/ttyACM0
"""

import argparse
import re
import sys
import time

try:
    import serial
    from serial.tools import list_ports
except ImportError:
    sys.exit("pyserial is required. Install it with:  python3 -m pip install pyserial")


BAUD = 115200
RE_LIST = re.compile(r"^\s*(\d+)\)\s+(.*?)(\s+<--\s+playing)?\s*$")
RE_NOW = re.compile(r"Now playing \[(\d+)/(\d+)\]:\s*(.*)$")


# --------------------------------------------------------------------------
# Port handling
# --------------------------------------------------------------------------
def autodetect_port():
    """Prefer the ESP32 USB-CDC port, then any ACM/USB serial port."""
    ports = list(list_ports.comports())
    for p in ports:
        if p.vid == 0x303A:            # Espressif VID
            return p.device
    for p in ports:
        if "ACM" in p.device or "USB" in p.device:
            return p.device
    for p in ports:
        return p.device
    return None


def open_port(port, baud=BAUD, wait_boot=True):
    ser = serial.Serial(port, baud, timeout=0.1, write_timeout=2)
    # Ask the board to not be held in reset (best effort).
    try:
        ser.dtr = False
        ser.rts = False
    except Exception:
        pass
    if wait_boot:
        # USB CDC re-enumerates/reboots when the port is opened.
        time.sleep(3.0)
        ser.reset_input_buffer()
    return ser


# --------------------------------------------------------------------------
# Serial conversation
# --------------------------------------------------------------------------
def read_response(ser, idle=0.4, max_wait=5.0):
    """Read until the device goes quiet for `idle` seconds."""
    buf = bytearray()
    start = time.time()
    last = time.time()
    while time.time() - start < max_wait:
        chunk = ser.read(512)
        if chunk:
            buf += chunk
            last = time.time()
        elif buf and (time.time() - last) > idle:
            break
        else:
            time.sleep(0.02)
    return buf.decode("utf-8", "replace")


def send(ser, command, idle=0.5, max_wait=6.0):
    """Send one line command and return the device's reply."""
    try:
        ser.reset_input_buffer()
    except Exception:
        pass
    ser.write((command.strip() + "\n").encode("utf-8"))
    ser.flush()
    return read_response(ser, idle=idle, max_wait=max_wait)


def parse_songs(text):
    """Turn the 'list' reply into {index: path}."""
    songs = {}
    for line in text.splitlines():
        m = RE_LIST.match(line)
        if m:
            songs[int(m.group(1))] = m.group(2).strip()
    return songs


def print_device(text):
    """Print device output, prefixed, skipping empty lines and I2S noise."""
    for line in text.splitlines():
        line = line.rstrip()
        if not line:
            continue
        if "register I2S object to platform failed" in line:
            continue      # harmless driver message on track switch
        print(f"  esp> {line}")


# --------------------------------------------------------------------------
# UI
# --------------------------------------------------------------------------
def pretty_name(path):
    """Show just the track name, without folder or downloader tags."""
    name = path.rsplit("/", 1)[-1]
    name = re.sub(r"^\[[^\]]*\]\s*", "", name)   # drop [SPOTDOWNLOADER.COM]
    if name.lower().endswith(".mp3"):
        name = name[:-4]
    return name


def show_menu(songs, current=None):
    if not songs:
        print("  (no song list received - is the SD card inserted?)")
        return
    names = {i: pretty_name(p) for i, p in songs.items()}
    width = max((len(n) for n in names.values()), default=0)
    for idx in sorted(songs):
        mark = "  <-- playing" if idx == current else ""
        print(f"  {idx:>2}) {names[idx]:<{width}}{mark}")


def interactive(ser):
    print("\n=== ESP32 Music Player ===")
    reply = send(ser, "list", idle=0.8, max_wait=6.0)
    songs = parse_songs(reply)
    show_menu(songs)
    reply2 = send(ser, "status", idle=0.4)
    current = None
    for line in reply2.splitlines():
        m = RE_NOW.match(line) or re.search(r"Current \[(\d+)/(\d+)\]", line)
        if m:
            current = int(m.group(1))

    print("\nCommands:")
    print("  <number>   play that song          n / next   next song")
    print("  p / prev   previous song           s / status show current song")
    print("  l / list   refresh list            q / quit   exit")

    while True:
        try:
            cmd = input("\nsong> ").strip()
        except (EOFError, KeyboardInterrupt):
            print("\nBye.")
            break

        if not cmd:
            continue
        low = cmd.lower()

        if low in ("q", "quit", "exit"):
            print("Bye.")
            break

        if low in ("l", "list", "r", "refresh"):
            reply = send(ser, "list", idle=0.8, max_wait=6.0)
            songs = parse_songs(reply)
            show_menu(songs, current)
            continue

        if low in ("h", "help", "?"):
            print("  <number> play | n next | p prev | s status | l list | q quit")
            continue

        if low in ("n", "next"):
            reply = send(ser, "next", idle=0.6, max_wait=8.0)
        elif low in ("p", "prev"):
            reply = send(ser, "prev", idle=0.6, max_wait=8.0)
        elif low in ("s", "status"):
            reply = send(ser, "status", idle=0.4)
        elif low.isdigit():
            if songs and int(low) not in songs:
                print(f"  No song number {low}. Valid range is 1..{max(songs)}.")
                continue
            reply = send(ser, f"play {low}", idle=0.6, max_wait=8.0)
        else:
            print("  Unknown command. Use a number, n, p, s, l, or q.")
            continue

        print_device(reply)
        m = RE_NOW.search(reply)
        if m:
            current = int(m.group(1))
            print(f"\n  Now playing #{current}: {m.group(3)}")


def one_shot(ser, command):
    reply = send(ser, command, idle=0.6, max_wait=8.0)
    print_device(reply)
    return 0


# --------------------------------------------------------------------------
def main():
    ap = argparse.ArgumentParser(
        description="Control the ESP32 music player over USB serial.")
    ap.add_argument("--port", help="serial port (auto-detected by default)")
    ap.add_argument("--baud", type=int, default=BAUD)
    ap.add_argument("command", nargs="?",
                    help="one-shot: list | play <n> | next | prev | status")
    ap.add_argument("arg", nargs="?")
    args = ap.parse_args()

    port = args.port or autodetect_port()
    if not port:
        sys.exit("No serial port found. Plug in the board or pass --port.")

    print(f"Connecting to {port} @ {args.baud} ...")
    try:
        ser = open_port(port, args.baud)
    except serial.SerialException as e:
        sys.exit(f"Could not open {port}: {e}")

    try:
        if args.command:
            cmd = args.command
            if cmd == "play" and args.arg:
                cmd = f"play {args.arg}"
            return one_shot(ser, cmd)
        return interactive(ser)
    finally:
        ser.close()


if __name__ == "__main__":
    sys.exit(main() or 0)
