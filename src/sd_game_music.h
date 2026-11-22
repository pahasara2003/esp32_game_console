#pragma once
#include <stddef.h>
#include <stdint.h>

// Legacy API name; game music is now embedded in flash and does not access SD.
namespace sd_music {
void begin();
bool fill(int16_t *samples, size_t count, bool enabled);
void stop();
}
