#!/usr/bin/env python3
"""Use the PC keyboard as the console's buttons over USB serial.

Arrows -> w/d/s/a (up/right/down/left), Space -> shoot/select, q quits.
Usage: python3 tools/pc_keys.py [port or ACM index]
Without an argument, automatically uses an available ACM port.
Close any other serial monitor first.
"""
import glob, sys, tty, termios, serial

ports = sorted(glob.glob("/dev/ttyACM*"))
if len(sys.argv) > 1:
    choice = sys.argv[1]
    port = f"/dev/ttyACM{choice}" if choice.isdecimal() else choice
else:
    if not ports:
        sys.exit("No /dev/ttyACM* devices found")
    port = ports[0]
if port not in ports and port.startswith("/dev/ttyACM"):
    sys.exit(f"Device not found: {port}")
print(f"Using {port}")
ser = serial.Serial(port, 115200, timeout=0)
ARROWS = {"\x1b[A": b"w", "\x1b[B": b"s", "\x1b[C": b"d", "\x1b[D": b"a"}

fd = sys.stdin.fileno()
old = termios.tcgetattr(fd)
print("Arrows = move, Space = shoot, q = quit\r")
try:
    tty.setcbreak(fd)
    termios.tcflush(fd, termios.TCIFLUSH)
    while True:
        ch = sys.stdin.read(1)
        if ch == "\x1b":
            ch += sys.stdin.read(2)
            if ch in ARROWS:
                ser.write(ARROWS[ch])
        elif ch == " ":
            ser.write(b"e")
        elif ch in ("q", "Q"):
            break
finally:
    termios.tcsetattr(fd, termios.TCSADRAIN, old)
    ser.close()
