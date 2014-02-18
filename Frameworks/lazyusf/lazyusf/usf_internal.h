#ifndef _USF_INTERNAL_H_
#define _USF_INTERNAL_H_

struct usf_state_helper
{
    size_t offset_to_structure;
};

#ifndef RCPREG_DEFINED
#define RCPREG_DEFINED
typedef uint32_t RCPREG;
#endif

struct usf_state
{
    // RSP vector registers, need to be aligned to 16 bytes
    // when SSE2 or SSSE3 is enabled, or for any hope of
    // auto vectorization

    // usf_clear takes care of aligning the structure within
    // the memory block passed into it, treating the pointer
    // as usf_state_helper, and storing an offset from the
    // pointer to the actual usf_state structure. The size
    // which is indicated for allocation accounts for this
    // with two pages of padding.

    short VR[32][8];
    short VACC[3][8];
    
    // RSP virtual registers, also needs alignment
    int SR[32];
    
    // rsp/rsp.c, not necessarily in need of alignment
    RCPREG* CR[16];
    
    // rsp/vu/cf.h, all need alignment
    short ne[8]; /* $vco:  high byte "NOTEQUAL" */
    short co[8]; /* $vco:  low byte "carry/borrow in/out" */
    short clip[8]; /* $vcc:  high byte (clip tests:  VCL, VCH, VCR) */
    short comp[8]; /* $vcc:  low byte (VEQ, VNE, VLT, VGE, VCL, VCH, VCR) */
    short vce[8]; /* $vce:  vector compare extension register */
    
    // All further members of the structure need not be aligned

    // rsp/vu/divrom.h
    int DivIn; /* buffered numerator of division read from vector file */
    int DivOut; /* global division result set by VRCP/VRCPL/VRSQ/VRSQH */
#if (0)
    int MovIn; /* We do not emulate this register (obsolete, for VMOV). */
#endif
    
    int DPH;
    
    // rsp/rsp.h
    int temp_PC;
    short MFC0_count[32];
    
    uint32_t cpu_running, cpu_stopped;
    
    // options from file tags
    uint32_t enablecompare, enableFIFOfull;
    
    // buffering for rendered sample data
    size_t sample_buffer_count;
    int16_t * sample_buffer;

    // audio.c
    int32_t SampleRate;
    int16_t samplebuf[16384];
    size_t samples_in_buffer;
    
    // cpu.c
    uint32_t NextInstruction, JumpToLocation, AudioIntrReg;
    CPU_ACTION * CPU_Action;
    SYSTEM_TIMERS * Timers;
    OPCODE Opcode;
    uint32_t CPURunning, SPHack;
    uint32_t * WaitMode;
    
    // interpreter_ops.c
    uint32_t SWL_MASK[4], SWR_MASK[4], LWL_MASK[4], LWR_MASK[4];
    int32_t SWL_SHIFT[4], SWR_SHIFT[4], LWL_SHIFT[4], LWR_SHIFT[4];
    int32_t RoundingModel;

    // memory.c
    uintptr_t *TLB_Map;
    uint8_t * MemChunk;
    uint32_t RdramSize, SystemRdramSize, RomFileSize;
    uint8_t * N64MEM, * RDRAM, * DMEM, * IMEM, * ROMPages[0x400], * savestatespace, * NOMEM;
    
    uint32_t WrittenToRom;
    uint32_t WroteToRom;
    uint32_t TempValue;
    uint32_t MemoryState;
    
    uint8_t EmptySpace;
    
    // pif.c
    uint8_t *PIF_Ram;
    
    // registers.c
    uint32_t PROGRAM_COUNTER, * CP0,*FPCR,*RegRDRAM,*RegSP,*RegDPC,*RegMI,*RegVI,*RegAI,*RegPI,
	*RegRI,*RegSI, HalfLine, RegModValue, ViFieldNumber, LLBit, LLAddr;
    void * FPRDoubleLocation[32], * FPRFloatLocation[32];
    MIPS_DWORD *GPR, *FPR, HI, LO;
    int32_t fpuControl;
    N64_REGISTERS * Registers;
    
    // tlb.c
    FASTTLB FastTlb[64];
    TLB tlb[32];
};

#define USF_STATE_HELPER ((usf_state_helper_t *)(state))

#define USF_STATE ((usf_state_t *)(((uint8_t *)(state))+((usf_state_helper_t *)(state))->offset_to_structure))

#endif
