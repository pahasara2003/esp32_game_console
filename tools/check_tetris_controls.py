"""Check production Tetris control timing independently of screen hardware."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root / "src/tetris.ino").read_text()

def function(name):
    start = source.index("void " + name + "() {")
    brace = source.index("{", start)
    depth = 1
    end = brace + 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[start:end]

prefix = r'''
#include <cassert>
#include <cstdint>
#include <iostream>
unsigned long nowMs=0,elapsedPlayMs=0,lastTick=0,lastDrop=0,lastSoftDrop=0,lastShownSecond=0;
unsigned long drop_interval=600,tetrisSideAt=0;
constexpr unsigned long TETRIS_SOFT_DROP_MS=35;
bool tetrisDownWasHeld=false,tetrisSideRepeating=false,menu_drawn=false;
int gameState=0,tetrisPauseSelection=0,tetrisSideHeld=0;
int falls=0,moves=0,rotations=0;
uint8_t tetrisPressed=0,held=0,pending=0;
unsigned long millis(){return nowMs;}
uint8_t tetrisInputPoll(){auto p=pending;pending=0;return p;}
uint8_t tetrisInputHeld(){return held;}
void tetrisInputRequireRelease(){held=0;}
void tryRotate(){++rotations;}
void updatePosition(int dx=0,int dy=1){if(dx)moves+=dx;if(dy)++falls;}
void updateTime(){}
void drawTetrisPauseMenu(){}
void handleTetrisPause(){}
void drawTetrisGameOver(){}
void handleTetrisGameOver(){}
'''
suffix = r'''
void tick(unsigned long end){for(;nowMs<end;++nowMs)tetrisLoop();}
int main(){
 held=6;pending=6;tick(360); // Down + Right stay independent.
 assert(falls>=10 && falls<=11);assert(moves==4);
 held=0;tick(410);int previous=falls;
 held=1;pending=1;tick(510);assert(rotations==1);assert(falls==previous);
 pending=16;held=20;tick(550);assert(gameState==1 && falls==previous);
 assert(tetrisSideHeld==0);
 std::cout<<"PASS: 35ms held-Down fall, independent lateral repeat, one rotation per press and pause priority\n";
}
'''
with tempfile.TemporaryDirectory(prefix="tetris-controls-check-") as tmp:
    path = Path(tmp)
    (path / "check.cpp").write_text(prefix + function("getCommand") + function("tetrisLoop") + suffix)
    subprocess.run(["g++", "-std=c++11", "-O2", str(path / "check.cpp"), "-o", str(path / "check")], check=True)
    subprocess.run([str(path / "check")], check=True)
