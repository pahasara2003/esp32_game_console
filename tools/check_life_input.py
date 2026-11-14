"""Check Life's real asynchronous ADC input code using a simulated ADS1115."""
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
source = "\n".join(line for line in (ROOT / "src" / "life_input.cpp").read_text().splitlines()
                   if not line.startswith("#include"))
prefix = r'''
#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <vector>
constexpr uint16_t ADS1X15_REG_CONFIG_MUX_SINGLE_0=0x4000, ADS1X15_REG_CONFIG_MUX_SINGLE_1=0x5000,
  ADS1X15_REG_CONFIG_MUX_SINGLE_2=0x6000, ADS1X15_REG_CONFIG_MUX_SINGLE_3=0x7000, RATE_ADS1115_860SPS=0xE0;
unsigned long nowMs=0;
unsigned long millis() { return nowMs; }
struct Adafruit_ADS1115 {
  int samples[4]={}; int channel=0,reads=0,starts=0; unsigned long due=0; uint16_t rate=0x80;
  void setDataRate(uint16_t value) { rate=value; }
  uint16_t getDataRate() { return rate; }
  void startADCReading(uint16_t mux,bool continuous) {
    assert(!continuous); channel=(mux-0x4000)/0x1000; assert(channel>=0&&channel<4);
    due=nowMs+2; ++starts;
  }
  bool conversionComplete() { return nowMs>=due; }
  int16_t getLastConversionResults() { assert(nowMs>=due); ++reads; return samples[channel]; }
} ads;
bool adsConnected=true;
void lifeInputEnd();
'''
suffix = r'''
static std::vector<int> poll(int ms) {
  std::vector<int> events;
  for(int i=0;i<ms;++i) {
    ++nowMs; auto before=nowMs; int event=lifeInputPoll(true);
    assert(nowMs==before); if(event>=0) events.push_back(event);
  }
  return events;
}
int main() {
  lifeInputBegin(); assert(ads.rate==RATE_ADS1115_860SPS);
  assert(poll(50).empty());
  const int portrait[]={1,2,3,0};
  for(int channel=0;channel<4;++channel) {
    ads.samples[channel]=22000;
    auto events=poll(50); assert(events.size()==1&&events[0]==portrait[channel]);
    assert(poll(80).empty()); events=poll(70); assert(events.size()==1&&events[0]==portrait[channel]);
    ads.samples[channel]=16500; assert(poll(20).empty()); // Hysteresis retains a real held press.
    ads.samples[channel]=0; assert(poll(50).empty()); assert(lifeInput::held==-1);
  }
  // One high conversion is rejected; a second one is needed before a press.
  while(lifeInput::channel!=0) poll(1);
  ads.samples[0]=22000;
  assert(poll(2).empty()); ads.samples[0]=0; assert(poll(20).empty());
  lifeInputEnd(); assert(ads.rate==0x80&&!lifeInput::active);
  int starts=ads.starts; assert(poll(25).empty()&&ads.starts==starts);
  adsConnected=false; lifeInputBegin(); assert(!lifeInput::active&&ads.rate==0x80);
  assert(poll(25).empty()); adsConnected=true;
  ads.rate=0xA0; lifeInputBegin(); lifeInputBegin(); lifeInputEnd(); assert(ads.rate==0xA0);
  std::cout<<"PASS: nonblocking conversion polling, portrait directions, two-sample debounce, hysteresis, repeat timing and ADC restoration\n";
}
'''
with tempfile.TemporaryDirectory(prefix="life-input-check-") as tmp:
    path = Path(tmp)
    (path / "check.cpp").write_text(prefix + source + suffix)
    subprocess.run(["g++", "-std=c++11", "-O2", str(path / "check.cpp"), "-o", str(path / "check")], check=True)
    subprocess.run([str(path / "check")], check=True)
