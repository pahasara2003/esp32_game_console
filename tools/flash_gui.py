#!/usr/bin/env python3
"""Mini GUI to flash a .bin file to an ESP32 using esptool."""

import queue
import subprocess
import sys
import threading
import tkinter as tk
from tkinter import filedialog, messagebox, scrolledtext, ttk

try:
    from serial.tools import list_ports
except ImportError:
    list_ports = None


class FlasherApp:
    def __init__(self, root):
        self.root = root
        self.root.title("ESP32 Flasher")
        self.log_queue = queue.Queue()

        frame = ttk.Frame(root, padding=10)
        frame.pack(fill="both", expand=True)

        ttk.Label(frame, text="Port").grid(row=0, column=0, sticky="w")
        self.port = ttk.Combobox(frame, width=30)
        self.port.grid(row=0, column=1, sticky="we", padx=5)
        ttk.Button(frame, text="Refresh", command=self.refresh_ports).grid(row=0, column=2)

        ttk.Label(frame, text="Binary").grid(row=1, column=0, sticky="w", pady=5)
        self.path = tk.StringVar()
        ttk.Entry(frame, textvariable=self.path, width=30).grid(row=1, column=1, sticky="we", padx=5)
        ttk.Button(frame, text="Browse", command=self.browse).grid(row=1, column=2)

        ttk.Label(frame, text="Offset").grid(row=2, column=0, sticky="w")
        self.offset = tk.StringVar(value="0x0")
        ttk.Entry(frame, textvariable=self.offset, width=12).grid(row=2, column=1, sticky="w", padx=5)

        ttk.Label(frame, text="Baud").grid(row=3, column=0, sticky="w", pady=5)
        self.baud = ttk.Combobox(frame, values=["115200", "460800", "921600"], width=12)
        self.baud.set("921600")
        self.baud.grid(row=3, column=1, sticky="w", padx=5)

        btns = ttk.Frame(frame)
        btns.grid(row=4, column=0, columnspan=3, pady=5, sticky="we")
        self.flash_btn = ttk.Button(btns, text="Flash", command=self.flash)
        self.flash_btn.pack(side="left")
        self.erase_btn = ttk.Button(btns, text="Erase flash", command=self.erase)
        self.erase_btn.pack(side="left", padx=5)

        self.log = scrolledtext.ScrolledText(frame, height=14, width=70, state="disabled")
        self.log.grid(row=5, column=0, columnspan=3, sticky="nsew")

        frame.columnconfigure(1, weight=1)
        frame.rowconfigure(5, weight=1)

        self.refresh_ports()
        self.root.after(100, self.drain_log)

    def refresh_ports(self):
        if list_ports is None:
            self.write_log("pyserial not installed: type the port manually (pip install esptool).\n")
            return
        ports = [p.device for p in list_ports.comports()]
        self.port["values"] = ports
        if ports and not self.port.get():
            self.port.set(ports[0])

    def browse(self):
        f = filedialog.askopenfilename(filetypes=[("Binary", "*.bin"), ("All files", "*.*")])
        if f:
            self.path.set(f)

    def write_log(self, text):
        self.log.configure(state="normal")
        self.log.insert("end", text)
        self.log.see("end")
        self.log.configure(state="disabled")

    def drain_log(self):
        while True:
            try:
                self.write_log(self.log_queue.get_nowait())
            except queue.Empty:
                break
        self.root.after(100, self.drain_log)

    def run_esptool(self, args):
        if not self.port.get():
            messagebox.showerror("Error", "Select a serial port.")
            return
        cmd = [sys.executable, "-m", "esptool", "--port", self.port.get(), "--baud", self.baud.get()] + args
        self.write_log("\n$ " + " ".join(cmd) + "\n")
        self.flash_btn.configure(state="disabled")
        self.erase_btn.configure(state="disabled")
        threading.Thread(target=self.worker, args=(cmd,), daemon=True).start()

    def worker(self, cmd):
        try:
            proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
            for line in proc.stdout:
                self.log_queue.put(line)
            code = proc.wait()
            self.log_queue.put("Done.\n" if code == 0 else f"Failed (exit {code}).\n")
        except FileNotFoundError:
            self.log_queue.put("esptool not found. Run: pip install esptool\n")
        finally:
            self.root.after(0, self.enable_buttons)

    def enable_buttons(self):
        self.flash_btn.configure(state="normal")
        self.erase_btn.configure(state="normal")

    def flash(self):
        if not self.path.get():
            messagebox.showerror("Error", "Choose a .bin file.")
            return
        self.run_esptool(["write-flash", self.offset.get(), self.path.get()])

    def erase(self):
        if messagebox.askyesno("Erase", "Erase the entire flash chip?"):
            self.run_esptool(["erase-flash"])


if __name__ == "__main__":
    root = tk.Tk()
    FlasherApp(root)
    root.mainloop()
