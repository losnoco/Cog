/*
** This file is part of the ProTracker 2.3D port/clone
** project by Olav "8bitbubsy" Sorensen.
**
** It contains unstructured and unclean code, but I care
** more about how the program works than how the source
** code looks. Although, I do respect coders that can
** master the art of writing clean and structured code.
** I know I can't.
**
** All of the files are considered 'public domain',
** do whatever you want with it.
**
*/

#ifndef __PT_BLEP_H
#define __PT_BLEP_H

#include <stdint.h>

// thanks to aciddose/ad/adejr for the blep/cutoff/filter stuff!

// information on blep variables
//
// ZC = zero crossings, the number of ripples in the impulse
// OS = oversampling, how many samples per zero crossing are taken
// SP = step size per output sample, used to lower the cutoff (play the impulse slower)
// NS = number of samples of impulse to insert
// RNS = the lowest power of two greater than NS, minus one (used to wrap output buffer)
//
// ZC and OS are here only for reference, they depend upon the data in the table and can't be changed.
// SP, the step size can be any number lower or equal to OS, as long as the result NS remains an integer.
// for example, if ZC=8,OS=5, you can set SP=1, the result is NS=40, and RNS must then be 63.
// the result of that is the filter cutoff is set at nyquist * (SP/OS), in this case nyquist/5.

#define ZC 8
#define OS 5
#define SP 5
#define NS (ZC * OS / SP)
#define RNS 7 // RNS = (2^ > NS) - 1

typedef struct blep_data
{
    int32_t index;
    int32_t samplesLeft;
    float buffer[RNS + 1];
    float lastInput;
    float lastOutput;
} BLEP;

void blepAdd(BLEP *b, float offset, float amplitude);
float blepRun(BLEP *b);

#endif

