#pragma once

#include "structures.h"

void RunImdct(mdct* mdct, double* input, double* output);
void Dct4(mdct* mdct, double* input, double* output);