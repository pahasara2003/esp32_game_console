#!/usr/bin/env python3
"""Flash the latest game console release to a connected ESP32-C3.

Usage: pip install esptool pyserial, then: python tools/flasher_gui.py
The release ships a merged image flashed at offset 0x0. Port is auto-detected.
"""
import json
import os
import queue
import re
import subprocess
import sys
import threading
import tkinter as tk
from tkinter import ttk
from urllib.request import urlopen, Request

REPO = "pahasara2003/esp32_game_console"
OFFSET = "0x0"  # merged image covers bootloader + app, per release notes
BAUD = "921600"
TMP = "/tmp/opencode"

# message tuples: ("log", line) ("pct", int) ("stage", text) ("board", text|None) ("busy", bool)
Q = queue.Queue()


def send(kind, value=""):
    Q.put((kind, str(value)))


def detect_port():
    """Auto-detect the ESP32-C3: prefer Espressif native USB (VID 0x303a),
    then common UART bridges; ignore the fake /dev/ttyS* ports on Linux."""
    import serial.tools.list_ports
    vid_order = {0x303A: 0, 0x10C4: 1, 0x1A86: 2}  # espressif, cp210x, ch340
    cands = []
    for p in serial.tools.list_ports.comports():
        if p.device.startswith("/dev/ttyS"):
            continue
        cands.append((vid_order.get(p.vid or 0, 9), p.device))
    if not cands:
        raise RuntimeError("No ESP32-C3 port detected")
    cands.sort()
    return cands[0][1]


def latest_bin_url():
    req = Request(f"https://api.github.com/repos/{REPO}/releases/latest",
                  headers={"Accept": "application/vnd.github+json"})
    with urlopen(req, timeout=30) as r:
        release = json.load(r)
    bins = [a for a in release["assets"] if a["name"].endswith(".bin")]
    if not bins:
        raise RuntimeError("No .bin asset in the latest release")
    return bins[0]["browser_download_url"], bins[0]["name"]


def download(url, dest):
    with urlopen(url, timeout=30) as r, open(dest, "wb") as f:
        total = int(r.headers.get("Content-Length", 0))
        done = 0
        while True:
            chunk = r.read(65536)
            if not chunk:
                break
            f.write(chunk)
            done += len(chunk)
            if total:
                send("pct", done * 100 // total)
                send("stage", f"Downloading {done * 100 // total}%")
    return dest


PCT_RE = re.compile(r"\((\d+) %\)")


def flash(port, path):
    # default-reset works over USB-Serial-JTAG on the C3; if it fails,
    # hold BOOT, tap RST, release BOOT, then retry.
    cmd = [sys.executable, "-m", "esptool", "--chip", "esp32c3",
           "--port", port, "--baud", BAUD,
           "--before", "default-reset", "--after", "hard-reset",
           "write_flash", OFFSET, path]
    proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                            text=True, bufsize=1)
    pct = 0
    for line in proc.stdout:
        for m in PCT_RE.finditer(line):
            pct = max(pct, int(m.group(1)))
        send("pct", pct)
        send("stage", f"Flashing {pct}%")
        # esptool rewrites progress with \r on one line; keep only the last
        # piece and only the lines worth showing in the log
        tail = line.strip().split("\r")[-1].strip()
        if tail and any(k in tail for k in
                        ("Connecting", "Chip is", "Flash memory", "Writing",
                         "Hash of data", "Verifying", "Hard resetting")):
            send("log", tail)
    proc.wait()
    if proc.returncode:
        raise RuntimeError(f"esptool failed (exit {proc.returncode})")


def worker():
    try:
        url, name = latest_bin_url()
        send("stage", f"Downloading {name}")
        send("log", f"Latest release: {name}")
        dest = os.path.join(TMP, name)
        download(url, dest)
        send("log", f"Saved to {dest}")

        port = detect_port()
        send("board", port)
        send("stage", "Flashing")
        send("pct", 0)
        send("log", f"Flashing to {port} at {BAUD} baud...")
        flash(port, dest)
        send("pct", 100)
        send("stage", "Done — board is resetting")
        send("log", "Flash complete! Board is resetting...")
    except Exception as e:
        send("stage", "Failed")
        send("log", f"ERROR: {e}")
    finally:
        send("busy", "")


# ---------------- UI ----------------

BG = "#f4f6fb"
CARD = "#ffffff"
BORDER = "#e2e8f0"
TRACK = "#e2e8f0"
FG = "#0f172a"
BODY = "#334155"
MUTED = "#64748b"
ACCENT = "#2563eb"
ACCENT_DARK = "#1d4ed8"
GREEN = "#15803d"
RED = "#dc2626"
FONT = "Segoe UI"

root = tk.Tk()
root.title("Game Console Flasher")
root.configure(bg=BG)
root.resizable(False, False)

style = ttk.Style()
style.theme_use("clam")
style.configure("TProgressbar", troughcolor=TRACK, background=ACCENT,
                bordercolor=TRACK, lightcolor=ACCENT, darkcolor=ACCENT, thickness=10)

busy = False


def card():
    return tk.Frame(outer, bg=CARD, highlightbackground=BORDER, highlightthickness=1)


outer = tk.Frame(root, bg=BG, padx=28, pady=24)
outer.pack()

tk.Label(outer, text="Game Console Flasher", bg=BG, fg=FG,
         font=(FONT, 18, "bold")).pack(anchor="w")
tk.Label(outer, text=f"ESP32-C3  ·  {REPO.split('/')[1]}  ·  offset {OFFSET}",
         bg=BG, fg=MUTED, font=(FONT, 10)).pack(anchor="w", pady=(2, 0))

status_card = card()
status_card.pack(fill="x", pady=(20, 0))
board_lbl = tk.Label(status_card, text="Scanning for ESP32-C3...", bg=CARD, fg=MUTED,
                     font=(FONT, 11), padx=16, pady=12)
board_lbl.pack(side="left")
tk.Button(status_card, text="Rescan", command=lambda: rescan(), bg=CARD, fg=ACCENT,
          activebackground=CARD, activeforeground=ACCENT_DARK, relief="flat", bd=0,
          font=(FONT, 10, "bold"), cursor="hand2", padx=16).pack(side="right")

bar = ttk.Progressbar(outer, mode="determinate", maximum=100, length=460)
bar.pack(fill="x", pady=(20, 6))
stage_lbl = tk.Label(outer, text="Ready", bg=BG, fg=MUTED, font=(FONT, 10))
stage_lbl.pack(anchor="w")

flash_btn = tk.Button(outer, text="Download latest & flash", bg=ACCENT, fg="white",
                      activebackground=ACCENT_DARK, activeforeground="white",
                      disabledforeground="#bfdbfe", relief="flat", bd=0,
                      font=(FONT, 12, "bold"), pady=12, cursor="hand2")
flash_btn.pack(fill="x", pady=(16, 0))

log_card = card()
log_card.pack(fill="x", pady=(20, 0))
tk.Label(log_card, text="LOG", bg=CARD, fg=MUTED, font=(FONT, 8, "bold"),
         anchor="w").pack(fill="x", padx=16, pady=(12, 0))
log_txt = tk.Text(log_card, height=10, width=60, state="disabled", bg=CARD, fg=BODY,
                  relief="flat", bd=0, highlightthickness=0, font=("Consolas", 9),
                  padx=16, pady=8, insertbackground=BODY)
log_txt.tag_config("err", foreground=RED)
log_txt.tag_config("ok", foreground=GREEN)
log_txt.pack(fill="x")

tk.Label(outer, text="If flashing fails: hold BOOT, tap RST, release BOOT, then retry.",
         bg=BG, fg=MUTED, font=(FONT, 9)).pack(anchor="w", pady=(14, 0))


def rescan():
    try:
        port = detect_port()
        board_lbl.config(text=f"●  ESP32-C3 on {port}", fg=GREEN)
        flash_btn.config(state="normal" if not busy else "disabled")
    except RuntimeError as e:
        board_lbl.config(text=f"○  {e}", fg=RED)
        flash_btn.config(state="disabled")


def start():
    global busy
    busy = True
    flash_btn.config(state="disabled", text="Working...")
    log_txt.config(state="normal")
    log_txt.delete("1.0", "end")
    log_txt.config(state="disabled")
    bar["value"] = 0
    threading.Thread(target=worker, daemon=True).start()


flash_btn.config(command=start)


def poll():
    global busy
    try:
        while True:
            kind, val = Q.get_nowait()
            if kind == "log":
                tag = "err" if val.startswith("ERROR") else "ok" if val.startswith("Flash complete") else ""
                log_txt.config(state="normal")
                log_txt.insert("end", val + "\n", tag)
                log_txt.see("end")
                log_txt.config(state="disabled")
            elif kind == "pct":
                bar["value"] = int(val)
            elif kind == "stage":
                stage_lbl.config(text=val)
            elif kind == "board":
                rescan()
            elif kind == "busy":
                busy = False
                flash_btn.config(text="Download latest & flash")
                rescan()
    except queue.Empty:
        pass
    root.after(150, poll)


rescan()
poll()
root.mainloop()
