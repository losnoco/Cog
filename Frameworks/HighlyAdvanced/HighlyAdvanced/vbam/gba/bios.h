#ifndef BIOS_H
#define BIOS_H

extern void BIOS_ArcTan(GBASystem *);
extern void BIOS_ArcTan2(GBASystem *);
extern void BIOS_BitUnPack(GBASystem *);
extern void BIOS_GetBiosChecksum(GBASystem *);
extern void BIOS_BgAffineSet(GBASystem *);
extern void BIOS_CpuSet(GBASystem *);
extern void BIOS_CpuFastSet(GBASystem *);
extern void BIOS_Diff8bitUnFilterWram(GBASystem *);
extern void BIOS_Diff8bitUnFilterVram(GBASystem *);
extern void BIOS_Diff16bitUnFilter(GBASystem *);
extern void BIOS_Div(GBASystem *);
extern void BIOS_DivARM(GBASystem *);
extern void BIOS_HuffUnComp(GBASystem *);
extern void BIOS_LZ77UnCompVram(GBASystem *);
extern void BIOS_LZ77UnCompWram(GBASystem *);
extern void BIOS_ObjAffineSet(GBASystem *);
extern void BIOS_RegisterRamReset(GBASystem *);
extern void BIOS_RegisterRamReset(GBASystem *, u32);
extern void BIOS_RLUnCompVram(GBASystem *);
extern void BIOS_RLUnCompWram(GBASystem *);
extern void BIOS_SoftReset(GBASystem *);
extern void BIOS_Sqrt(GBASystem *);
extern void BIOS_MidiKey2Freq(GBASystem *);
extern void BIOS_SndDriverJmpTableCopy(GBASystem *);

#endif // BIOS_H
