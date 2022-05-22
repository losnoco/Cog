//
//  cqt.h
//  CogAudio Framework
//
//  Created by Christopher Snowhill on 5/21/22.
//

#ifndef cqt_h
#define cqt_h

#ifdef __cplusplus
extern "C" {
#endif

void cqt_calculate(const float *data, const float sampleRate, float *freq, int samples_in);

void cqt_free(void);

#ifdef __cplusplus
}
#endif

#endif /* cqt_h */
