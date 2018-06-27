#pragma once

#include <stdint.h>

void Inintialize_ADS1115();
void OnConversionReady(int gpio, int level, uint32_t tick);
