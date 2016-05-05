/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2011-2016 Leandro Nini <drfiemost@users.sourceforge.net>
 * Copyright 2007-2010 Antti Lankila
 * Copyright 2000 Simon White
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "mos6510debug.h"

#ifdef DEBUG

#include <cstdio>
#include <cstdlib>

#include "mos6510.h"
#include "sidendian.h"
#include "opcodes.h"

namespace libsidplayfp
{

void MOS6510Debug::DumpState (event_clock_t time, MOS6510 &cpu)
{
    fprintf(cpu.m_fdbg, " PC  I  A  X  Y  SP  DR PR NV-BDIZC  Instruction (%d)\n", static_cast<int>(time));
    fprintf(cpu.m_fdbg, "%04x ",   cpu.instrStartPC);
    fprintf(cpu.m_fdbg, cpu.irqAssertedOnPin ? "t " : "f ");
    fprintf(cpu.m_fdbg, "%02x ",   cpu.Register_Accumulator);
    fprintf(cpu.m_fdbg, "%02x ",   cpu.Register_X);
    fprintf(cpu.m_fdbg, "%02x ",   cpu.Register_Y);
    fprintf(cpu.m_fdbg, "01%02x ", endian_16lo8(cpu.Register_StackPointer));
    fprintf(cpu.m_fdbg, "%02x ",   cpu.cpuRead (0));
    fprintf(cpu.m_fdbg, "%02x ",   cpu.cpuRead (1));

    fprintf(cpu.m_fdbg, cpu.flags.getN() ? "1" : "0");
    fprintf(cpu.m_fdbg, cpu.flags.getV()  ? "1" : "0");
    fprintf(cpu.m_fdbg, "1");
    fprintf(cpu.m_fdbg, cpu.flags.getB() ? "1" : "0");
    fprintf(cpu.m_fdbg, cpu.flags.getD() ? "1" : "0");
    fprintf(cpu.m_fdbg, cpu.flags.getI() ? "1" : "0");
    fprintf(cpu.m_fdbg, cpu.flags.getZ() ? "1" : "0");
    fprintf(cpu.m_fdbg, cpu.flags.getC() ? "1" : "0");

    const int opcode  = cpu.cpuRead(cpu.instrStartPC);

    fprintf(cpu.m_fdbg, "  %02x ", opcode);

    switch(opcode)
    {
    // Accumulator or Implied cpu.Cycle_EffectiveAddressing
    case ASLn: case LSRn: case ROLn: case RORn:
        fprintf(cpu.m_fdbg, "      ");
        break;
    // Zero Page Addressing Mode Handler
    case ADCz: case ANDz: case ASLz: case BITz: case CMPz: case CPXz:
    case CPYz: case DCPz: case DECz: case EORz: case INCz: case ISBz:
    case LAXz: case LDAz: case LDXz: case LDYz: case LSRz: case NOPz_:
    case ORAz: case ROLz: case RORz: case SAXz: case SBCz: case SREz:
    case STAz: case STXz: case STYz: case SLOz: case RLAz: case RRAz:
    // ASOz AXSz DCMz INSz LSEz - Optional Opcode Names
        fprintf(cpu.m_fdbg, "%02x    ", endian_16lo8(cpu.instrOperand));
            break;
    // Zero Page with X Offset Addressing Mode Handler
    case ADCzx:  case ANDzx: case ASLzx: case CMPzx: case DCPzx: case DECzx:
    case EORzx:  case INCzx: case ISBzx: case LDAzx: case LDYzx: case LSRzx:
    case NOPzx_: case ORAzx: case RLAzx: case ROLzx: case RORzx: case RRAzx:
    case SBCzx:  case SLOzx: case SREzx: case STAzx: case STYzx:
    // ASOzx DCMzx INSzx LSEzx - Optional Opcode Names
        fprintf(cpu.m_fdbg, "%02x    ", endian_16lo8(cpu.instrOperand));
            break;
    // Zero Page with Y Offset Addressing Mode Handler
    case LDXzy: case STXzy: case SAXzy: case LAXzy:
    // AXSzx - Optional Opcode Names
        fprintf(cpu.m_fdbg, "%02x    ", endian_16lo8(cpu.instrOperand));
            break;
    // Absolute Addressing Mode Handler
    case ADCa: case ANDa: case ASLa: case BITa: case CMPa: case CPXa:
    case CPYa: case DCPa: case DECa: case EORa: case INCa: case ISBa:
    case JMPw: case JSRw: case LAXa: case LDAa: case LDXa: case LDYa:
    case LSRa: case NOPa: case ORAa: case ROLa: case RORa: case SAXa:
    case SBCa: case SLOa: case SREa: case STAa: case STXa: case STYa:
    case RLAa: case RRAa:
    // ASOa AXSa DCMa INSa LSEa - Optional Opcode Names
        fprintf(cpu.m_fdbg, "%02x %02x ", endian_16lo8(cpu.instrOperand), endian_16hi8 (cpu.instrOperand));
            break;
    // Absolute With X Offset Addresing Mode Handler
    case ADCax:  case ANDax: case ASLax: case CMPax: case DCPax: case DECax:
    case EORax:  case INCax: case ISBax: case LDAax: case LDYax: case LSRax:
    case NOPax_: case ORAax: case RLAax: case ROLax: case RORax: case RRAax:
    case SBCax:  case SHYax: case SLOax: case SREax: case STAax:
    // ASOax DCMax INSax LSEax SAYax - Optional Opcode Names
        fprintf(cpu.m_fdbg, "%02x %02x ", endian_16lo8(cpu.instrOperand), endian_16hi8 (cpu.instrOperand));
            break;
    // Absolute With Y Offset Addresing Mode Handler
    case ADCay: case ANDay: case CMPay: case DCPay: case EORay: case ISBay:
    case LASay: case LAXay: case LDAay: case LDXay: case ORAay: case RLAay:
    case RRAay: case SBCay: case SHAay: case SHSay: case SHXay: case SLOay:
    case SREay: case STAay:
    // ASOay AXAay DCMay INSax LSEay TASay XASay - Optional Opcode Names
        fprintf(cpu.m_fdbg, "%02x %02x ", endian_16lo8(cpu.instrOperand), endian_16hi8 (cpu.instrOperand));
            break;
    // Immediate and Relative Addressing Mode Handler
    case ADCb: case ANDb: case ANCb_: case ANEb: case ASRb:  case ARRb:
    case BCCr: case BCSr: case BEQr: case BMIr: case BNEr: case BPLr:
    case BVCr: case BVSr:
    case CMPb: case CPXb: case CPYb:  case EORb: case LDAb:  case LDXb:
    case LDYb: case LXAb: case NOPb_: case ORAb: case SBCb_: case SBXb:
    // OALb ALRb XAAb - Optional Opcode Names
        fprintf(cpu.m_fdbg, "%02x    ", endian_16lo8(cpu.Cycle_Data));
            break;
    // Indirect Addressing Mode Handler
    case JMPi:
        fprintf(cpu.m_fdbg, "%02x %02x ", endian_16lo8(cpu.instrOperand), endian_16hi8 (cpu.instrOperand));
            break;
    // Indexed with X Preinc Addressing Mode Handler
    case ADCix: case ANDix: case CMPix: case DCPix: case EORix: case ISBix:
    case LAXix: case LDAix: case ORAix: case SAXix: case SBCix: case SLOix:
    case SREix: case STAix: case RLAix: case RRAix:
    // ASOix AXSix DCMix INSix LSEix - Optional Opcode Names
        fprintf(cpu.m_fdbg, "%02x    ", endian_16lo8(cpu.instrOperand));
            break;
    // Indexed with Y Postinc Addressing Mode Handler
    case ADCiy: case ANDiy: case CMPiy: case DCPiy: case EORiy: case ISBiy:
    case LAXiy: case LDAiy: case ORAiy: case RLAiy: case RRAiy: case SBCiy:
    case SHAiy: case SLOiy: case SREiy: case STAiy:
    // AXAiy ASOiy LSEiy DCMiy INSiy - Optional Opcode Names
        fprintf(cpu.m_fdbg, "%02x    ", endian_16lo8(cpu.instrOperand));
            break;
    default:
        fprintf(cpu.m_fdbg, "      ");
            break;
    }

    switch(opcode)
    {
    case ADCb: case ADCz: case ADCzx: case ADCa: case ADCax: case ADCay:
    case ADCix: case ADCiy:
        fprintf(cpu.m_fdbg, " ADC"); break;
    case ANCb_:
        fprintf(cpu.m_fdbg, "*ANC"); break;
    case ANDb: case ANDz: case ANDzx: case ANDa: case ANDax: case ANDay:
    case ANDix: case ANDiy:
        fprintf(cpu.m_fdbg, " AND"); break;
    case ANEb: // Also known as XAA
        fprintf(cpu.m_fdbg, "*ANE"); break;
    case ARRb:
        fprintf(cpu.m_fdbg, "*ARR"); break;
    case ASLn: case ASLz: case ASLzx: case ASLa: case ASLax:
        fprintf(cpu.m_fdbg, " ASL"); break;
    case ASRb: // Also known as ALR
        fprintf(cpu.m_fdbg, "*ASR"); break;
    case BCCr:
        fprintf(cpu.m_fdbg, " BCC"); break;
    case BCSr:
        fprintf(cpu.m_fdbg, " BCS"); break;
    case BEQr:
        fprintf(cpu.m_fdbg, " BEQ"); break;
    case BITz: case BITa:
        fprintf(cpu.m_fdbg, " BIT"); break;
    case BMIr:
        fprintf(cpu.m_fdbg, " BMI"); break;
    case BNEr:
        fprintf(cpu.m_fdbg, " BNE"); break;
    case BPLr:
        fprintf(cpu.m_fdbg, " BPL"); break;
    case BRKn:
        fprintf(cpu.m_fdbg, " BRK"); break;
    case BVCr:
        fprintf(cpu.m_fdbg, " BVC"); break;
    case BVSr:
        fprintf(cpu.m_fdbg, " BVS"); break;
    case CLCn:
        fprintf(cpu.m_fdbg, " CLC"); break;
    case CLDn:
        fprintf(cpu.m_fdbg, " CLD"); break;
    case CLIn:
        fprintf(cpu.m_fdbg, " CLI"); break;
    case CLVn:
        fprintf(cpu.m_fdbg, " CLV"); break;
    case CMPb: case CMPz: case CMPzx: case CMPa: case CMPax: case CMPay:
    case CMPix: case CMPiy:
        fprintf(cpu.m_fdbg, " CMP"); break;
    case CPXb: case CPXz: case CPXa:
        fprintf(cpu.m_fdbg, " CPX"); break;
    case CPYb: case CPYz: case CPYa:
        fprintf(cpu.m_fdbg, " CPY"); break;
    case DCPz: case DCPzx: case DCPa: case DCPax: case DCPay: case DCPix:
    case DCPiy: // Also known as DCM
        fprintf(cpu.m_fdbg, "*DCP"); break;
    case DECz: case DECzx: case DECa: case DECax:
        fprintf(cpu.m_fdbg, " DEC"); break;
    case DEXn:
        fprintf(cpu.m_fdbg, " DEX"); break;
    case DEYn:
        fprintf(cpu.m_fdbg, " DEY"); break;
    case EORb: case EORz: case EORzx: case EORa: case EORax: case EORay:
    case EORix: case EORiy:
        fprintf(cpu.m_fdbg, " EOR"); break;
    case INCz: case INCzx: case INCa: case INCax:
        fprintf(cpu.m_fdbg, " INC"); break;
    case INXn:
        fprintf(cpu.m_fdbg, " INX"); break;
    case INYn:
        fprintf(cpu.m_fdbg, " INY"); break;
    case ISBz: case ISBzx: case ISBa: case ISBax: case ISBay: case ISBix:
    case ISBiy: // Also known as INS
        fprintf(cpu.m_fdbg, "*ISB"); break;
    case JMPw: case JMPi:
        fprintf(cpu.m_fdbg, " JMP"); break;
    case JSRw:
        fprintf(cpu.m_fdbg, " JSR"); break;
    case LASay:
        fprintf(cpu.m_fdbg, "*LAS"); break;
    case LAXz: case LAXzy: case LAXa: case LAXay: case LAXix: case LAXiy:
        fprintf(cpu.m_fdbg, "*LAX"); break;
    case LDAb: case LDAz: case LDAzx: case LDAa: case LDAax: case LDAay:
    case LDAix: case LDAiy:
        fprintf(cpu.m_fdbg, " LDA"); break;
    case LDXb: case LDXz: case LDXzy: case LDXa: case LDXay:
        fprintf(cpu.m_fdbg, " LDX"); break;
    case LDYb: case LDYz: case LDYzx: case LDYa: case LDYax:
        fprintf(cpu.m_fdbg, " LDY"); break;
    case LSRz: case LSRzx: case LSRa: case LSRax: case LSRn:
        fprintf(cpu.m_fdbg, " LSR"); break;
    case NOPn_: case NOPb_: case NOPz_: case NOPzx_: case NOPa: case NOPax_:
        if(opcode != NOPn) fprintf(cpu.m_fdbg, "*");
        else fprintf(cpu.m_fdbg, " ");
        fprintf(cpu.m_fdbg, "NOP"); break;
    case LXAb: // Also known as OAL
        fprintf(cpu.m_fdbg, "*LXA"); break;
    case ORAb: case ORAz: case ORAzx: case ORAa: case ORAax: case ORAay:
    case ORAix: case ORAiy:
        fprintf(cpu.m_fdbg, " ORA"); break;
    case PHAn:
        fprintf(cpu.m_fdbg, " PHA"); break;
    case PHPn:
        fprintf(cpu.m_fdbg, " PHP"); break;
    case PLAn:
        fprintf(cpu.m_fdbg, " PLA"); break;
    case PLPn:
        fprintf(cpu.m_fdbg, " PLP"); break;
    case RLAz: case RLAzx: case RLAix: case RLAa: case RLAax: case RLAay:
    case RLAiy:
        fprintf(cpu.m_fdbg, "*RLA"); break;
    case ROLz: case ROLzx: case ROLa: case ROLax: case ROLn:
        fprintf(cpu.m_fdbg, " ROL"); break;
    case RORz: case RORzx: case RORa: case RORax: case RORn:
        fprintf(cpu.m_fdbg, " ROR"); break;
    case RRAa: case RRAax: case RRAay: case RRAz: case RRAzx: case RRAix:
    case RRAiy:
        fprintf(cpu.m_fdbg, "*RRA"); break;
    case RTIn:
        fprintf(cpu.m_fdbg, " RTI"); break;
    case RTSn:
        fprintf(cpu.m_fdbg, " RTS"); break;
    case SAXz: case SAXzy: case SAXa: case SAXix: // Also known as AXS
        fprintf(cpu.m_fdbg, "*SAX"); break;
    case SBCb_:
        if(opcode != SBCb) fprintf(cpu.m_fdbg, "*");
        else fprintf(cpu.m_fdbg, " ");
        fprintf(cpu.m_fdbg, "SBC"); break;
    case SBCz: case SBCzx: case SBCa: case SBCax: case SBCay: case SBCix:
    case SBCiy:
        fprintf(cpu.m_fdbg, " SBC"); break;
    case SBXb:
        fprintf(cpu.m_fdbg, "*SBX"); break;
    case SECn:
        fprintf(cpu.m_fdbg, " SEC"); break;
    case SEDn:
        fprintf(cpu.m_fdbg, " SED"); break;
    case SEIn:
        fprintf(cpu.m_fdbg, " SEI"); break;
    case SHAay: case SHAiy: // Also known as AXA
        fprintf(cpu.m_fdbg, "*SHA"); break;
    case SHSay: // Also known as TAS
        fprintf(cpu.m_fdbg, "*SHS"); break;
    case SHXay: // Also known as XAS
        fprintf(cpu.m_fdbg, "*SHX"); break;
    case SHYax: // Also known as SAY
        fprintf(cpu.m_fdbg, "*SHY"); break;
    case SLOz: case SLOzx: case SLOa: case SLOax: case SLOay: case SLOix:
    case SLOiy: // Also known as ASO
        fprintf(cpu.m_fdbg, "*SLO"); break;
    case SREz: case SREzx: case SREa: case SREax: case SREay: case SREix:
    case SREiy: // Also known as LSE
        fprintf(cpu.m_fdbg, "*SRE"); break;
    case STAz: case STAzx: case STAa: case STAax: case STAay: case STAix:
    case STAiy:
        fprintf(cpu.m_fdbg, " STA"); break;
    case STXz: case STXzy: case STXa:
        fprintf(cpu.m_fdbg, " STX"); break;
    case STYz: case STYzx: case STYa:
        fprintf(cpu.m_fdbg, " STY"); break;
    case TAXn:
        fprintf(cpu.m_fdbg, " TAX"); break;
    case TAYn:
        fprintf(cpu.m_fdbg, " TAY"); break;
    case TSXn:
        fprintf(cpu.m_fdbg, " TSX"); break;
    case TXAn:
        fprintf(cpu.m_fdbg, " TXA"); break;
    case TXSn:
        fprintf(cpu.m_fdbg, " TXS"); break;
    case TYAn:
        fprintf(cpu.m_fdbg, " TYA"); break;
    default:
        fprintf(cpu.m_fdbg, "*HLT"); break;
    }

    switch(opcode)
    {
    // Accumulator or Implied cpu.Cycle_EffectiveAddressing
    case ASLn: case LSRn: case ROLn: case RORn:
        fprintf(cpu.m_fdbg, "n  A");
        break;

    // Zero Page Addressing Mode Handler
    case ADCz: case ANDz: case ASLz: case BITz: case CMPz: case CPXz:
    case CPYz: case DCPz: case DECz: case EORz: case INCz: case ISBz:
    case LAXz: case LDAz: case LDXz: case LDYz: case LSRz: case ORAz:

    case ROLz: case RORz: case SBCz: case SREz: case SLOz: case RLAz:
    case RRAz:
    // ASOz AXSz DCMz INSz LSEz - Optional Opcode Names
        fprintf(cpu.m_fdbg, "z  %02x {%02x}", endian_16lo8(cpu.instrOperand), cpu.Cycle_Data);
        break;
    case SAXz: case STAz: case STXz: case STYz:
#ifdef DEBUG
    case NOPz_:
#endif
        fprintf(cpu.m_fdbg, "z  %02x", endian_16lo8(cpu.instrOperand));
        break;

    // Zero Page with X Offset Addressing Mode Handler
    case ADCzx: case ANDzx: case ASLzx: case CMPzx: case DCPzx: case DECzx:
    case EORzx: case INCzx: case ISBzx: case LDAzx: case LDYzx: case LSRzx:
    case ORAzx: case RLAzx: case ROLzx: case RORzx: case RRAzx: case SBCzx:
    case SLOzx: case SREzx:
    // ASOzx DCMzx INSzx LSEzx - Optional Opcode Names
        fprintf(cpu.m_fdbg, "zx %02x,X", endian_16lo8(cpu.instrOperand));
        fprintf(cpu.m_fdbg, " [%04x]{%02x}", cpu.Cycle_EffectiveAddress, cpu.Cycle_Data);
        break;
    case STAzx: case STYzx:
#ifdef DEBUG
    case NOPzx_:
#endif
        fprintf(cpu.m_fdbg, "zx %02x,X", endian_16lo8(cpu.instrOperand));
        fprintf(cpu.m_fdbg, " [%04x]", cpu.Cycle_EffectiveAddress);
        break;

    // Zero Page with Y Offset Addressing Mode Handler
    case LAXzy: case LDXzy:
    // AXSzx - Optional Opcode Names
        fprintf(cpu.m_fdbg, "zy %02x,Y", endian_16lo8(cpu.instrOperand));
        fprintf(cpu.m_fdbg, " [%04x]{%02x}", cpu.Cycle_EffectiveAddress, cpu.Cycle_Data);
        break;
    case STXzy: case SAXzy:
        fprintf(cpu.m_fdbg, "zy %02x,Y", endian_16lo8(cpu.instrOperand));
        fprintf(cpu.m_fdbg, " [%04x]", cpu.Cycle_EffectiveAddress);
        break;

    // Absolute Addressing Mode Handler
    case ADCa: case ANDa: case ASLa: case BITa: case CMPa: case CPXa:
    case CPYa: case DCPa: case DECa: case EORa: case INCa: case ISBa:
    case LAXa: case LDAa: case LDXa: case LDYa: case LSRa: case ORAa:
    case ROLa: case RORa: case SBCa: case SLOa: case SREa: case RLAa:
    case RRAa:
    // ASOa AXSa DCMa INSa LSEa - Optional Opcode Names
        fprintf(cpu.m_fdbg, "a  %04x {%02x}", cpu.instrOperand, cpu.Cycle_Data);
        break;
    case SAXa: case STAa: case STXa: case STYa:
#ifdef DEBUG
    case NOPa:
#endif
        fprintf(cpu.m_fdbg, "a  %04x", cpu.instrOperand);
        break;
    case JMPw: case JSRw:
        fprintf(cpu.m_fdbg, "w  %04x", cpu.instrOperand);
        break;

    // Absolute With X Offset Addresing Mode Handler
    case ADCax: case ANDax: case ASLax: case CMPax: case DCPax: case DECax:
    case EORax: case INCax: case ISBax: case LDAax: case LDYax: case LSRax:
    case ORAax: case RLAax: case ROLax: case RORax: case RRAax: case SBCax:
    case SLOax: case SREax:
    // ASOax DCMax INSax LSEax SAYax - Optional Opcode Names
        fprintf(cpu.m_fdbg, "ax %04x,X", cpu.instrOperand);
        fprintf(cpu.m_fdbg, " [%04x]{%02x}", cpu.Cycle_EffectiveAddress, cpu.Cycle_Data);
        break;
    case SHYax: case STAax:
#ifdef DEBUG
    case NOPax_:
#endif
        fprintf(cpu.m_fdbg, "ax %04x,X", cpu.instrOperand);
        fprintf(cpu.m_fdbg, " [%04x]", cpu.Cycle_EffectiveAddress);
        break;

    // Absolute With Y Offset Addresing Mode Handler
    case ADCay: case ANDay: case CMPay: case DCPay: case EORay: case ISBay:
    case LASay: case LAXay: case LDAay: case LDXay: case ORAay: case RLAay:
    case RRAay: case SBCay: case SHSay: case SLOay: case SREay:
    // ASOay AXAay DCMay INSax LSEay TASay XASay - Optional Opcode Names
        fprintf(cpu.m_fdbg, "ay %04x,Y", cpu.instrOperand);
        fprintf(cpu.m_fdbg, " [%04x]{%02x}", cpu.Cycle_EffectiveAddress, cpu.Cycle_Data);
        break;
    case SHAay: case SHXay: case STAay:
        fprintf(cpu.m_fdbg, "ay %04x,Y", cpu.instrOperand);
        fprintf(cpu.m_fdbg, " [%04x]", cpu.Cycle_EffectiveAddress);
        break;

    // Immediate Addressing Mode Handler
    case ADCb: case ANDb: case ANCb_: case ANEb: case ASRb:  case ARRb:
    case CMPb: case CPXb: case CPYb:  case EORb: case LDAb:  case LDXb:
    case LDYb: case LXAb: case ORAb: case SBCb_: case SBXb:
    // OALb ALRb XAAb - Optional Opcode Names
#ifdef DEBUG
    case NOPb_:
#endif
        fprintf(cpu.m_fdbg, "b  #%02x", endian_16lo8(cpu.instrOperand));
        break;

    // Relative Addressing Mode Handler
    case BCCr: case BCSr: case BEQr: case BMIr: case BNEr: case BPLr:
    case BVCr: case BVSr:
        fprintf(cpu.m_fdbg, "r  #%02x", endian_16lo8(cpu.instrOperand));
        fprintf(cpu.m_fdbg, " [%04x]", cpu.Cycle_EffectiveAddress);
        break;

    // Indirect Addressing Mode Handler
    case JMPi:
        fprintf(cpu.m_fdbg, "i  (%04x)", cpu.instrOperand);
        fprintf(cpu.m_fdbg, " [%04x]", cpu.Cycle_EffectiveAddress);
        break;

    // Indexed with X Preinc Addressing Mode Handler
    case ADCix: case ANDix: case CMPix: case DCPix: case EORix: case ISBix:
    case LAXix: case LDAix: case ORAix: case SBCix: case SLOix: case SREix:
    case RLAix: case RRAix:
    // ASOix AXSix DCMix INSix LSEix - Optional Opcode Names
        fprintf(cpu.m_fdbg, "ix (%02x,X)", endian_16lo8(cpu.instrOperand));
        fprintf(cpu.m_fdbg, " [%04x]{%02x}", cpu.Cycle_EffectiveAddress, cpu.Cycle_Data);
        break;
    case SAXix: case STAix:
        fprintf(cpu.m_fdbg, "ix (%02x,X)", endian_16lo8(cpu.instrOperand));
        fprintf(cpu.m_fdbg, " [%04x]", cpu.Cycle_EffectiveAddress);
        break;

    // Indexed with Y Postinc Addressing Mode Handler
    case ADCiy: case ANDiy: case CMPiy: case DCPiy: case EORiy: case ISBiy:
    case LAXiy: case LDAiy: case ORAiy: case RLAiy: case RRAiy: case SBCiy:
    case SLOiy: case SREiy:
    // AXAiy ASOiy LSEiy DCMiy INSiy - Optional Opcode Names
        fprintf(cpu.m_fdbg, "iy (%02x),Y", endian_16lo8(cpu.instrOperand));
        fprintf(cpu.m_fdbg, " [%04x]{%02x}", cpu.Cycle_EffectiveAddress, cpu.Cycle_Data);
        break;
    case SHAiy: case STAiy:
        fprintf(cpu.m_fdbg, "iy (%02x),Y", endian_16lo8(cpu.instrOperand));
        fprintf(cpu.m_fdbg, " [%04x]", cpu.Cycle_EffectiveAddress);
        break;

    default:
        break;
    }

    fprintf(cpu.m_fdbg, "\n\n");
    fflush(cpu.m_fdbg);
}

}

#endif
