#pragma once
#include <stdint.h>

void tetrisInputBegin();
void tetrisInputEnd();
void tetrisInputRequireRelease();
// Bits 0..4 are Up, Right, Down, Left, Shoot. Poll returns new presses.
uint8_t tetrisInputPoll();
uint8_t tetrisInputHeld();
