#pragma once
#include <algorithm>
#include <stdint.h>
#include <string.h>
using std::min;
using std::max;
using portMUX_TYPE = int;
#define portMUX_INITIALIZER_UNLOCKED 0
#define portENTER_CRITICAL(x) ((void)(x))
#define portEXIT_CRITICAL(x) ((void)(x))
template<class T> T constrain(T x, T a, T b) { return std::min(b, std::max(a,x)); }
