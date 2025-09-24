#pragma once

#include <stdint.h>

enum ResetType {
    NONE,
    GS_RESET,
    GM_RESET,
};

#ifdef __cplusplus
extern "C" {
#endif

// set buffer to NULL to detect if it exists
// set size to maximum size, it will return the actual size, and error if too small
// returns 0 on success, -1 on error
typedef int (* sc55_read_rom)(void *context, const char *name, uint8_t * buffer, uint32_t * size);

struct sc55_state * sc55_init(int port, ResetType resetType, sc55_read_rom readCallback, void *readContext);
void sc55_free(struct sc55_state *st);

uint32_t sc55_get_sample_rate(struct sc55_state *st);

void sc55_spin(struct sc55_state *st, uint32_t count);

void sc55_render(struct sc55_state *st, short *buffer, uint32_t count);

void sc55_write_uart(struct sc55_state *st, const uint8_t *data, uint32_t count);

#ifdef __cplusplus
}
#endif

