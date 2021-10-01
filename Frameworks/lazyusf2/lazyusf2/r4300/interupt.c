/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - interupt.c                                              *
 *   Mupen64Plus homepage: http://code.google.com/p/mupen64plus/           *
 *   Copyright (C) 2002 Hacktarux                                          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.          *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#define M64P_CORE_PROTOTYPES 1

#include "usf/usf.h"

#include "usf/usf_internal.h"

#include "interupt.h"
#include "cached_interp.h"
#include "cp0.h"
#include "exception.h"
#include "new_dynarec/new_dynarec.h"
#include "r4300.h"
#include "r4300_core.h"
#include "reset.h"

#include "ai/ai_controller.h"
#include "api/m64p_types.h"
#include "api/callbacks.h"
#include "main/main.h"
#include "main/savestates.h"
#include "pi/pi_controller.h"
#include "rdp/rdp_core.h"
#include "rsp/rsp_core.h"
#include "si/si_controller.h"
#include "vi/vi_controller.h"


#include <string.h>

/***************************************************************************
 * Pool of Single Linked List Nodes
 **************************************************************************/
#ifndef INTERUPT_STRUCTS
#define INTERUPT_STRUCTS
#define POOL_CAPACITY 16

struct interrupt_event
{
    int type;
    unsigned int count;
};


struct node
{
    struct interrupt_event data;
    struct node *next;
};

struct pool
{
    struct node nodes[POOL_CAPACITY];
    struct node* stack[POOL_CAPACITY];
    size_t index;
};


struct interrupt_queue
{
    struct pool pool;
    struct node* first;
};
#endif

static struct node* alloc_node(struct pool* p);
static void free_node(struct pool* p, struct node* node);
static void clear_pool(struct pool* p);


/* node allocation/deallocation on a given pool */
static struct node* alloc_node(struct pool* p)
{
    /* return NULL if pool is too small */
    if (p->index >= POOL_CAPACITY)
        return NULL;

    return p->stack[p->index++];
}

static void free_node(struct pool* p, struct node* node)
{
    if (p->index == 0 || node == NULL)
        return;

    p->stack[--p->index] = node;
}

/* release all nodes */
static void clear_pool(struct pool* p)
{
    size_t i;

    for(i = 0; i < POOL_CAPACITY; ++i)
        p->stack[i] = &p->nodes[i];

    p->index = 0;
}

/***************************************************************************
 * Interrupt Queue
 **************************************************************************/


static void clear_queue(usf_state_t * state)
{
    state->q.first = NULL;
    clear_pool(&state->q.pool);
}


static int before_event(usf_state_t * state, unsigned int evt1, unsigned int evt2, int type2)
{
    int count = state->g_cp0_regs[CP0_COUNT_REG];

    if (state->cycle_count > 0)
        count -= state->cycle_count;

    if ((evt1 - count) < (evt2 - count)) return 1;
    else return 0;
}

void add_interupt_event(usf_state_t * state, int type, unsigned int delay)
{
    add_interupt_event_count(state, type, state->g_cp0_regs[CP0_COUNT_REG] + delay);
}

void add_interupt_event_count(usf_state_t * state, int type, unsigned int count)
{
    struct node* event;
    struct node* e;
   
    if (get_event(state, type)) {
        DebugMessage(state, M64MSG_WARNING, "two events of type 0x%x in interrupt queue", type);
    }

    event = alloc_node(&state->q.pool);
    if (event == NULL)
    {
        DebugMessage(state, M64MSG_ERROR, "Failed to allocate node for new interrupt event");
        return;
    }

    event->data.count = count;
    event->data.type = type;

    if (state->q.first == NULL)
    {
        state->q.first = event;
        event->next = NULL;
        state->next_interupt = state->q.first->data.count;
        state->cycle_count = state->g_cp0_regs[CP0_COUNT_REG] - state->q.first->data.count;
    }
    else if (before_event(state, count, state->q.first->data.count, state->q.first->data.type))
    {
        event->next = state->q.first;
        state->q.first = event;
        state->next_interupt = state->q.first->data.count;
        state->cycle_count = state->g_cp0_regs[CP0_COUNT_REG] - state->q.first->data.count;
    }
    else
    {
        for(e = state->q.first;
            e->next != NULL &&
            (!before_event(state, count, e->next->data.count, e->next->data.type));
            e = e->next);

        if (e->next == NULL)
        {
            e->next = event;
            event->next = NULL;
        }
        else
        {
            for(; e->next != NULL && e->next->data.count == count; e = e->next);

            event->next = e->next;
            e->next = event;
        }
    }
}

static void remove_interupt_event(usf_state_t * state)
{
    struct node* e;

    e = state->q.first;
    state->q.first = e->next;
    free_node(&state->q.pool, e);

    state->next_interupt = (state->q.first != NULL)
        ? state->q.first->data.count
        : 0;

    state->cycle_count = (state->q.first != NULL)
        ? (state->g_cp0_regs[CP0_COUNT_REG] - state->q.first->data.count)
        : 0;
}

unsigned int get_event(usf_state_t * state, int type)
{
    struct node* e = state->q.first;

    if (e == NULL)
        return 0;

    if (e->data.type == type)
        return e->data.count;

    for(; e->next != NULL && e->next->data.type != type; e = e->next);

    return (e->next != NULL)
        ? e->next->data.count
        : 0;
}

int get_next_event_type(usf_state_t * state)
{
    return (state->q.first == NULL)
        ? 0
        : state->q.first->data.type;
}

void remove_event(usf_state_t * state, int type)
{
    struct node* to_del;
    struct node* e = state->q.first;

    if (e == NULL)
        return;

    if (e->data.type == type)
    {
        state->q.first = e->next;
        free_node(&state->q.pool, e);
    }
    else
    {
        for(; e->next != NULL && e->next->data.type != type; e = e->next);

        if (e->next != NULL)
        {
            to_del = e->next;
            e->next = to_del->next;
            free_node(&state->q.pool, to_del);
        }
    }
}

void translate_event_queue(usf_state_t * state, unsigned int base)
{
    struct node* e;

    remove_event(state, COMPARE_INT);
    remove_event(state, SPECIAL_INT);

    for(e = state->q.first; e != NULL; e = e->next)
    {
        e->data.count = (e->data.count - state->g_cp0_regs[CP0_COUNT_REG]) + base;
    }

    state->g_cp0_regs[CP0_COUNT_REG] = base;
    add_interupt_event_count(state, SPECIAL_INT, ((state->g_cp0_regs[CP0_COUNT_REG] & UINT32_C(0x80000000)) ^ UINT32_C(0x80000000)));

    /* Add count_per_op to avoid wrong event order in case CP0_COUNT_REG == CP0_COMPARE_REG */
    state->g_cp0_regs[CP0_COUNT_REG] += state->count_per_op;
    state->cycle_count += state->count_per_op;
    add_interupt_event_count(state, COMPARE_INT, state->g_cp0_regs[CP0_COMPARE_REG]);
    state->g_cp0_regs[CP0_COUNT_REG] -= state->count_per_op;

    /* Update next interrupt in case first event is COMPARE_INT */
    state->cycle_count = state->g_cp0_regs[CP0_COUNT_REG] - state->q.first->data.count;
}

int save_eventqueue_infos(usf_state_t * state, char *buf)
{
    int len;
    struct node* e;

    len = 0;

    for(e = state->q.first; e != NULL; e = e->next)
    {
        memcpy(buf + len    , &e->data.type , 4);
        memcpy(buf + len + 4, &e->data.count, 4);
        len += 8;
    }

    *((unsigned int*)&buf[len]) = 0xFFFFFFFF;
    return len+4;
}

void load_eventqueue_infos(usf_state_t * state, char *buf)
{
    int len = 0;
    clear_queue(state);
    while (*((unsigned int*)&buf[len]) != 0xFFFFFFFF)
    {
        int type = *((unsigned int*)&buf[len]);
        unsigned int count = *((unsigned int*)&buf[len+4]);
        add_interupt_event_count(state, type, count);
        len += 8;
    }
    remove_event(state, SPECIAL_INT);
    add_interupt_event_count(state, SPECIAL_INT, ((state->g_cp0_regs[CP0_COUNT_REG] & UINT32_C(0x80000000)) ^ UINT32_C(0x80000000)));
}

void init_interupt(usf_state_t * state)
{
    state->SPECIAL_done = 1;

    state->g_vi.delay = state->g_vi.next_vi = 5000;

    clear_queue(state);
    add_interupt_event_count(state, VI_INT, state->g_vi.next_vi);
    add_interupt_event_count(state, SPECIAL_INT, 0x80000000);
    add_interupt_event_count(state, COMPARE_INT, 0);
}

void check_interupt(usf_state_t * state)
{
    struct node* event;

    state->g_r4300.mi.regs[MI_INTR_REG] &= ~MI_INTR_AI;
    state->g_r4300.mi.regs[MI_INTR_REG] |= state->g_r4300.mi.AudioIntrReg & MI_INTR_AI;
    
#ifdef DEBUG_INFO
    if (state->g_r4300.mi.regs[MI_INTR_REG] && state->debug_log)
        fprintf(state->debug_log, "Interrupt %d - ", state->g_r4300.mi.regs[MI_INTR_REG]);
#endif
    if (state->g_r4300.mi.regs[MI_INTR_REG] & state->g_r4300.mi.regs[MI_INTR_MASK_REG])
    {
#ifdef DEBUG_INFO
        if (state->debug_log)
          fprintf(state->debug_log, "triggered\n");
#endif
        state->g_cp0_regs[CP0_CAUSE_REG] = (state->g_cp0_regs[CP0_CAUSE_REG] | 0x400) & 0xFFFFFF83;
    }
    else
    {
#ifdef DEBUG_INFO
        if (state->g_r4300.mi.regs[MI_INTR_REG] && state->debug_log)
            fprintf(state->debug_log, "masked\n");
#endif
        state->g_cp0_regs[CP0_CAUSE_REG] &= ~0x400;
    }
    if ((state->g_cp0_regs[CP0_STATUS_REG] & 7) != 1) return;
    if (state->g_cp0_regs[CP0_STATUS_REG] & state->g_cp0_regs[CP0_CAUSE_REG] & 0xFF00)
    {
        event = alloc_node(&state->q.pool);

        if (event == NULL)
        {
            DebugMessage(state, M64MSG_ERROR, "Failed to allocate node for new interrupt event");
            return;
        }

        event->data.count = state->next_interupt = state->g_cp0_regs[CP0_COUNT_REG];
        event->data.type = CHECK_INT;
        state->cycle_count = 0;

        if (state->q.first == NULL)
        {
            state->q.first = event;
            event->next = NULL;
        }
        else
        {
            event->next = state->q.first;
            state->q.first = event;
        }
    }
}

static void wrapped_exception_general(usf_state_t * state)
{
#ifdef NEW_DYNAREC
    if (r4300emu == CORE_DYNAREC) {
        state->g_cp0_regs[CP0_EPC_REG] = pcaddr;
        state->pcaddr = 0x80000180;
        state->g_cp0_regs[CP0_STATUS_REG] |= 2;
        state->g_cp0_regs[CP0_CAUSE_REG] &= 0x7FFFFFFF;
        state->pending_exception=1;
    } else {
        exception_general(state);
    }
#else
    exception_general(state);
#endif
}

void raise_maskable_interrupt(usf_state_t * state, uint32_t cause)
{
    state->g_cp0_regs[CP0_CAUSE_REG] = (state->g_cp0_regs[CP0_CAUSE_REG] | cause) & 0xffffff83;

    if (!(state->g_cp0_regs[CP0_STATUS_REG] & state->g_cp0_regs[CP0_CAUSE_REG] & 0xff00))
        return;

    if ((state->g_cp0_regs[CP0_STATUS_REG] & 7) != 1)
        return;

    wrapped_exception_general(state);
}

static void special_int_handler(usf_state_t * state)
{
    remove_interupt_event(state);
    add_interupt_event_count(state, SPECIAL_INT, ((state->g_cp0_regs[CP0_COUNT_REG] & UINT32_C(0x80000000)) ^ UINT32_C(0x80000000)));
}

static void compare_int_handler(usf_state_t * state)
{
    remove_interupt_event(state);
    state->g_cp0_regs[CP0_COUNT_REG]+=state->count_per_op;
    state->cycle_count += state->count_per_op;
    add_interupt_event_count(state, COMPARE_INT, state->g_cp0_regs[CP0_COMPARE_REG]);
    state->g_cp0_regs[CP0_COUNT_REG]-=state->count_per_op;

    /* Update next interrupt in case first event is COMPARE_INT */
    state->cycle_count = state->g_cp0_regs[CP0_COUNT_REG] - state->q.first->data.count;

    if (state->enablecompare)
        raise_maskable_interrupt(state, 0x8000);
}

static void hw2_int_handler(usf_state_t * state)
{
    // Hardware Interrupt 2 -- remove interrupt event from queue
    remove_interupt_event(state);

    state->g_cp0_regs[CP0_STATUS_REG] = (state->g_cp0_regs[CP0_STATUS_REG] & ~0x00380000) | 0x1000;
    state->g_cp0_regs[CP0_CAUSE_REG] = (state->g_cp0_regs[CP0_CAUSE_REG] | 0x1000) & 0xffffff83;

    wrapped_exception_general(state);
}

static void nmi_int_handler(usf_state_t * state)
{
    // Non Maskable Interrupt -- remove interrupt event from queue
    remove_interupt_event(state);
    // setup r4300 Status flags: reset TS and SR, set BEV, ERL, and SR
    state->g_cp0_regs[CP0_STATUS_REG] = (state->g_cp0_regs[CP0_STATUS_REG] & ~0x00380000) | 0x00500004;
    state->g_cp0_regs[CP0_CAUSE_REG]  = 0x00000000;
    // simulate the soft reset code which would run from the PIF ROM
    r4300_reset_soft(state);
    // clear all interrupts, reset interrupt counters back to 0
    state->g_cp0_regs[CP0_COUNT_REG] = 0;
    state->g_gs_vi_counter = 0;
    init_interupt(state);
    // clear the audio status register so that subsequent write_ai() calls will work properly
    state->g_ai.regs[AI_STATUS_REG] = 0;
    // set ErrorEPC with the last instruction address
    state->g_cp0_regs[CP0_ERROREPC_REG] = state->PC->addr;
    // reset the r4300 internal state
    if (state->r4300emu != CORE_PURE_INTERPRETER)
    {
        // clear all the compiled instruction blocks and re-initialize
        free_blocks(state);
        init_blocks(state);
    }
    // adjust ErrorEPC if we were in a delay slot, and clear the delay_slot and dyna_interp flags
    if(state->delay_slot==1 || state->delay_slot==3)
    {
        state->g_cp0_regs[CP0_ERROREPC_REG]-=4;
    }
    state->delay_slot = 0;
    state->dyna_interp = 0;
    // set next instruction address to reset vector
    state->last_addr = 0xa4000040;
    generic_jump_to(state, 0xa4000040);
}


void osal_fastcall gen_interupt(usf_state_t * state)
{
    if (state->stop == 1)
    {
        state->g_gs_vi_counter = 0; // debug
#ifdef DYNAREC
        dyna_stop(state);
#endif
    }

    if (!state->interupt_unsafe_state)
    {
        if (state->reset_hard_job)
        {
            reset_hard(state);
            state->reset_hard_job = 0;
            return;
        }
    }

    if (state->skip_jump)
    {
        unsigned int dest = state->skip_jump;
        state->skip_jump = 0;

        state->next_interupt = (state->q.first != NULL)
            ? state->q.first->data.count
            : 0;

        state->cycle_count = (state->q.first != NULL)
            ? (state->g_cp0_regs[CP0_COUNT_REG] - state->q.first->data.count)
            : 0;

        state->last_addr = dest;
        generic_jump_to(state, dest);
        return;
    }

    switch(state->q.first->data.type)
    {
        case SPECIAL_INT:
            special_int_handler(state);
            break;

        case VI_INT:
            remove_interupt_event(state);
            vi_vertical_interrupt_event(&state->g_vi);
            break;

        case COMPARE_INT:
            compare_int_handler(state);
            break;

        case CHECK_INT:
            remove_interupt_event(state);
            wrapped_exception_general(state);
            break;

        case SI_INT:
            remove_interupt_event(state);
            si_end_of_dma_event(&state->g_si);
            break;

        case PI_INT:
            remove_interupt_event(state);
            pi_end_of_dma_event(&state->g_pi);
            break;

        case AI_INT:
            remove_interupt_event(state);
            ai_end_of_dma_event(&state->g_ai);
            break;

        case SP_INT:
            remove_interupt_event(state);
            rsp_interrupt_event(&state->g_sp);
            break;

        case DP_INT:
            remove_interupt_event(state);
            rdp_interrupt_event(&state->g_dp);
            break;

        case HW2_INT:
            hw2_int_handler(state);
            break;

        case NMI_INT:
            nmi_int_handler(state);
            break;

        default:
            DebugMessage(state, M64MSG_ERROR, "Unknown interrupt queue event type %.8X.", state->q.first->data.type);
            remove_interupt_event(state);
            wrapped_exception_general(state);
            break;
    }
}
