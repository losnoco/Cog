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

enum { lcd_width_max = 1024,
       lcd_height_max = 1024 };

enum { lcd_background_width = 741,
       lcd_background_height = 268 };

enum { lcd_background_size = lcd_background_width * lcd_background_height };
typedef uint32_t *lcd_background_t; // back.data

enum { lcd_buffer_size = lcd_width_max * lcd_height_max };
typedef uint32_t *lcd_buffer_t; // render output

// set buffer to NULL to detect if it exists
// set size to maximum size, it will return the actual size, and error if too small
// returns 0 on success, -1 on error
typedef int (* sc55_read_rom)(void *context, const char *name, uint8_t * buffer, uint32_t * size);

// receives opaque LCD state updates every sample interval that the LCD has changed
// timestamp is milliseconds, absolute elapsed since boot
typedef void (* sc55_push_lcd)(void *context, int port, const void *state, size_t size, uint64_t timestamp);

struct sc55_state * sc55_init(int port, enum ResetType resetType, sc55_read_rom readCallback, void *readContext);
void sc55_free(struct sc55_state *st);

uint32_t sc55_get_sample_rate(struct sc55_state *st);

void sc55_spin(struct sc55_state *st, uint32_t count);

void sc55_render(struct sc55_state *st, short *buffer, uint32_t count);
void sc55_render_with_lcd(struct sc55_state *st, short *buffer, uint32_t count, sc55_push_lcd lcdCallback, void *lcdContext);

void sc55_write_uart(struct sc55_state *st, const uint8_t *data, uint32_t count);

// LCD renderer

// Disable and erase
uint32_t sc55_lcd_state_size(void);
void sc55_lcd_clear(void *lcdState, size_t stateSize, uint32_t width, uint32_t height);

// Get screen size, static for a given model
void sc55_lcd_get_size(const struct sc55_state *st, uint32_t *width, uint32_t *height);

// state and size are expected to match what is passed to the callback above
// width and height are set on return
void sc55_lcd_render_screen(const lcd_background_t lcd_background, lcd_buffer_t lcd_buffer, const void *lcdState, size_t stateSize);

#ifdef __cplusplus
}
#endif

