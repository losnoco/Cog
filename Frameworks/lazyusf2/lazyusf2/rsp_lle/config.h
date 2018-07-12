/******************************************************************************\
* Authors:  Iconoclast                                                         *
* Release:  2013.12.04                                                         *
* License:  CC0 Public Domain Dedication                                       *
*                                                                              *
* To the extent possible under law, the author(s) have dedicated all copyright *
* and related and neighboring rights to this software to the public domain     *
* worldwide. This software is distributed without any warranty.                *
*                                                                              *
* You should have received a copy of the CC0 Public Domain Dedication along    *
* with this software.                                                          *
* If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.             *
\******************************************************************************/
#define _CRT_SECURE_NO_WARNINGS
/*
 * This is only here for people using modern Microsoft compilers.
 * Usually the default warning level complains over "deprecated" CRT methods.
 * It's basically Microsoft's way of saying they're better than everyone.
 */

#define MINIMUM_MESSAGE_PRIORITY    1
#define EXTERN_COMMAND_LIST_GBI
#define EXTERN_COMMAND_LIST_ABI
#define SEMAPHORE_LOCK_CORRECTIONS
#define WAIT_FOR_CPU_HOST
#define EMULATE_STATIC_PC

#ifdef EMULATE_STATIC_PC
#define CONTINUE    {continue;}
#define JUMP        {goto BRANCH;}
#else
#define CONTINUE    {break;}
#define JUMP        {break;}
#endif

#if (0)
#define SP_EXECUTE_LOG
#define VU_EMULATE_SCALAR_ACCUMULATOR_READ
#endif

#define CFG_HLE_GFX     (0)
#define CFG_HLE_AUD     (0)
#define CFG_HLE_VID     (0) /* reserved/unused */
#define CFG_HLE_JPG     (0) /* unused */
#define CFG_QUEUE_E_DRAM    (0)
#define CFG_QUEUE_E_DMEM    (0)
#define CFG_QUEUE_E_IMEM    (0)
/*
 * Note:  This never actually made it into the configuration system.
 * Instead, DMEM and IMEM are always exported on every call to DllConfig().
 */

/*
 * Special switches.
 * (generally for correcting RSP clock behavior on Project64 2.x)
 * Also includes RSP register states debugger.
 */
#define CFG_WAIT_FOR_CPU_HOST       (1)
#define CFG_MEND_SEMAPHORE_LOCK     (0)
#define CFG_TRACE_RSP_REGISTERS     (0)
