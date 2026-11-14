# Generates modern flat smartphone-style 52x52 RGB565 icons (magenta = transparent).
# Colours are pushed extra-saturated because the ILI9341 panel renders washed out.
import math
from PIL import Image, ImageDraw, ImageEnhance
S = 4; N = 52; W = N * S
def grad(top, bot):
    im = Image.new('RGB', (W, W)); d = ImageDraw.Draw(im)
    for y in range(W):
        t = y / (W - 1)
        d.line([(0, y), (W, y)], fill=tuple(int(top[i] + (bot[i] - top[i]) * t) for i in range(3)))
    return im
def base(top, bot):
    im = grad(top, bot)
    mask = Image.new('L', (W, W), 0)
    ImageDraw.Draw(mask).rounded_rectangle([0, 0, W - 1, W - 1], radius=13 * S, fill=255)
    return im, mask, ImageDraw.Draw(im)
def p(*v): return [x * S for x in v]
WH = (255, 255, 255)
def tetris(d):
    for x, y, c in [(10,26,(255,200,0)),(19,26,(255,200,0)),(28,26,(255,200,0)),(19,17,(255,200,0)),
                    (24,35,(0,230,255)),(33,35,(0,230,255)),(33,26,(0,230,255)),(42,26,(0,230,255)),
                    (6,35,WH),(15,35,WH)]:
        d.rounded_rectangle(p(x, y, x + 8, y + 8), radius=2*S, fill=c)
def snake(d):
    pts = [(12,40),(12,30),(24,30),(24,20),(38,20),(38,12)]
    d.line(p(*sum(pts, ())), fill=WH, width=7*S, joint='curve')
    for x, y in pts: d.ellipse(p(x-3.5, y-3.5, x+3.5, y+3.5), fill=WH)
    d.ellipse(p(36, 36, 44, 44), fill=(255, 40, 60))
def breakout(d):
    cols = [(255,70,90),(255,180,0),(80,230,120)]
    for r in range(3):
        for c in range(4):
            d.rounded_rectangle(p(7+c*10, 9+r*6, 15+c*10, 13+r*6), radius=S, fill=cols[r])
    d.ellipse(p(28, 30, 34, 36), fill=WH)
    d.rounded_rectangle(p(14, 41, 38, 45), radius=2*S, fill=WH)
def run(d):
    d.ellipse(p(28, 8, 36, 16), fill=WH)
    for seg in [(30,18,24,30),(24,30,30,38),(30,38,28,46),(24,30,16,38),(30,19,38,26),(38,26,44,24),(29,20,20,22),(20,22,16,28)]:
        d.line(p(*seg), fill=WH, width=5*S)
    for y in (24, 32): d.line(p(6, y, 13, y), fill=(255,255,255), width=2*S)
def galaga(d):
    d.polygon(p(26,8, 36,38, 26,33, 16,38), fill=WH)
    d.polygon(p(16,30, 8,42, 18,38), fill=(255,60,90)); d.polygon(p(36,30, 44,42, 34,38), fill=(255,60,90))
    d.ellipse(p(23, 18, 29, 24), fill=(0, 200, 255))
    d.polygon(p(22,38, 26,46, 30,38), fill=(255,200,0))
def puzzle(d):
    for i in range(9):
        if i == 8: continue
        x, y = 8 + (i % 3) * 12, 8 + (i // 3) * 12
        d.rounded_rectangle(p(x, y, x + 11, y + 11), radius=2*S, fill=WH if i != 4 else (255,220,0))
def tictac(d):
    for v in (20, 32):
        d.line(p(v, 8, v, 44), fill=WH, width=3*S); d.line(p(8, v, 44, v), fill=WH, width=3*S)
    d.line(p(9,9,17,17), fill=(255,230,0), width=3*S); d.line(p(17,9,9,17), fill=(255,230,0), width=3*S)
    d.ellipse(p(23,23,29,29), outline=WH, width=3*S)
    d.line(p(35,35,43,43), fill=(255,230,0), width=3*S); d.line(p(43,35,35,43), fill=(255,230,0), width=3*S)
def pong(d):
    for y in range(8, 46, 6): d.line(p(26, y, 26, y + 3), fill=(255,255,255), width=S)
    d.rounded_rectangle(p(8, 12, 13, 28), radius=2*S, fill=WH)
    d.rounded_rectangle(p(39, 24, 44, 40), radius=2*S, fill=WH)
    d.ellipse(p(30, 18, 36, 24), fill=(255, 230, 0))
def mines(d):
    for a in range(0, 360, 45):
        r = math.radians(a)
        d.line(p(26, 28, 26 + 16 * math.cos(r), 28 + 16 * math.sin(r)), fill=(30,30,40), width=3*S)
    d.ellipse(p(15, 17, 37, 39), fill=(30, 30, 40))
    d.ellipse(p(20, 21, 26, 27), fill=WH)
    d.polygon(p(34,6, 34,18, 44,12), fill=(255,40,60)); d.line(p(34,6,34,22), fill=(30,30,40), width=2*S)
def life(d):
    for x, y in [(1,0),(2,1),(0,2),(1,2),(2,2)]:
        d.rounded_rectangle(p(10+x*12, 10+y*12, 20+x*12, 20+y*12), radius=3*S, fill=WH)
    for x in range(3):
        for y in range(3):
            if (x, y) not in [(1,0),(2,1),(0,2),(1,2),(2,2)]:
                d.rounded_rectangle(p(10+x*12, 10+y*12, 20+x*12, 20+y*12), radius=3*S, outline=(255,255,255), width=S)
def mandel(d):
    for r, c in [(18,(255,140,0)),(13,(255,60,120)),(8,(140,40,255))]:
        d.ellipse(p(30-r, 26-r, 30+r*0.8, 26+r), fill=c)
    d.ellipse(p(8, 21, 18, 31), fill=(255,140,0)); d.ellipse(p(11, 23, 17, 29), fill=(20,10,40))
    d.ellipse(p(22, 18, 36, 34), fill=(20, 10, 40))
def sound(d):
    d.rectangle(p(20, 12, 24, 36), fill=WH); d.rectangle(p(36, 8, 40, 32), fill=WH)
    d.polygon(p(20,12, 40,6, 40,12, 20,18), fill=WH)
    d.ellipse(p(11, 32, 24, 42), fill=WH); d.ellipse(p(27, 28, 40, 38), fill=WH)
def videos(d):
    d.rounded_rectangle(p(8, 13, 44, 39), radius=5*S, fill=WH)
    d.polygon(p(21,19, 21,33, 33,26), fill=(255,120,0))
def settings(d):
    for a in range(0, 360, 45):
        r = math.radians(a)
        d.line(p(26, 26, 26 + 18 * math.cos(r), 26 + 18 * math.sin(r)), fill=WH, width=7*S)
    d.ellipse(p(12, 12, 40, 40), fill=WH); d.ellipse(p(20, 20, 32, 32), fill=(110, 110, 125))
def info(d):
    d.ellipse(p(9, 9, 43, 43), outline=WH, width=4*S)
    d.ellipse(p(23, 15, 29, 21), fill=WH); d.rounded_rectangle(p(23, 24, 29, 37), radius=S, fill=WH)
ICONS = [
 ('tetris_home_icon',     (140,60,255),(70,0,200),   tetris),
 ('snake_home_icon',      (0,230,110),(0,150,60),    snake),
 ('breakout_home_icon',   (40,60,90),(10,15,35),     breakout),
 ('run_home_icon',        (255,150,0),(255,60,0),    run),
 ('galaga_home_icon',     (30,60,200),(10,10,70),    galaga),
 ('puzzle_home_icon',     (255,60,130),(200,0,90),   puzzle),
 ('tic_tac_home_icon',    (0,200,255),(0,100,230),   tictac),
 ('pong_home_icon',       (60,60,70),(15,15,20),     pong),
 ('mines_home_icon',      (240,240,245),(190,195,210), mines),
 ('life_home_icon',       (0,220,200),(0,130,160),   life),
 ('mandelbrot_home_icon', (40,20,80),(10,5,30),      mandel),
 ('sound_home_icon',      (255,60,90),(220,0,60),    sound),
 ('videos_home_icon',     (255,170,0),(255,90,0),    videos),
 ('settings_home_icon',   (160,160,175),(90,90,105), settings),
 ('about_home_icon',      (0,140,255),(0,70,220),    info),
]
out = ['#pragma once', '#include <stdint.h>', '#include <pgmspace.h>', '',
       '// Generated by tools/make_icons.py: 52x52 RGB565, magenta 0xF81F transparent.', '']
for name, top, bot, fn in ICONS:
    im, mask, d = base(top, bot); fn(d)
    im = ImageEnhance.Color(im).enhance(1.25)
    im = im.resize((N, N), Image.LANCZOS); mask = mask.resize((N, N), Image.LANCZOS)
    px = []
    for y in range(N):
        for x in range(N):
            if mask.getpixel((x, y)) < 110: px.append(0xF81F); continue
            r, g, b = im.getpixel((x, y))
            v = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)
            px.append(0xF81E if v == 0xF81F else v)
    out.append(f'const uint16_t {name}[2704] PROGMEM = {{')
    for i in range(0, len(px), 16): out.append('  ' + ', '.join(f'0x{v:04X}' for v in px[i:i+16]) + ',')
    out.append('};'); out.append('')
    Image.composite(im, Image.new('RGB', (N, N), (30, 30, 60)), mask).resize((N*3, N*3), Image.NEAREST).save(f'/tmp/claude-1000/-home-pahasara-gameConsole/5a8399be-79e5-4ae1-a092-35d5a2d860bf/scratchpad/{name}.png')
open('src/home_icons.h', 'w').write('\n'.join(out))
