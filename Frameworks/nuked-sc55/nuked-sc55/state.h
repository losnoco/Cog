#pragma once

#include <stdint.h>

#include "api.h"

enum {
    DEV_P1DDR = 0x00,
    DEV_P5DDR = 0x08,
    DEV_P6DDR = 0x09,
    DEV_P7DDR = 0x0c,
    DEV_P7DR = 0x0e,
    DEV_FRT1_TCR = 0x10,
    DEV_FRT1_TCSR = 0x11,
    DEV_FRT1_FRCH = 0x12,
    DEV_FRT1_FRCL = 0x13,
    DEV_FRT1_OCRAH = 0x14,
    DEV_FRT1_OCRAL = 0x15,
    DEV_FRT2_TCR = 0x20,
    DEV_FRT2_TCSR = 0x21,
    DEV_FRT2_FRCH = 0x22,
    DEV_FRT2_FRCL = 0x23,
    DEV_FRT2_OCRAH = 0x24,
    DEV_FRT2_OCRAL = 0x25,
    DEV_FRT3_TCR = 0x30,
    DEV_FRT3_TCSR = 0x31,
    DEV_FRT3_FRCH = 0x32,
    DEV_FRT3_FRCL = 0x33,
    DEV_FRT3_OCRAH = 0x34,
    DEV_FRT3_OCRAL = 0x35,
    DEV_PWM1_TCR = 0x40,
    DEV_PWM1_DTR = 0x41,
    DEV_PWM2_TCR = 0x44,
    DEV_PWM2_DTR = 0x45,
    DEV_PWM3_TCR = 0x48,
    DEV_PWM3_DTR = 0x49,
    DEV_TMR_TCR = 0x50,
    DEV_TMR_TCSR = 0x51,
    DEV_TMR_TCORA = 0x52,
    DEV_TMR_TCORB = 0x53,
    DEV_TMR_TCNT = 0x54,
    DEV_SMR = 0x58,
    DEV_BRR = 0x59,
    DEV_SCR = 0x5a,
    DEV_TDR = 0x5b,
    DEV_SSR = 0x5c,
    DEV_RDR = 0x5d,
    DEV_ADDRAH = 0x60,
    DEV_ADDRAL = 0x61,
    DEV_ADDRBH = 0x62,
    DEV_ADDRBL = 0x63,
    DEV_ADDRCH = 0x64,
    DEV_ADDRCL = 0x65,
    DEV_ADDRDH = 0x66,
    DEV_ADDRDL = 0x67,
    DEV_ADCSR = 0x68,
    DEV_IPRA = 0x70,
    DEV_IPRB = 0x71,
    DEV_IPRC = 0x72,
    DEV_IPRD = 0x73,
    DEV_DTEA = 0x74,
    DEV_DTEB = 0x75,
    DEV_DTEC = 0x76,
    DEV_DTED = 0x77,
    DEV_WCR = 0x78,
    DEV_RAME = 0x79,
    DEV_P1CR = 0x7c,
    DEV_P9DDR = 0x7e,
    DEV_P9DR = 0x7f,
};

const uint16_t sr_mask = 0x870f;
enum {
    STATUS_T = 0x8000,
    STATUS_N = 0x08,
    STATUS_Z = 0x04,
    STATUS_V = 0x02,
    STATUS_C = 0x01,
    STATUS_INT_MASK = 0x700
};

enum {
    VECTOR_RESET = 0,
    VECTOR_RESERVED1, // UNUSED
    VECTOR_INVALID_INSTRUCTION,
    VECTOR_DIVZERO,
    VECTOR_TRAP,
    VECTOR_RESERVED2, // UNUSED
    VECTOR_RESERVED3, // UNUSED
    VECTOR_RESERVED4, // UNUSED
    VECTOR_ADDRESS_ERROR,
    VECTOR_TRACE,
    VECTOR_RESERVED5, // UNUSED
    VECTOR_NMI,
    VECTOR_RESERVED6, // UNUSED
    VECTOR_RESERVED7, // UNUSED
    VECTOR_RESERVED8, // UNUSED
    VECTOR_RESERVED9, // UNUSED
    VECTOR_TRAPA_0,
    VECTOR_TRAPA_1,
    VECTOR_TRAPA_2,
    VECTOR_TRAPA_3,
    VECTOR_TRAPA_4,
    VECTOR_TRAPA_5,
    VECTOR_TRAPA_6,
    VECTOR_TRAPA_7,
    VECTOR_TRAPA_8,
    VECTOR_TRAPA_9,
    VECTOR_TRAPA_A,
    VECTOR_TRAPA_B,
    VECTOR_TRAPA_C,
    VECTOR_TRAPA_D,
    VECTOR_TRAPA_E,
    VECTOR_TRAPA_F,
    VECTOR_IRQ0,
    VECTOR_IRQ1,
    VECTOR_INTERNAL_INTERRUPT_88, // UNUSED
    VECTOR_INTERNAL_INTERRUPT_8C, // UNUSED
    VECTOR_INTERNAL_INTERRUPT_90, // FRT1 ICI
    VECTOR_INTERNAL_INTERRUPT_94, // FRT1 OCIA
    VECTOR_INTERNAL_INTERRUPT_98, // FRT1 OCIB
    VECTOR_INTERNAL_INTERRUPT_9C, // FRT1 FOVI
    VECTOR_INTERNAL_INTERRUPT_A0, // FRT2 ICI
    VECTOR_INTERNAL_INTERRUPT_A4, // FRT2 OCIA
    VECTOR_INTERNAL_INTERRUPT_A8, // FRT2 OCIB
    VECTOR_INTERNAL_INTERRUPT_AC, // FRT2 FOVI
    VECTOR_INTERNAL_INTERRUPT_B0, // FRT3 ICI
    VECTOR_INTERNAL_INTERRUPT_B4, // FRT3 OCIA
    VECTOR_INTERNAL_INTERRUPT_B8, // FRT3 OCIB
    VECTOR_INTERNAL_INTERRUPT_BC, // FRT3 FOVI
    VECTOR_INTERNAL_INTERRUPT_C0, // CMIA
    VECTOR_INTERNAL_INTERRUPT_C4, // CMIB
    VECTOR_INTERNAL_INTERRUPT_C8, // OVI
    VECTOR_INTERNAL_INTERRUPT_CC, // UNUSED
    VECTOR_INTERNAL_INTERRUPT_D0, // ERI
    VECTOR_INTERNAL_INTERRUPT_D4, // RXI
    VECTOR_INTERNAL_INTERRUPT_D8, // TXI
    VECTOR_INTERNAL_INTERRUPT_DC, // UNUSED
    VECTOR_INTERNAL_INTERRUPT_E0, // ADI
};

enum {
    INTERRUPT_SOURCE_NMI = 0,
    INTERRUPT_SOURCE_IRQ0, // GPINT
    INTERRUPT_SOURCE_IRQ1,
    INTERRUPT_SOURCE_FRT0_ICI,
    INTERRUPT_SOURCE_FRT0_OCIA,
    INTERRUPT_SOURCE_FRT0_OCIB,
    INTERRUPT_SOURCE_FRT0_FOVI,
    INTERRUPT_SOURCE_FRT1_ICI,
    INTERRUPT_SOURCE_FRT1_OCIA,
    INTERRUPT_SOURCE_FRT1_OCIB,
    INTERRUPT_SOURCE_FRT1_FOVI,
    INTERRUPT_SOURCE_FRT2_ICI,
    INTERRUPT_SOURCE_FRT2_OCIA,
    INTERRUPT_SOURCE_FRT2_OCIB,
    INTERRUPT_SOURCE_FRT2_FOVI,
    INTERRUPT_SOURCE_TIMER_CMIA,
    INTERRUPT_SOURCE_TIMER_CMIB,
    INTERRUPT_SOURCE_TIMER_OVI,
    INTERRUPT_SOURCE_ANALOG,
    INTERRUPT_SOURCE_UART_RX,
    INTERRUPT_SOURCE_UART_TX,
    INTERRUPT_SOURCE_MAX
};

struct mcu_t {
    uint16_t r[8];
    uint16_t pc;
    uint16_t sr;
    uint8_t cp, dp, ep, tp, br;
    uint8_t sleep;
    uint8_t ex_ignore;
    int32_t exception_pending;
    uint8_t interrupt_pending[INTERRUPT_SOURCE_MAX];
    uint8_t trapa_pending[16];
    uint64_t cycles;
};

struct submcu_t {
    uint16_t pc;
    uint8_t a;
    uint8_t x;
    uint8_t y;
    uint8_t s;
    uint8_t sr;
    uint64_t cycles;
    uint8_t sleep;
};

struct pcm_t {
    uint32_t ram1[32][8];
    uint16_t ram2[32][16];
    uint32_t select_channel;
    uint32_t voice_mask;
    uint32_t voice_mask_pending;
    uint32_t voice_mask_updating;
    uint32_t write_latch;
    uint32_t wave_read_address;
    uint8_t wave_byte_latch;
    uint32_t read_latch;
    uint8_t config_reg_3c; // SC55:c3 JV880:c0
    uint8_t config_reg_3d;
    uint32_t irq_channel;
    uint32_t irq_assert;

    uint32_t nfs;

    uint32_t tv_counter;

    uint64_t cycles;

    uint16_t eram[0x4000];

    int accum_l;
    int accum_r;
    int rcsum[2];
};

enum {
    // SC55
    MCU_BUTTON_POWER = 0,
    MCU_BUTTON_INST_L = 3,
    MCU_BUTTON_INST_R = 4,
    MCU_BUTTON_INST_MUTE = 5,
    MCU_BUTTON_INST_ALL = 6,

    MCU_BUTTON_MIDI_CH_L = 8,
    MCU_BUTTON_MIDI_CH_R = 9,
    MCU_BUTTON_CHORUS_L = 10,
    MCU_BUTTON_CHORUS_R = 11,
    MCU_BUTTON_PAN_L = 12,
    MCU_BUTTON_PAN_R = 13,
    MCU_BUTTON_PART_R = 14,

    MCU_BUTTON_KEY_SHIFT_L = 16,
    MCU_BUTTON_KEY_SHIFT_R = 17,
    MCU_BUTTON_REVERB_L = 18,
    MCU_BUTTON_REVERB_R = 19,
    MCU_BUTTON_LEVEL_L = 20,
    MCU_BUTTON_LEVEL_R = 21,
    MCU_BUTTON_PART_L = 22,

    // SC155 extra buttons
    MCU_BUTTON_USER = 1,
    MCU_BUTTON_PART_SEL = 2,
    MCU_BUTTON_INST_CALL = 7,
    MCU_BUTTON_PAN = 15,
    MCU_BUTTON_LEVEL = 23,
    MCU_BUTTON_PART1 = 24,
    MCU_BUTTON_PART2 = 25,
    MCU_BUTTON_PART3 = 26,
    MCU_BUTTON_PART4 = 27,
    MCU_BUTTON_PART5 = 28,
    MCU_BUTTON_PART6 = 29,
    MCU_BUTTON_PART7 = 30,
    MCU_BUTTON_PART8 = 31,

    // JV880
    MCU_BUTTON_CURSOR_L = 0,
    MCU_BUTTON_CURSOR_R = 1,
    MCU_BUTTON_TONE_SELECT = 2,
    MCU_BUTTON_MUTE = 3,
    MCU_BUTTON_DATA = 4,
    MCU_BUTTON_MONITOR = 5,
    MCU_BUTTON_COMPARE = 6,
    MCU_BUTTON_ENTER = 7,
    MCU_BUTTON_UTILITY = 8,
    MCU_BUTTON_PREVIEW = 9,
    MCU_BUTTON_PATCH_PERFORM = 10,
    MCU_BUTTON_EDIT = 11,
    MCU_BUTTON_SYSTEM = 12,
    MCU_BUTTON_RHYTHM = 13,
};

enum {
    ROM_SET_MK2 = 0,
    ROM_SET_ST,
    ROM_SET_MK1,
    ROM_SET_CM300,
    ROM_SET_JV880,
    ROM_SET_SCB55,
    ROM_SET_RLP3237,
    ROM_SET_SC155,
    ROM_SET_SC155MK2,
    ROM_SET_COUNT
};

extern const char* rs_name[ROM_SET_COUNT];

enum { uart_buffer_size = 8192 };

struct frt_t {
    uint8_t tcr;
    uint8_t tcsr;
    uint16_t frc;
    uint16_t ocra;
    uint16_t ocrb;
    uint16_t icr;
    uint8_t status_rd;
};

struct mcu_timer_t {
    uint8_t tcr;
    uint8_t tcsr;
    uint8_t tcora;
    uint8_t tcorb;
    uint8_t tcnt;
    uint8_t status_rd;
};

enum { ROM1_SIZE = 0x8000 };
enum { ROM2_SIZE = 0x80000 };
enum { RAM_SIZE = 0x400 };
enum { SRAM_SIZE = 0x8000 };
enum { NVRAM_SIZE = 0x8000 }; // JV880 only
enum { CARDRAM_SIZE = 0x8000 }; // JV880 only
enum { ROMSM_SIZE = 0x1000 };

struct lcd_state_t {
    uint32_t LCD_DL, LCD_N, LCD_F, LCD_D, LCD_C, LCD_B, LCD_ID, LCD_S;
    uint32_t LCD_DD_RAM, LCD_AC, LCD_CG_RAM;
    uint32_t LCD_RAM_MODE; //= 0;
    uint8_t LCD_Data[80];
    uint8_t LCD_CG[64];

    uint8_t lcd_enable; //= 1;

    // static state carrying info
    uint32_t lcd_width;
    uint32_t lcd_height;

    uint32_t lcd_col1;
    uint32_t lcd_col2;

    uint8_t mcu_mk1;
    uint8_t mcu_cm300;
    uint8_t mcu_st;
    uint8_t mcu_jv880;
    uint8_t mcu_scb55;
    uint8_t mcu_sc155;
};

struct lcd_t {
    // These are initialized once on startup and are stable
    int lcd_width;
    int lcd_height;
    uint32_t lcd_col1;
    uint32_t lcd_col2;

    // This mutates, and will be pushed to callback periodically as a raw data block
    struct lcd_state_t state;

    struct lcd_state_t lastState;
};

struct sc55_state {
    struct mcu_t mcu;

    struct submcu_t sm;

    int port;

    int romset;

    int mcu_mk1;   //= 0; // 0 - SC-55mkII, SC-55ST. 1 - SC-55, CM-300/SCC-1
    int mcu_cm300; //= 0; // 0 - SC-55, 1 - CM-300/SCC-1
    int mcu_st;    //= 0; // 0 - SC-55mk2, 1 - SC-55ST
    int mcu_jv880; //= 0; // 0 - SC-55, 1 - JV880
    int mcu_scb55; //= 0; // 0 - sub mcu (e.g SC-55mk2), 1 - no sub mcu (e.g SCB-55)
    int mcu_sc155; //= 0; // 0 - SC-55(MK2), 1 - SC-155(MK2)

    uint32_t audio_buffer_size;
    uint32_t audio_page_size;
    short *sample_buffer;

    uint32_t sample_rate;

    uint32_t sample_buffer_requested;
    uint32_t sample_buffer_count;
    uint32_t sample_read_ptr;
    uint32_t sample_write_ptr;

    int ga_int[8];
    int ga_int_enable;  //= 0;
    int ga_int_trigger; //= 0;
    int ga_lcd_counter; //= 0;

    uint8_t dev_register[0x80];

    uint16_t ad_val[4];
    uint8_t ad_nibble; //= 0x00;
    uint8_t sw_pos;    //= 3;
    uint8_t io_sd;     //= 0x00;

    int adf_rd; //= 0;

    uint64_t analog_end_time;

    int ssr_rd; //= 0;

    uint32_t uart_write_ptr;
    uint32_t uart_read_ptr;
    uint8_t uart_buffer[uart_buffer_size];

    uint8_t uart_rx_byte;
    uint64_t uart_rx_delay;
    uint64_t uart_tx_delay;

    uint8_t uart_rx_gotbyte;

    uint32_t mcu_button_pressed;

    uint8_t sm_rom[4096];

    uint8_t sm_ram[128];
    uint8_t sm_shared_ram[192];
    uint8_t sm_access[0x18];

    uint8_t sm_p0_dir;
    uint8_t sm_p1_dir;

    uint8_t sm_device_mode[32];
    uint8_t sm_cts;

    uint64_t sm_timer_cycles;
    uint8_t sm_timer_prescaler;
    uint8_t sm_timer_counter;

    struct pcm_t pcm;

    uint64_t sample_counter;
    sc55_push_lcd lcdCallback;
    void *lcdContext;
    struct lcd_t lcd;

    uint8_t rom1[ROM1_SIZE];
    uint8_t rom2[ROM2_SIZE];
    uint8_t ram[RAM_SIZE];
    uint8_t sram[SRAM_SIZE];
    uint8_t nvram[NVRAM_SIZE];
    uint8_t cardram[CARDRAM_SIZE];

    uint8_t waverom1[0x200000];
    uint8_t waverom2[0x200000];
    uint8_t waverom3[0x100000];
    uint8_t waverom_card[0x200000];
    uint8_t waverom_exp[0x800000];

    uint32_t rom2_mask; //= ROM2_SIZE - 1;

    uint8_t mcu_p0_data; //= 0x00;
    uint8_t mcu_p1_data; //= 0x00;

    uint32_t operand_type;
    uint16_t operand_ea;
    uint8_t operand_ep;
    uint8_t operand_size;
    uint8_t operand_reg;
    uint8_t operand_status;
    uint16_t operand_data;
    uint8_t opcode_extended;

    uint64_t timer_cycles;
    uint8_t timer_tempreg;

    frt_t frt[3];
    mcu_timer_t timer;
};
