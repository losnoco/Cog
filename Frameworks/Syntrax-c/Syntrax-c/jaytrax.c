#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include "jaytrax.h"
#include "mixcore.h"

#ifndef M_PI
#define M_PI (3.14159265359)
#endif

int32_t frequencyTable[SE_NROFFINETUNESTEPS][128];
int16_t sineTab[256];
uint8_t isStaticInit = 0;

//void SoundEngine::DeAllocate()

static void handleEffects(JT1Player* SELF, int32_t channr) {
    int32_t f;
    for (f=0; f<SE_EFF_INST; f++) {
        JT1Voice* vc; JT1VoiceEffect* vfx; JT1Inst* ins; JT1Effect* fx;
        int16_t s;
        
        vc  = &SELF->voices[channr];
        ins = SELF->song->instruments[vc->instrument];
        fx  = &ins->fx[f];
        vfx = &vc->fx[f];
        
        // increase oscilator
        s = (int16_t) fx->oscspd;
        vfx->osccnt += s;
        vfx->osccnt &= 255;

        switch (fx->effecttype) {
            case 0: //none
                break;
            case 1: { //negate
                    int32_t dest;
                    int16_t *dw;
                    int16_t i,s,c;
                    dest = fx->dsteffect;
                    s = (int16_t)fx->effectspd;
                    c = (int16_t)vfx->fxcnt1;
                    dw = &vc->waves[256*dest];
                    for (i=0; i<s;i++) {
                        c++;
                        c&=255;
                        dw[c] = 0-dw[c];
                    }
                    vfx->fxcnt1 = (int32_t)c;
                }
                break;
            case 2: { // warp
                    int32_t dest;
                    int16_t *dw;
                    int16_t i,s,c;
                    dest = fx->dsteffect;
                    s = (int16_t) fx->effectspd;
                    dw = &vc->waves[256*dest];
                    c = 0;
                    for (i=0; i<256; i++) {
                        dw[i] += c;
                        c+=s;
                    }
                }
                break;
            case 3: { // Filter
                    int32_t dest, src;
                    int16_t *dw, *sw;
                    int16_t i, s, t;
                    dest = fx->dsteffect;
                    src = fx->srceffect1;
                    dw = &vc->waves[256*dest];
                    sw = &vc->waves[256*src];
                    s = (int16_t) fx->effectspd;
                    if(s>12) s=12; //not more than 12 times...it slowes down too much
                    for (t=0; t<s; t++) {
                        dw[0] = (sw[255] +sw[1])>>1;
                        for (i=1; i<255; i++) {
                            dw[i] = (sw[i-1] +sw[i+1])>>1;
                        }
                        dw[255] = (sw[254] +sw[0])>>1;
                    }
                }
                break;
            case 4: { // Wavemix
                    int32_t dest, src1, src2;
                    int16_t *dw, *sw1, *sw2;
                    int16_t i, s, c;
                    dest = fx->dsteffect;
                    src1 = fx->srceffect1;
                    src2 = fx->srceffect2;
                    dw = &vc->waves[256*dest];
                    sw1 = &vc->waves[256*src1];
                    sw2 = &vc->waves[256*src2];
                    s = (int16_t) fx->effectspd;
                    vfx->fxcnt1 += s;
                    vfx->fxcnt1 &= 255;
                    c = (int16_t)vfx->fxcnt1;
                    for (i=0; i<256; i++) {
                        dw[i] = (sw1[i] +sw2[c])>>1;
                        c++;
                        c&=255;
                    }
                }
                break;
            case 5: { // Resonance
                    int32_t dest, src1, src2;
                    int16_t *dw, *sw1, *sw2;
                    int16_t i, c;
                    dest = fx->dsteffect;
                    src1 = fx->srceffect1;
                    src2 = fx->osceffect-1;
                    dw = &vc->waves[256*dest];
                    sw1 = &vc->waves[256*src1];
                    sw2 = &vc->waves[256*src2];

                    c = (int16_t)vfx->osccnt;

    // init
                    double centerFreq, bandwidth;
                    if(fx->osceffect==0) {
                        centerFreq = (double)(fx->effectvar1*20);
                        bandwidth = (double)(fx->effectvar2*16);
                    } else {
                        if(fx->oscflg) {
                            centerFreq = (double)(fx->effectvar1*20);
                            bandwidth = (double)(sw2[c]+32768)/16;
                        } else {
                            centerFreq = (double)(sw2[c]+32768)/13;
                            bandwidth = (double)(fx->effectvar2*16);
                        }
                    }

                    vfx->b2 = exp(-(2 * M_PI) * (bandwidth / 22000));
                    vfx->b1 = (-4.0 * vfx->b2) / (1.0 + vfx->b2) * cos(2 * M_PI * (centerFreq / 22000));
                    vfx->a0 = (1.0 - vfx->b2) * sqrt(1.0 - (vfx->b1 * vfx->b1) / (4.0 * vfx->b2));

                    for (i=0; i<256; i++) {
                        double o;
                        o = vfx->a0 * ((double)(sw1[i])/32768) - vfx->b1 * vfx->y1 - vfx->b2 * vfx->y2;

                        vfx->y2 = vfx->y1;
                        vfx->y1 = o;
                        if(o>.9999)o=.9999;
                        if(o<-.9999)o=-.9999;
                        dw[i] = (int16_t)(o*32768);
                    }
                }
                break;
            case 6: { // Reso Whistle
                    int32_t dest,src1,src2;
                    int16_t *dw,*sw1,*sw2;
                    int16_t i,c;
                    dest = fx->dsteffect;
                    src1 = fx->srceffect1;
                    src2 = fx->osceffect-1;
                    dw = &vc->waves[256*dest];
                    sw1 = &vc->waves[256*src1];
                    sw2 = &vc->waves[256*src2];

                    c = (int16_t)vfx->osccnt;

    // init
                    double centerFreq, bandwidth;
                    
                    if(fx->osceffect==0) {
                        centerFreq = (double)(fx->effectvar1*20);
                        bandwidth = (double)(fx->effectvar2*16);
                    } else {
                        if(fx->oscflg) {
                            centerFreq = (double)(fx->effectvar1*20);
                            bandwidth = (double)(sw2[c]+32768)/16;
                        } else {
                            centerFreq = (double)(sw2[c]+32768)/13;
                            bandwidth = (double)(fx->effectvar2*16);
                        }
                    }

                    vfx->b2 = exp(-(2 * M_PI) * (bandwidth / 22000));
                    vfx->b1 = (-4.0 * vfx->b2) / (1.0 + vfx->b2) * cos(2 * M_PI * (centerFreq / 22000));
                    vfx->a0 = (1.0 - vfx->b2) * sqrt(1.0 - (vfx->b1 * vfx->b1) / (4.0 * vfx->b2));

                    vfx->b2*=1.2; // do the reso whistle
                    for (i=0; i<256; i++) {
                        double o;
                        o = vfx->a0 * ((double)(sw1[i])/32768) - vfx->b1 * vfx->y1 - vfx->b2 * vfx->y2;

                        vfx->y2 = vfx->y1;
                        vfx->y1 = o;
                        if(o>.9999)o=.9999;
                        if(o<-.9999)o=-.9999;
                        dw[i] = (int16_t)(o*32768);
                    }
                }
                break;
            case 7: { // Morphing
                    int32_t dest,src1,src2,osc;
                    int16_t *dw,*sw1,*sw2,*ow;
                    int16_t i,c;
                    dest = fx->dsteffect;
                    src1 = fx->srceffect1;
                    src2 = fx->srceffect2;
                    osc = fx->osceffect-1;
                    dw = &vc->waves[256*dest];
                    sw1 = &vc->waves[256*src1];
                    sw2 = &vc->waves[256*src2];
                    ow = &vc->waves[256*osc];

                    c = (int16_t)vfx->osccnt;

    // init
                    int16_t m1,m2;
                    if(fx->osceffect==0) {
                        m1 = fx->effectvar1;
                    } else {
                        if(fx->oscflg) {
                            m1 = fx->effectvar1;
                        } else {
                            m1 = (ow[c]+32768)/256;
                        }
                    }

                    m2 = 255-m1;
                    for (i=0; i<256; i++) {
                        int32_t a;
                        a=(((int32_t)sw1[i]*m1)/256)+(((int32_t)sw2[i]*m2)/256);
                        dw[i] = (int16_t)(a);
                    }
                }
                break;
            case 8: { // Dyna-Morphing
                    int32_t dest,src1,src2,osc;
                    int16_t *dw,*sw1,*sw2,*ow,*si;
                    int16_t i,c;
                    si = &sineTab[0];
                    dest = fx->dsteffect;
                    src1 = fx->srceffect1;
                    src2 = fx->srceffect2;
                    osc = fx->osceffect-1;
                    dw = &vc->waves[256*dest];
                    sw1 = &vc->waves[256*src1];
                    sw2 = &vc->waves[256*src2];
                    ow = &vc->waves[256*osc];

                    c = (int16_t)vfx->osccnt;

    // init
                    int16_t m1,m2,sc; //sc is sincnt
                    if(fx->osceffect==0) {
                        sc = fx->effectvar1;
                    } else {
                        if(fx->oscflg)
                        {
                            sc = fx->effectvar1;
                        }
                        else
                        {
                            sc = (ow[c]+32768)/256;
                        }
                    }

                    for (i=0; i<256; i++) {
                        int32_t a;
                        m1=(si[sc]>>8)+128;
                        m2 = 255-m1;
                        a=(((int32_t)sw1[i]*m1)/256)+(((int32_t)sw2[i]*m2)/256);
                        dw[i] = (int16_t)(a);
                        sc++;
                        sc&=255;
                    }
                }
                break;
            case 9: { // Distortion
                    int32_t dest,src1,osc;
                    int16_t *dw,*sw1,*ow;
                    int16_t i,c;
                    dest = fx->dsteffect;
                    src1 = fx->srceffect1;
                    osc = fx->osceffect-1;
                    dw = &vc->waves[256*dest];
                    sw1 = &vc->waves[256*src1];
                    ow = &vc->waves[256*osc];

                    c = (int16_t)vfx->osccnt;

    // init
                    int16_t m1;
                    if(fx->osceffect==0) {
                        m1 = fx->effectvar1;
                    } else {
                        if(fx->oscflg) {
                            m1 = fx->effectvar1;
                        } else {
                            m1 = (ow[c]+32768)/256;
                        }
                    }

                    for (i=0; i<256; i++) {
                        int32_t a;
                        a=((int32_t)sw1[i]*m1)/16;
                        a+=32768;
                        if(a<0)a=-a;
                        a%=131072;
                        if(a>65535) a = 131071-a;
                        a-=32768;
                        dw[i] = (int16_t)(a);
                    }
                }
                break;
            case 10: { // Scroll left
                
                    int32_t dest;
                    int16_t *dw;
                    int16_t i,t;
                    dest = fx->dsteffect;
                    dw = &vc->waves[256*dest];

                    t=dw[0];
                    for (i=0; i<255; i++) {
                        dw[i] = dw[i+1];
                    }
                    dw[255]=t;
                }
                break;
            case 11: { // Upsample
                
                    int32_t dest;
                    int16_t *dw;
                    int16_t i,c;
                    c = (int16_t)vfx->fxcnt1;
                    if(c != 0) { // timeout ended?
                        vfx->fxcnt1--;
                        break;
                    }
                    vfx->fxcnt1 = fx->effectvar1;
                    dest = fx->dsteffect;
                    dw = &vc->waves[256*dest];

                    for (i=0; i<128; i++) {
                        dw[i]=dw[i*2];
                    }
                    memcpy(&dw[128], &dw[0], 256);
                }
                break;
            case 12: { // Clipper
                    int32_t dest,src1,osc;
                    int16_t *dw,*sw1,*ow;
                    int16_t i,c;
                    dest = fx->dsteffect;
                    src1 = fx->srceffect1;
                    osc = fx->osceffect-1;
                    dw = &vc->waves[256*dest];
                    sw1 = &vc->waves[256*src1];
                    ow = &vc->waves[256*osc];

                    c = (int16_t)vfx->osccnt;

    // init
                    int16_t m1;
                    if(fx->osceffect==0) {
                        m1 = fx->effectvar1;
                    } else {
                        if(fx->oscflg) {
                            m1 = fx->effectvar1;
                        } else {
                            m1 = (ow[c]+32768)/256;
                        }
                    }

                    for (i=0; i<256; i++) {
                        int32_t a;
                        a=((int32_t)sw1[i]*m1)/16;
                        if(a<-32767)a=-32767;
                        if(a>32767)a=32767;
                        dw[i] = (int16_t)(a);
                    }
                }
                break;
            case 13: { // bandpass
                    int32_t dest,src1,src2;
                    int16_t *dw,*sw1,*sw2;
                    int16_t i,c;
                    int32_t _2_pi_w0;
                    int32_t _1000_Q;

                    dest = fx->dsteffect;
                    src1 = fx->srceffect1;
                    src2 = fx->osceffect-1;
                    dw = &vc->waves[256*dest];
                    sw1 = &vc->waves[256*src1];
                    sw2 = &vc->waves[256*src2];

                    c = (int16_t)vfx->osccnt;

    // init
                    int32_t freq,reso;
                    if(fx->osceffect==0) {
                        freq = fx->effectvar1;
                        reso = fx->effectvar2;
                        freq*=16; //(freq 0 - 16000hz)
                    } else {
                        if(fx->oscflg) {
                            freq = fx->effectvar1;
                            reso = (sw2[c]+32768)>>8;
                            freq*=16; //(freq 0 - 16000hz)
                        } else {
                            freq = (sw2[c]+32768)/16;
                            reso = fx->effectvar2;
                        }
                    }
                    //calc freq;
                    //double x = freq - 920.0;
                    //double w0 = 228 + 3900/2*(1 + tanh(_copysign(pow(fabs(x), 0.85)/95, x)));
                    double w0 = 228+freq;
                    _2_pi_w0 = (int32_t)(2*M_PI*w0);

                    //calc Q
                    _1000_Q = 707 + 1000*reso/128;

                    int32_t _2_pi_w0_delta_t;
                    int32_t Vhp_next;
                    int32_t Vbp_next;
                    int32_t Vlp_next;
                    int32_t Vbp;
                    int32_t Vlp;
                    int32_t Vhp;
                    int32_t Vi;
                    int32_t s;
                    int32_t delta_t;

                    Vbp = vfx->Vbp;
                    Vlp = vfx->Vlp;
                    Vhp = vfx->Vhp;
                    delta_t=8;
                    
                    // now let's throw our waveform through the resonator!
                    for (i=0; i<256; i++) {
                        // delta_t is converted to seconds given a 1MHz clock by dividing
                        // with 1 000 000. This is done in three operations to avoid integer
                        // multiplication overflow.
                        _2_pi_w0_delta_t = _2_pi_w0*delta_t/100;

                        // Calculate filter outputs.
                        Vi=sw1[i];
                        Vhp_next = Vbp*1000/_1000_Q - Vlp + Vi;
                        Vbp_next = Vbp - _2_pi_w0_delta_t*(Vhp/100)/100;
                        Vlp_next = Vlp - _2_pi_w0_delta_t*(Vbp/100)/100;
                        Vhp = Vhp_next;
                        Vbp = Vbp_next;
                        Vlp = Vlp_next;

                        s = Vlp;
                        if(s>32767)s=32767;
                        if(s<-32767)s=-32767;

                        dw[i] = (int16_t)(s);
                    }
                    vfx->Vbp = Vbp;
                    vfx->Vlp = Vlp;
                    vfx->Vhp = Vhp;
                }
                break;
            case 14: { // highpass
                    int32_t dest,src1,src2;
                    int16_t *dw,*sw1,*sw2;
                    int16_t i,c;
                    int32_t _2_pi_w0;
                    int32_t _1000_Q;

                    dest = fx->dsteffect;
                    src1 = fx->srceffect1;
                    src2 = fx->osceffect-1;
                    dw = &vc->waves[256*dest];
                    sw1 = &vc->waves[256*src1];
                    sw2 = &vc->waves[256*src2];

                    c = (int16_t)vfx->osccnt;

    // init
                    int32_t freq, reso;
                    if(fx->osceffect==0) {
                        freq = fx->effectvar1;
                        reso = fx->effectvar2;
                        freq*=32; //(freq 0 - 16000hz)
                    } else {
                        if(fx->oscflg) {
                            freq = fx->effectvar1;
                            reso = (sw2[c]+32768)>>8;
                            freq*=32; //(freq 0 - 16000hz)
                        } else {
                            freq = (sw2[c]+32768)/8;
                            reso = fx->effectvar2;
                        }
                    }
                    //calc freq;
                    //double x = freq - 920.0;
                    //double w0 = 228 + 3900/2*(1 + tanh(_copysign(pow(fabs(x), 0.85)/95, x)));
                    double w0 = 228+freq;
                    _2_pi_w0 = (int32_t)(2*M_PI*w0);

                    //calc Q
                    _1000_Q = 707 + 1000*reso/128;

                    int32_t _2_pi_w0_delta_t;
                    int32_t Vhp_next;
                    int32_t Vbp_next;
                    int32_t Vlp_next;
                    int32_t Vbp;
                    int32_t Vlp;
                    int32_t Vhp;
                    int32_t Vi;
                    int32_t s;
                    int32_t delta_t;

                    Vbp = vfx->Vbp;
                    Vlp = vfx->Vlp;
                    Vhp = vfx->Vhp;
                    delta_t=8;
                    
                    // now let's throw our waveform through the resonator!
                    for (i=0; i<256; i++) {
                        // delta_t is converted to seconds given a 1MHz clock by dividing
                        // with 1 000 000. This is done in three operations to avoid integer
                        // multiplication overflow.
                        _2_pi_w0_delta_t = _2_pi_w0*delta_t/100;

                        // Calculate filter outputs.
                        Vi=sw1[i];
                        Vhp_next = Vbp*1000/_1000_Q - Vlp + Vi;
                        Vbp_next = Vbp - _2_pi_w0_delta_t*(Vhp/100)/100;
                        Vlp_next = Vlp - _2_pi_w0_delta_t*(Vbp/100)/100;
                        Vhp = Vhp_next;
                        Vbp = Vbp_next;
                        Vlp = Vlp_next;

                        s = Vhp;
                        if(s>32767)s=32767;
                        if(s<-32767)s=-32767;

                        dw[i] = (int16_t)(s);
                    }
                    vfx->Vbp = Vbp;
                    vfx->Vlp = Vlp;
                    vfx->Vhp = Vhp;
                }
                break;
            case 15: { // bandpass
                    int32_t dest,src1,src2;
                    int16_t *dw,*sw1,*sw2;
                    int16_t i,c;
                    int32_t _2_pi_w0;
                    int32_t _1000_Q;

                    dest = fx->dsteffect;
                    src1 = fx->srceffect1;
                    src2 = fx->osceffect-1;
                    dw = &vc->waves[256*dest];
                    sw1 = &vc->waves[256*src1];
                    sw2 = &vc->waves[256*src2];

                    c = (int16_t)vfx->osccnt;

    // init

                    int32_t freq,reso;
                    if(fx->osceffect==0) {
                        freq = fx->effectvar1;
                        reso = fx->effectvar2;
                        freq*=16; //(freq 0 - 16000hz)
                    } else {
                        if(fx->oscflg) {
                            freq = fx->effectvar1;
                            reso = (sw2[c]+32768)>>8;
                            freq*=16; //(freq 0 - 16000hz)
                        } else {
                            freq = (sw2[c]+32768)/16;
                            reso = fx->effectvar2;
                        }
                    }
                    //calc freq;
                    //double x = freq - 920.0;
                    //double w0 = 228 + 3900/2*(1 + tanh(_copysign(pow(fabs(x), 0.85)/95, x)));
                    double w0 = 228+freq;
                    _2_pi_w0 = (int32_t)(2*M_PI*w0);

                    //calc Q
                    _1000_Q = 707 + 1000*reso/128;

                    int32_t _2_pi_w0_delta_t;
                    int32_t Vhp_next;
                    int32_t Vbp_next;
                    int32_t Vlp_next;
                    int32_t Vbp;
                    int32_t Vlp;
                    int32_t Vhp;
                    int32_t Vi;
                    int32_t s;
                    int32_t delta_t;

                    Vbp = vfx->Vbp;
                    Vlp = vfx->Vlp;
                    Vhp = vfx->Vhp;
                    delta_t=8;
                    
                    // now let's throw our waveform through the resonator!
                    for (i=0; i<256; i++) {
                        // delta_t is converted to seconds given a 1MHz clock by dividing
                        // with 1 000 000. This is done in three operations to avoid integer
                        // multiplication overflow.
                        _2_pi_w0_delta_t = _2_pi_w0*delta_t/100;

                        // Calculate filter outputs.
                        Vi=sw1[i];
                        Vhp_next = Vbp*1000/_1000_Q - Vlp + Vi;
                        Vbp_next = Vbp - _2_pi_w0_delta_t*(Vhp/100)/100;
                        Vlp_next = Vlp - _2_pi_w0_delta_t*(Vbp/100)/100;
                        Vhp = Vhp_next;
                        Vbp = Vbp_next;
                        Vlp = Vlp_next;

                        s = Vbp;
                        if(s>32767)s=32767;
                        if(s<-32767)s=-32767;

                        dw[i] = (int16_t)(s);
                    }
                    vfx->Vbp = Vbp;
                    vfx->Vlp = Vlp;
                    vfx->Vhp = Vhp;
                }
                break;
            case 16: { // Noise
                
                    int32_t dest;
                    int16_t *dw;
                    int16_t i;
                    dest = fx->dsteffect;
                    dw = &vc->waves[256*dest];

                    for (i=0; i<256; i++) {
                        //TODO: LCG or twin LSFR noise
                        dw[i]=(rand()*2)-32768;
                    }
                }
                break;
            case 17: { // Squash
                    int32_t dest,src1,osc;
                    int16_t *dw,*sw1,*ow;
                    int16_t i,c;
                    dest = fx->dsteffect;
                    src1 = fx->srceffect1;
                    osc = fx->osceffect-1;
                    dw = &vc->waves[256*dest];
                    sw1 = &vc->waves[256*src1];
                    ow = &vc->waves[256*osc];

                    c = (int16_t)vfx->osccnt;

    // init
                    uint16_t m1a, m1b, m2;
                    if(fx->osceffect==0) {
                        m1a = fx->effectvar1;
                        m1b = fx->effectvar2;
                    } else {
                        if(fx->oscflg) {
                            m1a = fx->effectvar1;
                            m1b = (ow[c]+32768)/256;
                        } else {
                            m1a = (ow[c]+32768)/256;
                            m1b = fx->effectvar2;
                        }
                    }

                    m1b<<=8;
                    m1a+=m1b; //m1 is now the counter for the squash
                    m2=0;     //m2 is the actual counter which is 256 times too large (fixed point)
                    for (i=0; i<256; i++) {
                        int32_t a,b;
                        b=sw1[m2>>8];
                        a=sw1[(m2>>8)+1];
                        a*=(m2&255);
                        b*=(255-(m2&255));
                        a=(a>>8)+(b>>8);
                        dw[i] = a;
                        m2+=m1a;
                    }
                }
                break;
        }
    }
}

static void handleInstrument(JT1Player* SELF, int32_t channr) {
    JT1Voice* vc; JT1Inst* ins;
    int32_t vol, freq, pan;
    
    vc = &SELF->voices[channr];
    ins = SELF->song->instruments[vc->instrument];
    
    //vol
    if (ins->amwave == 0) { //volume wave?
        vol = 0;
    } else {
        vc->volcnt += ins->amspd;
        if (vc->volcnt >= 256) {
            vc->volcnt -= 256;
            vc->volcnt += ins->amlooppoint;
            if(vc->volcnt >= 256) {
                vc->volcnt = ins->amlooppoint;
            }
        }

        vol = vc->waves[(256*(ins->amwave-1))+vc->volcnt];
        vol = vol+32768;
        vol /= 6;
        vol = -vol;                //10930;
        if (vol <-10000) vol = -10000;
    }   

    //last but not least, the master volume
    vol += 10000;
    vol *= ins->mastervol;
    vol >>=8;
    vol *= SELF->masterVolume; //and the replayers master master volume
    vol >>=8;
    vol -= 10000;
    vc->curvol = vol;
    //if(vc->buf) vc->buf->SetVolume(vol);

    //update panning
    if(ins->panwave == 0) { //panning wave?
        pan = 0;
    } else {
        vc->pancnt += ins->panspd;
        if (vc->pancnt >= 256) {
            vc->pancnt -= 256;
            vc->pancnt += ins->panlooppoint;
            if(vc->pancnt >= 256) {
                vc->pancnt = ins->panlooppoint;
            }
        }

        pan = vc->waves[(256*(ins->panwave-1))+vc->pancnt];
        pan >>=7;
    }   
    //if(vc->buf) vc->buf->SetPan(pan);
    vc->curpan = pan;
    
    //update freq
    int32_t k;
    k = 0;
    k = SELF->song->arpTable[(ins->arpeggio*16)+vc->arpcnt];
    vc->arpcnt++;
    vc->arpcnt&=15;

    freq = frequencyTable[ins->finetune][k+vc->curnote]; 

    if(vc->freqdel) {
        vc->freqdel--;
    } else {
        if(ins->fmwave != 0) { //frequency wave?
            vc->freqcnt += ins->fmspd;
            if (vc->freqcnt >= 256) {
                vc->freqcnt -= 256;
                vc->freqcnt += ins->fmlooppoint;
                if(vc->freqcnt >= 256) {
                    vc->freqcnt = ins->fmlooppoint;
                }
            }
            freq -= vc->waves[(256*(ins->fmwave-1))+vc->freqcnt];
        }
    }
    freq += vc->bendadd;
    vc->curfreq = freq;
    
    //update pitchbend
    
    if(vc->bendspd != 0) {
        if(vc->bendspd >0) {
            if(vc->bendadd < vc->destfreq) {
                vc->bendadd += vc->bendspd;
                if(vc->bendadd > vc->destfreq) {
                    vc->bendadd = vc->destfreq;
                }
            }
        } else {
            if(vc->bendadd > vc->destfreq) {
                vc->bendadd += vc->bendspd;
                if(vc->bendadd < vc->destfreq) {
                    vc->bendadd = vc->destfreq;
                }
            }
        }
    }
}

static void playInstrument(JT1Player* SELF, int32_t channr, int32_t instNum, int32_t note) {
    JT1Voice* vc; JT1Inst* ins;
    int32_t f;
    
    // instruments init
    if(instNum > SELF->song->nrofinst) return; // not allowed!
    vc = &SELF->voices[channr];
    if(vc->instrument == -1 && instNum == 0) return; //geen instrument 0 op een gemute channel...er was namelijk geen previous instrument
    ins = SELF->song->instruments[instNum-1];
    
    vc->arpcnt = 0;
    vc->volcnt = 0;
    vc->pancnt = 0;
    vc->freqcnt = 0;
    vc->curnote = note;
    vc->curfreq = 0;
    vc->bendtonote = note;
    vc->bendadd = 0;
    vc->destfreq = 0;
    vc->bendspd = 0;

    if(instNum) { // do not copy if 0
        int32_t i;
        
        //TODO: check sample usage
        if(!ins->sharing) { // no sample sharing
            vc->sampledata = SELF->song->samples[instNum-1];
        } else {
            vc->sampledata = SELF->song->samples[ins->sharing-1];
        }
        vc->samplepos = ins->startpoint<<8;
        vc->looppoint = ins->looppoint<<8;
        vc->endpoint = ins->endpoint<<8;
        vc->loopflg = ins->loopflg;
        vc->bidirecflg = ins->bidirecflg;

        vc->freqdel = ins->fmdelay;
        for (i=0; i<SE_WAVES_INST; i++) {
            if (ins->resetwave[i]) memcpy(&vc->waves[i*256], &ins->waves[i*256], 256 * sizeof(int16_t));
        }
        vc->instrument = instNum-1;
    }
    
    // effects init
    for (f=0; f<SE_EFF_INST; f++) {
        JT1Effect* fx; JT1VoiceEffect* vfx;
        
        fx  = &SELF->song->instruments[vc->instrument]->fx[f];
        vfx = &vc->fx[f];
        if (fx->effecttype && fx->reseteffect) {
            vfx->osccnt = 0;
            vfx->fxcnt1 = 0;
            vfx->fxcnt2 = 0;
            vfx->y2     = 0;
            vfx->y1     = 0;
            vfx->Vhp    = 0;
            vfx->Vbp    = 0;
            vfx->Vlp    = 0;
        }
    }
}

static void handleSong(JT1Player* SELF) {
    int16_t i;
    int32_t step;
    
    if (!SELF->playFlg) return; 
    if (SELF->pauseFlg) return;
    
    SELF->patternDelay--;
    if (SELF->patternDelay==0) {
        step = SELF->playMode == SE_PM_SONG ? SELF->playStep : SELF->patternOffset;
        
        if ((step&1) == 0) { // change the groove
            SELF->playSpeed = 8 - SELF->subsong->groove;
        } else {
            SELF->playSpeed = 8 + SELF->subsong->groove;
        }
        SELF->patternDelay = SELF->playSpeed;
        
        if (SELF->playMode == SE_PM_PATTERN) {
            SELF->patternOffset++;
            SELF->patternOffset %= SELF->patternLength;
        } else {
            for (i=0; i < SELF->subsong->nrofchans; i++) {
                JT1Voice* vc = &SELF->voices[i];
                
                vc->patpos++;
                //the ==-1 part is that the song counter always is 1 before the start...so if the song starts at the beginning, the pos is -1
                if (vc->patpos == SELF->subsong->orders[i][vc->songpos].patlen || vc->songpos == -1) {
                    vc->patpos = 0;
                    vc->songpos++;
                }
            }
            
            SELF->playStep++;
            if (SELF->playStep==64) {
                SELF->playStep=0;
                SELF->playPosition++;
            }
            
            //has endpos been reached?
            if (SELF->playPosition == SELF->subsong->endpos && SELF->playStep == SELF->subsong->endstep) {
                if (SELF->subsong->songloop) {  //does song loop?
                    int32_t maat, pos, t;
                    uint8_t isSkipLoop = 0;
                    
                    // now me must reset all the playpointers to the loop positions
                    for (t=0; t<SE_NROFCHANS; t++) {
                        JT1Order* orders = SELF->subsong->orders[t];
                        JT1Voice* vc = &SELF->voices[t];
                        int32_t endpos;
                        int32_t lastmaat;
                        
                        maat = 0;
                        pos = 0;
                        lastmaat=0;
                        
                        endpos = (SELF->subsong->looppos * 64) + SELF->subsong->loopstep;
                        while (pos<256) {
                            if (maat > endpos) {
                                if (pos != endpos) pos--;
                                break;
                            }
                            lastmaat=maat;
                            maat+=orders[pos].patlen;
                            pos++;
                        }
                        //oops! starting position too far!
                        if (pos == 256) { //!WARN: >= 256?
                            SELF->playFlg = 0;
                            isSkipLoop = 1;
                            break;
                        }
                        
                        endpos -= lastmaat;
                        endpos &= 63;
                        
                        vc->songpos = pos;
                        vc->patpos  = endpos;
                    }
                    
                    if (!isSkipLoop) {
                        SELF->playPosition = SELF->subsong->looppos;
                        SELF->playStep     = SELF->subsong->loopstep;
                    }

                    SELF->loopCnt ++;
                } else { // stop song
                    SELF->playFlg  = 0;
                    SELF->pauseFlg = 0;
                    SELF->playMode = SE_PM_SONG;
                    SELF->playPosition = SELF->subsong->songpos;
                    SELF->playStep     = SELF->subsong->songstep;
                }
            }
        }
    }
}

static void handleScript(JT1Player* SELF, int32_t f,int32_t s, int32_t d, int32_t p, int32_t channr) { //note, script,dstnote,param,channr
    JT1Voice* vc; JT1Inst* ins;
    int32_t a;
    
    vc = &SELF->voices[channr];
    if(vc->instrument==-1) return; //no change
    ins = SELF->song->instruments[vc->instrument];
    
    switch(s) {
        default:
        case 0:
            return;
        case 1:  //pitch bend
            if (vc->bendtonote) { //hebben we al eens gebend?
                a = frequencyTable[ins->finetune][vc->bendtonote]; // begin frequentie
                vc->curnote = vc->bendtonote;
            } else {
                a = frequencyTable[ins->finetune][f]; // begin freqeuntie
            }
            vc->bendadd = 0;
            vc->destfreq = frequencyTable[ins->finetune][d] - a;
            vc->bendspd = p*20;
            vc->bendtonote = d;
            break;
        case 2:  //waveform
            if (d>15) d = 15;
            ins->waveform = d;
            break;
        case 3:  //wavelength
            d = (d>192 ? 256 : (d>96 ? 128 : (d>48 ? 64 : 32)));
            ins->wavelength = d;
            break;
        case 4:  //mastervol
            ins->mastervol = d;
            break;
        case 5:  //amwaveform
            if (d>15) d = 15;
            ins->amwave = d;
            break;
        case 6:  //amspd
            ins->amspd = d;
            break;
        case 7:  //amlooppoint
            ins->amlooppoint = d;
            break;
        case 8:  //finetune
            if (d>15) d = 15;
            ins->finetune = d;
            break;
        case 9:  //fmwaveform
            if (d>15) d = 15;
            ins->fmwave = d;
            break;
        case 10: //fmspd
            ins->fmspd = d;
            break;
        case 11: //fmlooppoint
            ins->fmlooppoint = d;
            break;
        case 12: //fmdelay
            ins->fmdelay = d;
            break;
        case 13: //arpeggio
            if (d>15) d = 15;
            ins->arpeggio = d;
            break;
            
        case 14: //fx 0 fxdstwave
            if (d>15) d = 15;
            ins->fx[0].dsteffect = d;
            break;
        case 15: //fx 0 fxsrcwave1
            if (d>15) d = 15;
            ins->fx[0].srceffect1 = d;
            break;
        case 16: //fx 0 fxsrcwave2
            if (d>15) d = 15;
            ins->fx[0].srceffect2 = d;
            break;
        case 17: //fx 0 fxoscwave
            if (d>15) d = 15;
            ins->fx[0].osceffect = d;
            break;
        case 18: //fx 0 effectvar1
            ins->fx[0].effectvar1 = d;
            break;
        case 19: //fx 0 effectvar2
            ins->fx[0].effectvar2 = d;
            break;
        case 20: //fx 0 effectspd
            ins->fx[0].effectspd = d;
            break;
        case 21: //fx 0 oscspd
            ins->fx[0].oscspd = d;
            break;
        case 22: //fx 0 oscflg
            if (d>1) d=1;
            ins->fx[0].oscflg = d;
            break;
        case 23: //fx 0 effecttype
            if (d>=SE_NROFEFFECTS) d=SE_NROFEFFECTS-1;
            ins->fx[0].effecttype = d;
            break;
        case 24: //fx 0 reseteffect
            if (d>1) d=1;
            ins->fx[0].reseteffect = d;
            break;
            
        case 25: //fx 1 fxdstwave
            if (d>15) d = 15;
            ins->fx[1].dsteffect = d;
            break;
        case 26: //fx 1 fxsrcwave1
            if (d>15) d = 15;
            ins->fx[1].srceffect1 = d;
            break;
        case 27: //fx 1 fxsrcwave2
            if (d>15) d = 15;
            ins->fx[1].srceffect2 = d;
            break;
        case 28: //fx 1 fxoscwave
            if (d>15) d = 15;
            ins->fx[1].osceffect = d;
            break;
        case 29: //fx 1 effectvar1
            ins->fx[1].effectvar1 = d;
            break;
        case 30: //fx 1 effectvar2
            ins->fx[1].effectvar2 = d;
            break;
        case 31: //fx 1 effectspd
            ins->fx[1].effectspd = d;
            break;
        case 32: //fx 1 oscspd
            ins->fx[1].oscspd = d;
            break;
        case 33: //fx 1 oscflg
            if (d>1) d=1;
            ins->fx[1].oscflg = d;
            break;
        case 34: //fx 1 effecttype
            if (d>=SE_NROFEFFECTS) d=SE_NROFEFFECTS-1;
            ins->fx[1].effecttype = d;
            break;
        case 35: //fx 1 reseteffect
            if (d>1) d=1;
            ins->fx[1].reseteffect = d;
            break;
            
        case 36: //fx 2 fxdstwave
            if (d>15) d = 15;
            ins->fx[2].dsteffect = d;
            break;
        case 37: //fx 2 fxsrcwave1
            if (d>15) d = 15;
            ins->fx[2].srceffect1 = d;
            break;
        case 38: //fx 2 fxsrcwave2
            if (d>15) d = 15;
            ins->fx[2].srceffect2 = d;
            break;
        case 39: //fx 2 fxoscwave
            if (d>15) d = 15;
            ins->fx[2].osceffect = d;
            break;
        case 40: //fx 2 effectvar1
            ins->fx[2].effectvar1 = d;
            break;
        case 41: //fx 2 effectvar2
            ins->fx[2].effectvar2 = d;
            break;
        case 42: //fx 2 effectspd
            ins->fx[2].effectspd = d;
            break;
        case 43: //fx 2 oscspd
            ins->fx[2].oscspd = d;
            break;
        case 44: //fx 2 oscflg
            if (d>1) d=1;
            ins->fx[2].oscflg = d;
            break;
        case 45: //fx 2 effecttype
            if (d>=SE_NROFEFFECTS) d=SE_NROFEFFECTS-1;
            ins->fx[2].effecttype = d;
            break;
        case 46: //fx 2 reseteffect
            if (d>1) d=1;
            ins->fx[2].reseteffect = d;
            break;
            
        case 47: //fx 3 fxdstwave
            if (d>15) d = 15;
            ins->fx[3].dsteffect = d;
            break;
        case 48: //fx 3 fxsrcwave1
            if (d>15) d = 15;
            ins->fx[3].srceffect1 = d;
            break;
        case 49: //fx 3 fxsrcwave2
            if (d>15) d = 15;
            ins->fx[3].srceffect2 = d;
            break;
        case 50: //fx 3 fxoscwave
            if (d>15) d = 15;
            ins->fx[3].osceffect = d;
            break;
        case 51: //fx 3 effectvar1
            ins->fx[3].effectvar1 = d;
            break;
        case 52: //fx 3 effectvar2
            ins->fx[3].effectvar2 = d;
            break;
        case 53: //fx 3 effectspd
            ins->fx[3].effectspd = d;
            break;
        case 54: //fx 3 oscspd
            ins->fx[3].oscspd = d;
            break;
        case 55: //fx 3 oscflg
            if (d>1) d=1;
            ins->fx[3].oscflg = d;
            break;
        case 56: //fx 3 effecttype
            if (d>=SE_NROFEFFECTS) d=SE_NROFEFFECTS-1;
            ins->fx[3].effecttype = d;
            break;
        case 57: //fx 3 reseteffect
            if (d>1) d=1;
            ins->fx[3].reseteffect = d;
            break;
            
        case 58: //resetwave 00
            if (d>1) d=1;
            ins->resetwave[0] = d;
            break;
        case 59: //resetwave 01
            if (d>1) d=1;
            ins->resetwave[1] = d;
            break;
        case 60: //resetwave 02
            if (d>1) d=1;
            ins->resetwave[2] = d;
            break;
        case 61: //resetwave 03
            if (d>1) d=1;
            ins->resetwave[3] = d;
            break;
        case 62: //resetwave 04
            if (d>1) d=1;
            ins->resetwave[4] = d;
            break;
        case 63: //resetwave 05
            if (d>1) d=1;
            ins->resetwave[5] = d;
            break;
        case 64: //resetwave 06
            if (d>1) d=1;
            ins->resetwave[6] = d;
            break;
        case 65: //resetwave 07
            if (d>1) d=1;
            ins->resetwave[7] = d;
            break;
        case 66: //resetwave 08
            if (d>1) d=1;
            ins->resetwave[8] = d;
            break;
        case 67: //resetwave 09
            if (d>1) d=1;
            ins->resetwave[9] = d;
            break;
        case 68: //resetwave 10
            if (d>1) d=1;
            ins->resetwave[10] = d;
            break;
        case 69: //resetwave 11
            if (d>1) d=1;
            ins->resetwave[11] = d;
            break;
        case 70: //resetwave 12
            if (d>1) d=1;
            ins->resetwave[12] = d;
            break;
        case 71: //resetwave 13
            if (d>1) d=1;
            ins->resetwave[13] = d;
            break;
        case 72: //resetwave 14
            if (d>1) d=1;
            ins->resetwave[14] = d;
            break;
        case 73: //resetwave 15
            if (d>1) d=1;
            ins->resetwave[15] = d;
            break;
            
        case 74: //Change bpm
            if (d<=10) d=10;
            if (d>220) d=220;
            SELF->subsong->songspd = d;

            float t;
            t  = (float)SELF->subsong->songspd; //bpm
            t /=60.0;         //bps
            t *=32.0;
            SELF->timeSpd = (int32_t)(44100.0/t);
            break;
        case 75: //Change Groove
            if (d>3) d=3;
            SELF->subsong->groove = d;
            break;
        case 76: //Fire External Event
            //TODO: add this effect
            break;
    }
}

static void handlePattern(JT1Player* SELF, int32_t channr) {
    JT1Voice* vc; JT1Row* row;
    int32_t pat,off;
    int32_t f,d,s,p;
    
    if (SELF->pauseFlg) return;
    if (!SELF->playFlg) return; 
    if (SELF->patternDelay != SELF->playSpeed) return;
    
    vc = &SELF->voices[channr];
    
    if (SELF->playMode == SE_PM_PATTERN) {
        if (channr > 0) return;  // just play channel 0
        pat = SELF->currentPattern;
        off = SELF->patternOffset;
    } else {
        if (SELF->subsong->mute[channr]) return;
        off = vc->patpos;
        pat = SELF->subsong->orders[channr][vc->songpos].patnr;
    }

    row = &SELF->song->patterns[(pat*64)+off];
    //init instrument
    f = row->srcnote;
    if (f) playInstrument(SELF, channr, row->inst, f);

    //handle special effects
    s = row->script;
    d = row->dstnote;
    p = row->param;
    handleScript(SELF, f, s, d, p, channr);
}

static void advanceSong(JT1Player* SELF) {
    handleSong(SELF);
    for (int i=0; i < SELF->subsong->nrofchans; i++) {
        handlePattern(SELF, i);
        if(SELF->voices[i].instrument != -1) {// mute?
            handleInstrument(SELF, i); //do volume and pitch things
            handleEffects(SELF, i);    //do instrument effects 
        }
    }
}

static void PlayPattern(JT1Player* SELF, int PatternNr) {
    SELF->playFlg = 1;
    SELF->currentPattern = PatternNr;
    SELF->patternOffset = 63;
    SELF->patternDelay = 1;
    SELF->playMode = SE_PM_PATTERN;
    SELF->playSpeed = SELF->song->subsongs[0]->songspd - SELF->song->subsongs[0]->groove;
}

static void clearSoundBuffers(JT1Player* SELF) {
    int32_t i,j;

    // clear delaybuffers
    memset(SELF->overlapBuffer,    0, WANTEDOVERLAP*2*sizeof(int16_t));
    memset(SELF->leftDelayBuffer,  0, 65536*sizeof(int16_t));
    memset(SELF->rightDelayBuffer, 0, 65536*sizeof(int16_t));

    //initialize channel data
    for (i=0;i<SE_NROFCHANS;i++) {
        JT1Voice* vc = &SELF->voices[i];
        
        vc->songpos = 0;
        vc->patpos = 0;
        vc->instrument = -1;
        vc->volcnt = 0;
        vc->arpcnt = 0;
        vc->pancnt = 0;
        vc->curnote = 0;
        vc->curfreq = 0;
        vc->bendadd = 0;
        vc->destfreq = 0;
        vc->bendspd = 0;
        vc->freqcnt = 0;
        vc->freqdel = 0;
        vc->sampledata = NULL;
        vc->endpoint = 0;
        vc->synthPos = 0;
        vc->samplepos = 0;
        vc->curvol = 0;
        vc->curpan = 0;
        vc->bendtonote = 0;
        vc->looppoint = 0;
        vc->loopflg = 0;
        vc->bidirecflg = 0;
        
        vc->wavePtr     = NULL;
        vc->waveLength  = 0;
        vc->freqOffset  = 0;
        vc->gainMainL   = 0;
        vc->gainMainR   = 0;
        vc->gainEchoL   = 0;
        vc->gainEchoR   = 0;
        
        for(j=0;j<4;j++) {
            JT1VoiceEffect* vfx = &vc->fx[j];
            
            vfx->fxcnt1 = 0;
            vfx->fxcnt2 = 0;
            vfx->osccnt = 0;
            vfx->a0 = 0;
            vfx->b1 = 0;
            vfx->b2 = 0;
            vfx->y1 = 0;
            vfx->y2 = 0;
            vfx->Vhp = 0;
            vfx->Vbp = 0;
            vfx->Vlp = 0;
        }
        memset(vc->waves, 0, 4096*sizeof(int16_t));
    }
}

//---------------------API

int jaytrax_loadSong(JT1Player* SELF, JT1Song* sng) {
    SELF->song = sng;
    jaytrax_changeSubsong(SELF, 0);
    return 1;
}

// This function ensures that the play routine is called properly and everything is initialized in a good way
void jaytrax_changeSubsong(JT1Player* SELF, int subsongnr) {
    int maat, pos, t;
    JT1Order* order;

    if (subsongnr > SELF->song->nrofsongs) return;
    SELF->subsongNr = subsongnr;
    SELF->subsong = SELF->song->subsongs[subsongnr];
    clearSoundBuffers(SELF);

    for(t=0; t < SE_NROFCHANS; t++) {
        JT1Voice* vc;
        int endpos, lastmaat;
        
        vc = &SELF->voices[t];
        
        maat = pos = lastmaat = 0;
        order = SELF->subsong->orders[t];
        endpos = (SELF->subsong->songpos * 64) + SELF->subsong->songstep - 1; //minus 1 because we immediately start with the new note
        while (pos<256) {
            if (maat >= endpos) {
                if(pos != endpos) pos--;
                break;
            }
            lastmaat=maat;
            maat+=order[pos].patlen;
            pos++;
        }
        //oops! starting position too far!
        if (pos == 256) return; //!WARN: >= 256?

        endpos-=lastmaat;
        //endpos-=maat;
        endpos &=63;
        
        vc->songpos = pos;
        vc->patpos  = endpos;
    }

    SELF->patternDelay = 1;
    SELF->playFlg = 1;
    SELF->pauseFlg = 0;
    SELF->loopCnt = 0;
    SELF->playSpeed = 8 + SELF->subsong->groove;
    //SELF->playSpeed = 8;

    if (SELF->subsong->songspd != 0) {
        float t;
        t  = (float)SELF->subsong->songspd; //bpm
        t /= 60.0;                    //bps
        t *= 32.0;
        SELF->timeCnt = SELF->timeSpd = (int)(44100.0/t);
    }

    if(SELF->subsong->songstep == 0) {
        SELF->playPosition = SELF->subsong->songpos - 1;
    } else {
        SELF->playPosition = SELF->subsong->songpos;
    }
    SELF->playStep  = SELF->subsong->songstep - 1;
    SELF->playStep &= 63;
}

void jaytrax_stopSong(JT1Player* SELF) {
    SELF->playFlg  = 0;
    SELF->pauseFlg = 0;
    SELF->playMode = SE_PM_SONG;
    if(SELF->song) {
        SELF->playPosition = SELF->subsong->songpos;
        SELF->playStep     = SELF->subsong->songstep;
    }
}

void jaytrax_pauseSong(JT1Player* SELF) {
    SELF->pauseFlg = 1;
}

void jaytrax_continueSong(JT1Player* SELF) {
    SELF->pauseFlg = 0;
}

JT1Player* jaytrax_init(void) {
    JT1Player* SELF = calloc(1, sizeof(JT1Player));
    //lazy static init
    if (!isStaticInit) {
        int32_t i, j;
        double f,y;
        //freq and finetune table
        y=2;
        for (j=0;j<SE_NROFFINETUNESTEPS;j++) {
            for (i=0;i<128;i++) {
                // formule = wortel(x/12)
                f=((i+3)*16)-j;   // we beginnen met de a want die is 440hz
                f=f/192;
                f=pow(y,f);
                f=f*220;
                frequencyTable[j][i]=(int32_t)(f+0.5);
            }
        }
        
        //sine table
        for (i=0;i<256;i++) {
            sineTab[i] = (int16_t)(sin((M_PI*i)/128)*32760);
        }
        
        isStaticInit = 1;
    }
    
    //init instance
    SELF->overlapCnt = 0;
    
    SELF->playPosition = 0;  // waar is de song nu?
    SELF->playStep = 0;
    SELF->subsongNr = 0; 
    SELF->playFlg = 0; 
    SELF->pauseFlg = 0;
    SELF->loopCnt = 0;

    SELF->patternDelay = 0;
    SELF->playSpeed = 0;
    SELF->masterVolume = 256;
    SELF->playMode = SE_PM_SONG;
    jaymix_setInterp(&SELF->itp, ITP_CUBIC);
    SELF->song = NULL;
    SELF->subsong = NULL;

    //initialize rendering counters and speed
    SELF->timeCnt = 2200;
    SELF->timeSpd = 2200;
    
    clearSoundBuffers(SELF);
    SELF->delayCnt=0;
    return SELF;
}

void jaytrax_free(JT1Player* SELF) {
    free(SELF);
}

void jaytrax_setInterpolation(JT1Player* SELF, uint8_t id) {
    jaymix_setInterp(&SELF->itp, id);
}

void jaytrax_renderChunk(JT1Player* SELF, int16_t* outbuf, int32_t nrofsamples, int32_t frequency) {
    int16_t ic, is;
    int32_t r;
    int16_t amplification;                          // amount to amplify afterwards
    uint16_t echodelaytime;                 //delaytime for echo (differs for MIDI or songplayback)
    int16_t chanNr;
    // we calc nrofsamples samples in blocks of 'timeCnt' big (de songspd)
    
    r = 0;
    echodelaytime = amplification = 0;
    while (nrofsamples > 0) {
        int32_t availOvlap, frameLen;
        int16_t nos;
        
        frameLen = (SELF->timeSpd * frequency) / 44100;
        availOvlap = MIN(WANTEDOVERLAP, frameLen);
        if (SELF->timeCnt<nrofsamples) {
            nos = SELF->timeCnt;   //Complete block
            SELF->timeCnt = frameLen;
        } else {
            nos = nrofsamples;  //Last piece
            SELF->timeCnt = SELF->timeCnt - nos; 
        }
        nrofsamples-=nos;
        
        if (!outbuf) {
            //times two for stereo
            r += (nos*2);
        } else {
            if (!SELF->song || !SELF->subsong || SELF->subsong->nrofchans == 0) {
                for(is=0; is < nos; is++) { //clean renderbuffer
                    outbuf[r++] = 0;
                    outbuf[r++] = 0;
                }
            } else {
                chanNr = SELF->subsong->nrofchans;
                
                //preparation of wave pointers and freq offset
                for(ic=0; ic < chanNr; ic++) {
                    JT1Voice* vc;
                    int16_t instnr;
                    int16_t volMain, volEcho;
                    
                    vc = &SELF->voices[ic];
                    instnr = vc->instrument;
                    if (instnr == -1) { // mute?
                        vc->wavePtr  = NULL;
                    } else {
                        if(vc->sampledata) {
                            vc->wavePtr  = (int16_t*)vc->sampledata;
                        } else {
                            JT1Inst* inst = SELF->song->instruments[instnr];
                            vc->wavePtr    = &vc->waves[256*inst->waveform];
                            vc->waveLength = ((inst->wavelength-1)<<8)+255;  //fixed point 8 bit (last 8 bits should be set)
                        }
                    }
                    
                    //calculate frequency
                    if (vc->curfreq < 10) vc->curfreq = 10;
                    vc->freqOffset = (256*vc->curfreq)/frequency * (vc->freqOffset<0 ? -1 : 1);
                    
                    if (vc->curpan == 0) { //panning?
                        vc->gainMainL = 256; //center
                        vc->gainMainR = 256;
                    } else {
                        if (vc->curpan > 0) {
                            vc->gainMainL = 256-(vc->curpan);
                            vc->gainMainR = 256;
                        } else {
                            vc->gainMainL = 256;
                            vc->gainMainR = 256+(vc->curpan);
                        }
                    }
                    
                    //gains
                    volMain = (vc->curvol+10000)/39;
                    if (volMain > 256) volMain = 256;
                    volEcho = SELF->subsong->delayamount[ic];
                    
                    //premultiply volumes
                    vc->gainMainL = (vc->gainMainL * volMain)>>8;
                    vc->gainMainR = (vc->gainMainR * volMain)>>8;
                    vc->gainEchoL = (vc->gainMainL * volEcho)>>8;
                    vc->gainEchoR = (vc->gainMainR * volEcho)>>8;
                }
                amplification = SELF->subsong->amplification;
                echodelaytime = SELF->subsong->delaytime;
                
                //main render
                while (nos > 0) {
                    int16_t morenos  = MIN(nos, MIXBUF_LEN);
                    int16_t* overBuf = &SELF->overlapBuffer[0];
                    int16_t* delLBuf = &SELF->leftDelayBuffer[0];
                    int16_t* delRBuf = &SELF->rightDelayBuffer[0];
                    int32_t* tempBuf = &SELF->tempBuf[0];
                    
                    
                    jaymix_mixCore(SELF, morenos);
                    
                    for(is=0; is < morenos; is++) {
                        int32_t lsample, rsample, echosamplel, echosampler;
                        int32_t off = MIXBUF_NR * is;
                        int32_t ocnt = SELF->overlapCnt;
                        int32_t delcnt = SELF->delayCnt;
                        
                        lsample     = tempBuf[off + BUF_MAINL];
                        rsample     = tempBuf[off + BUF_MAINR];
                        echosamplel = tempBuf[off + BUF_ECHOL];
                        echosampler = tempBuf[off + BUF_ECHOR];
                        
                        lsample  = ((lsample / chanNr) + delLBuf[delcnt]) / 2;
                        lsample *= amplification;
                        lsample /= 100;
                        CLAMP(lsample, -32760, 32760);
                        
                        rsample  = ((rsample / chanNr) + delRBuf[delcnt]) / 2;
                        rsample *= amplification;
                        rsample /= 100;
                        CLAMP(rsample, -32760, 32760);
                        
                        //interpolate from overlap buffer
                        if(ocnt < availOvlap) {
                            int32_t lovlapsamp, rovlapsamp;
                            lovlapsamp = overBuf[ocnt*2+0];
                            rovlapsamp = overBuf[ocnt*2+1];
                            lsample  = ((lsample * (ocnt)) / availOvlap) + ((lovlapsamp * (availOvlap - ocnt)) / availOvlap);
                            rsample  = ((rsample * (ocnt)) / availOvlap) + ((rovlapsamp * (availOvlap - ocnt)) / availOvlap);
                            SELF->overlapCnt++;
                        }
                        
                        outbuf[r++] = lsample;
                        outbuf[r++] = rsample;
                        
                        delLBuf[delcnt] = ((echosamplel / chanNr) + delLBuf[delcnt]) / 2;
                        delRBuf[delcnt] = ((echosampler / chanNr) + delRBuf[delcnt]) / 2;
                        SELF->delayCnt++;
                        SELF->delayCnt %= echodelaytime / (44100 / frequency);
                    }
                    
                    nos -= morenos;
                }
            }
        }
        
        if(SELF->timeCnt == frameLen) {
            int32_t tempdelaycnt;
            struct {
                int32_t synthPos;
                int32_t sampPos;
                int32_t freqOffset;
            } temp[SE_NROFCHANS];
            
            tempdelaycnt = SELF->delayCnt;
            for(ic=0; ic < SE_NROFCHANS; ic++) {
                JT1Voice* vc = &SELF->voices[ic];
                
                temp[ic].synthPos      = vc->synthPos;
                temp[ic].sampPos       = vc->samplepos;
                temp[ic].freqOffset    = vc->freqOffset;
            }
            
            if (outbuf && SELF->song && SELF->subsong && SELF->subsong->nrofchans != 0) {
                int16_t nos2 = availOvlap;
                chanNr = SELF->subsong->nrofchans;
                
                //render to overlap buffer
                assert(availOvlap - SELF->overlapCnt == 0);
                while (nos2 > 0) {
                    int16_t morenos  = MIN(nos2, MIXBUF_LEN);
                    int16_t* overBuf = &SELF->overlapBuffer[0];
                    int16_t* delLBuf = &SELF->leftDelayBuffer[0];
                    int16_t* delRBuf = &SELF->rightDelayBuffer[0];
                    int32_t* tempBuf = &SELF->tempBuf[0];
                    
                    
                    jaymix_mixCore(SELF, morenos);
                    
                    for(is=0; is < morenos; is++) {
                        int32_t lsample, rsample;
                        int32_t off = MIXBUF_NR * is;
                        int32_t ocnt = SELF->overlapCnt;
                        int32_t delcnt = SELF->delayCnt;
                        
                        lsample  = tempBuf[off + BUF_MAINL];
                        rsample  = tempBuf[off + BUF_MAINR];
                        
                        lsample  = ((lsample / chanNr) + delLBuf[delcnt]) / 2;
                        lsample *= amplification;
                        lsample /= 100;
                        CLAMP(lsample, -32760, 32760);
                        
                        rsample  = ((rsample / chanNr) + delRBuf[delcnt]) / 2;
                        rsample *= amplification;
                        rsample /= 100;
                        CLAMP(rsample, -32760, 32760);
                        
                        overBuf[(availOvlap - ocnt)*2+0] = lsample;
                        overBuf[(availOvlap - ocnt)*2+1] = rsample;
                        
                        SELF->delayCnt++;
                        SELF->delayCnt %= echodelaytime / (44100 / frequency);
                        SELF->overlapCnt--;
                    }
                    nos2 -= morenos;
                }
                assert(SELF->overlapCnt == 0);
            }
            
            SELF->delayCnt = tempdelaycnt;
            
            for(ic=0; ic < SE_NROFCHANS; ic++) {
                JT1Voice* vc = &SELF->voices[ic];
                
                vc->synthPos    = temp[ic].synthPos;
                vc->samplepos   = temp[ic].sampPos;
                vc->freqOffset  = temp[ic].freqOffset;
            }
            
            //Update song pointers
            advanceSong(SELF);
        }
    }
}

int32_t jaytrax_getLength(JT1Player* SELF, int subsongnr, int loopCnt, int frequency) {
    int32_t length = 0;
    int32_t lengthMax = frequency * 60 * 30;
    int32_t frameLen;
    jaytrax_changeSubsong(SELF, subsongnr);
    while (SELF->playFlg && SELF->loopCnt < loopCnt && length < lengthMax) {
        frameLen = (SELF->timeSpd * frequency) / 44100;
        advanceSong(SELF);
        length += frameLen;
    }
    return length < lengthMax ? length : -1;
}
