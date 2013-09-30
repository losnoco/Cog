SegaCore - Saturn and Dreamcast sound emulation


The Saturn (SCSP) and Dreamcast (AICA) use almost-identical sound chips both
made by Yamaha, so it's effective to emulate them both using the same code.
They only differ slightly in which features they support.

To name a few specifics:

                             SCSP     AICA
                             ----     ----
Channels                     32       64
DSP multiplier constants     64       128
FM support                   Yes      No
4-bit ADPCM samples          No       Yes

(The FM support is pretty cool - pretty much arbitrary connections between
channels.  Emulating it would be really inefficient, though.)


yam.c is the heart and does most of the sound chip emulation.

dcsound.c and satsound.c are simply front ends for yam.c.

arm.c handles ARM7 emulation (Dreamcast) and is tragically slow at the moment.

Starscream handles 68000 emulation (Saturn) and is as speedy as you'd expect.
The version used here is "0.27" which I never made official.  The only
difference from 0.26 is that I added a state pointer (EBP) and reshuffled some
of the other registers around.
