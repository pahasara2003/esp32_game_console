#pragma once
#include <stdint.h>
// Commands/bits: Up, Right, Down, Left, Shoot. The scanner owns the ADC.
bool buttonsBegin();
uint8_t buttonsHeld();
int buttonsNextPress();
bool buttonsPendingPress();
void buttonsDebugKey(char key);
// Clear pending commands on screen changes; each key rearms independently
// after its own confirmed release, so held keys cannot block other controls.
void buttonsRequireRelease();
void buttonsPrintDiagnostics();
void buttonsRecordFor(uint32_t durationMs);
