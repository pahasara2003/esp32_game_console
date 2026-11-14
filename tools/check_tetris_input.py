"""Exercise the production Tetris input scanner against a simulated ADC."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = "\n".join(line for line in (root / "src/tetris_input.cpp").read_text().splitlines()
                   if not line.startswith("#include"))
prefix = r'''
#include <cassert>
#include <cstdint>
#include <iostream>
constexpr int SHOOT=5, LOW=0;
constexpr uint16_t ADS1X15_REG_CONFIG_MUX_SINGLE_0=0x4000, ADS1X15_REG_CONFIG_MUX_SINGLE_1=0x5000,
 ADS1X15_REG_CONFIG_MUX_SINGLE_2=0x6000, ADS1X15_REG_CONFIG_MUX_SINGLE_3=0x7000,
 RATE_ADS1115_860SPS=0xE0;
unsigned long nowMs=0;
bool shoot=false;
unsigned long millis() { return nowMs; }
int digitalRead(int pin) { assert(pin==SHOOT); return shoot; }
struct Adafruit_ADS1115 {
 int samples[4]={}, channel=0; unsigned long due=0; uint16_t rate=0xA0;
 void setDataRate(uint16_t v) { rate=v; }
 uint16_t getDataRate() { return rate; }
 void startADCReading(uint16_t mux,bool continuous) {
   assert(!continuous); channel=(mux-0x4000)/0x1000; due=nowMs+2;
 }
 bool conversionComplete() { return nowMs>=due; }
 int16_t getLastConversionResults() { assert(nowMs>=due); return samples[channel]; }
} ads;
bool adsConnected=true;
'''
suffix = r'''
unsigned events=0;
uint8_t poll(int ms) {
 uint8_t result=0;
 for(int i=0;i<ms;++i) {
   ++nowMs; auto before=nowMs; auto edge=tetrisInputPoll();
   assert(nowMs==before); result|=edge; if(edge) ++events;
 }
 return result;
}
int main() {
 tetrisInputBegin(); assert(ads.rate==0xE0); assert(poll(60)==0);
 const uint8_t expected[]={2,4,8,1};
 for(int i=0;i<4;++i) {
   events=0; ads.samples[i]=22000;
   assert(poll(50)==expected[i]); assert(tetrisInputHeld()==expected[i]);
   assert(poll(500)==0 && events==1); // Holding generates no repeated press edge.
   ads.samples[i]=16500; assert(poll(50)==0 && tetrisInputHeld()==expected[i]);
   ads.samples[i]=0; assert(poll(50)==0 && tetrisInputHeld()==0);
 }
 ads.samples[0]=ads.samples[1]=22000;
 assert(poll(50)==6 && tetrisInputHeld()==6); // Right must not suppress Down.
 ads.samples[0]=0; assert(poll(50)==0 && tetrisInputHeld()==4);
 tetrisInputRequireRelease(); assert(poll(100)==0 && tetrisInputHeld()==0);
 ads.samples[1]=0; assert(poll(60)==0);
 // Short digital bounce is ignored; one stable press remains one action.
 shoot=true; assert(poll(5)==0); shoot=false; assert(poll(20)==0);
 shoot=true; assert(poll(20)==16); assert(poll(200)==0);
 tetrisInputRequireRelease(); assert(poll(40)==0);
 shoot=false; assert(poll(60)==0); shoot=true; assert(poll(20)==16);
 shoot=false; poll(30);
 tetrisInputEnd(); assert(ads.rate==0xA0 && tetrisInputHeld()==0);
 tetrisInputBegin(); tetrisInputBegin(); tetrisInputEnd(); assert(ads.rate==0xA0);
 adsConnected=false; tetrisInputBegin(); poll(20);
 shoot=true; assert(poll(20)==16); // Pause still works without the ADC.
 tetrisInputEnd();
 std::cout << "PASS: directions, simultaneous Down+Right, hysteresis, debounce, release gating, ADC restoration and digital-only input\n";
}
'''
with tempfile.TemporaryDirectory(prefix="tetris-input-check-") as tmp:
    path = Path(tmp)
    (path / "check.cpp").write_text(prefix + source + suffix)
    subprocess.run(["g++", "-std=c++11", "-O2", str(path / "check.cpp"), "-o", str(path / "check")], check=True)
    subprocess.run([str(path / "check")], check=True)
