#ifndef _SPLAY_WINDOW_H_
#define _SPLAY_WINDOW_H_

extern ATTR_ALIGN(64) REAL win[4][36];
extern ATTR_ALIGN(64) REAL winINV[4][36];

inline REAL* getSplayWindow(int nr) { return win[nr]; }
inline REAL* getSplayWindowINV(int nr) { return winINV[nr]; }

#endif
