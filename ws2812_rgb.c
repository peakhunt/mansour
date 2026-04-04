#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "ws2812.pio.h"
#include "ws2812_rgb.h"
#include "mainloop_timer.h"

#define RGB_INTERVAL      10

static PIO      pio = pio0;
static uint32_t sm = 0;
static uint32_t sm2 = 1;

static float angle = 0.0f;
static float angle2 = 0.0f;

static SoftTimerElem  _t1;
static SoftTimerElem  _t2;

static void
rgb_callback(SoftTimerElem* te)
{
  float r_val = (sinf(angle) + 1.0f) / 2.0f;
  float g_val = (sinf(angle + 2.0f * M_PI / 3.0f) + 1.0f) / 2.0f;
  float b_val = (sinf(angle + 4.0f * M_PI / 3.0f) + 1.0f) / 2.0f;

  // Scale to 32 (dim/classy) or 255 (blinding/Taco Bibi)
  uint8_t r = (uint8_t)(r_val * 32.0f);
  uint8_t g = (uint8_t)(g_val * 32.0f);
  uint8_t b = (uint8_t)(b_val * 32.0f);

  // Pack GRB: 0xGGRRBB00
  uint32_t color = ((uint32_t)g << 24) | 
    ((uint32_t)r << 16) | 
    ((uint32_t)b << 8);
    ((uint32_t)b << 0);
  

  pio_sm_put_blocking(pio, sm, color);

  angle += 0.02f; // Slowed it down for maximum "Artistic" effect
  if (angle > 2.0f * M_PI) angle -= 2.0f * M_PI;

  mainloop_timer_schedule(&_t1, RGB_INTERVAL);
}

#if 0
static void
rainbow_wave_callback(SoftTimerElem* te)
{
#if 1
    static float t = 0.0f;
    t += 0.15f; // Faster wave flow

    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            float hue = sinf(x * 0.5f + t) + cosf(y * 0.5f + t);
            
            // Full 0.0 to 1.0 range for maximum juice
            float r_f = (sinf(hue + 0.0f) + 1.0f) * 0.5f;
            float g_f = (sinf(hue + 2.094f) + 1.0f) * 0.5f;
            float b_f = (sinf(hue + 4.188f) + 1.0f) * 0.5f;

            // UNLEASH THE BEAST: Scale to 255
            uint8_t r = (uint8_t)(r_f * 255.0f);
            uint8_t g = (uint8_t)(g_f * 255.0f);
            uint8_t b = (uint8_t)(b_f * 255.0f);

            uint32_t color = ((uint32_t)g << 24) | ((uint32_t)r << 16) | ((uint32_t)b << 8);
            pio_sm_put_blocking(pio, sm2, color);
        }
    }
    mainloop_timer_schedule(&_t2, 16); // ~60 FPS
#else
    // TACO BIBI STRESS TEST
for (int i = 0; i < 64; i++) {
    // Pure White: 255 R, 255 G, 255 B
    pio_sm_put_blocking(pio, sm2, 0xFFFFFF00); 
}

#endif
}
#endif

#if 0
static void
puppy_animation_callback(SoftTimerElem* te)
{
    static float t = 0.0f;
    t += 0.07f; // Animation clock

    // Procedural "Bones"
    float head_tilt = sinf(t * 0.5f) * 0.3f;   // Puppy head tilt logic
    float tail_wag = sinf(t * 4.0f) * 0.8f;    // High-speed wagging

    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            // Normalize coordinates to -1.0 to 1.0
            float nx = (x - 3.5f) / 3.5f;
            float ny = (y - 3.5f) / 3.5f;

            // 1. Head Logic (Tilting Circle)
            float hx = nx - head_tilt;
            float hy = ny - 0.4f;
            float head_dist = sqrtf(hx*hx + hy*hy);

            // 2. Body Logic (The "Puppy Loaf")
            float body_dist = sqrtf(nx*nx + (ny+0.2f)*(ny+0.2f)*0.6f);

            // 3. Tail Logic (Moving Line)
            float tx = nx + 0.6f; 
            float ty = ny + 0.5f - (nx * tail_wag * 0.5f);
            float tail_dist = sqrtf(tx*tx + ty*ty);

            uint8_t r=0, g=0, b=0;

            // Color Layering
            if (head_dist < 0.45f || body_dist < 0.5f || tail_dist < 0.2f) {
                r = 45; g = 35; // Golden Retriever Gold
            }
            
            // Add Eyes (They tilt with the head!)
            float eye_x = fabsf(hx) - 0.15f;
            float eye_y = hy - 0.1f;
            if (sqrtf(eye_x*eye_x + eye_y*eye_y) < 0.1f) {
                r = 0; g = 0; b = 60; // Electric Blue Eyes
            }

            // Push to PIO (GP22 / Physical 29)
            uint32_t color = ((uint32_t)g << 24) | ((uint32_t)r << 16) | ((uint32_t)b << 8);
            pio_sm_put_blocking(pio, sm2, color);
        }
    }

    mainloop_timer_schedule(&_t2, 20); // 50 FPS
}
#endif

#if 0
static void
puppy_eye_callback(SoftTimerElem* te)
{
    static float t = 0.0f;
    t += 0.05f;

    // Use math to "look" around the room
    float look_x = sinf(t * 0.7f) * 1.5f;
    float look_y = cosf(t * 0.3f) * 1.5f;
    
    // Blink logic (stays open most of the time)
    float blink = sinf(t * 0.2f);
    bool is_blinking = (blink > 0.9f);

    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            float nx = (x - 3.5f);
            float ny = (y - 3.5f);
            float dist = sqrtf((nx-look_x)*(nx-look_x) + (ny-look_y)*(ny-look_y));

            uint8_t r=0, g=0, b=0;

            if (is_blinking) {
                // Eyelid color (Golden)
                r = 30; g = 25;
            } else {
                // Eye Ball (White/Blue)
                if (dist < 1.5f) { 
                    b = 80; // Pupil
                } else if (sqrtf(nx*nx + ny*ny) < 3.5f) {
                    r = 40; g = 40; b = 40; // Eye White
                }
            }

            uint32_t color = ((uint32_t)g << 24) | ((uint32_t)r << 16) | ((uint32_t)b << 8);
            pio_sm_put_blocking(pio, sm2, color);
        }
    }
    mainloop_timer_schedule(&_t2, 20);
}
#endif

#if 0
static void
mbs_liquid_callback(SoftTimerElem* te)
{
    static float t = 0.0f;
    t += 0.06f; // Speed of the "soul"

    // Three "Puppy Souls" orbiting in 2D space
    float x1 = 3.5f + 2.0f * sinf(t);
    float y1 = 3.5f + 2.0f * cosf(t * 1.3f);
    float x2 = 3.5f + 2.5f * sinf(t * 0.7f);
    float y2 = 3.5f + 2.5f * cosf(t * 1.1f);
    float x3 = 3.5f + 1.5f * sinf(t * 1.9f);
    float y3 = 3.5f + 1.5f * cosf(t * 0.5f);

    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            // MBS Class Physics: Inverse Square Law for each glob
            float d1 = 1.0f / (powf(x - x1, 2) + powf(y - y1, 2) + 0.5f);
            float d2 = 1.0f / (powf(x - x2, 2) + powf(y - y2, 2) + 0.5f);
            float d3 = 1.0f / (powf(x - x3, 2) + powf(y - y3, 2) + 0.5f);
            
            float intensity = d1 + d2 + d3; // Sum the fields

            uint8_t r = 0, g = 0, b = 0;

            // Thresholding: Only light up where the "gravity" is strong
            if (intensity > 0.4f) {
                // Golden Puppy Glow (G: 60, R: 80, B: 10)
                r = (uint8_t)(fminf(intensity * 100.0f, 200.0f));
                g = (uint8_t)(r * 0.7f);
                b = (uint8_t)(r * 0.1f);
            }

            uint32_t color = ((uint32_t)g << 24) | ((uint32_t)r << 16) | ((uint32_t)b << 8);
            pio_sm_put_blocking(pio, sm2, color);
        }
    }
    mainloop_timer_schedule(&_t2, 16); // 60Hz - Total FPU Flex
}
#endif

#if 0
// 5x8 Font-ish Bitmask for "MBS MANSOUR"
// Each byte is one row of the scrolling message
static const uint64_t msg_mask[] = {
    0x0000000000000000, // Gap
    0x44EE44444BAA4444, // M  B  S
    0xAA44AA66A22A6644, //  M  A  N  S  O  U  R
    0x0000000000000000  // Gap
};

// Simplified: Let's use a 1D array of columns for easier scrolling
// 1 = Pixel On, 0 = Pixel Off
static const uint8_t mbs_scroll[] = {
    0,0,0, // Gap
    0x7F, 0x02, 0x04, 0x02, 0x7F, 0, // M
    0x7F, 0x49, 0x49, 0x49, 0x36, 0, // B
    0x46, 0x49, 0x49, 0x49, 0x31, 0, // S
    0,0,0, // Gap
    0x7F, 0x02, 0x04, 0x02, 0x7F, 0, // M
    0x7E, 0x11, 0x11, 0x11, 0x7E, 0, // A
    0x7F, 0x04, 0x08, 0x10, 0x7F, 0, // N
    0x46, 0x49, 0x49, 0x49, 0x31, 0, // S
    0x3E, 0x41, 0x41, 0x41, 0x3E, 0, // O
    0x3F, 0x40, 0x40, 0x40, 0x3F, 0, // U
    0x7F, 0x09, 0x19, 0x29, 0x46, 0, // R
    0,0,0 // Gap
};

static void
mbs_scroll_callback(SoftTimerElem* te)
{
    static float t = 0.0f;
    static int scroll_pos = 0;
    static int sub_tick = 0;

    t += 0.1f; // Rainbow phase speed

    for (int x = 0; x < 8; x++) {
        // Wrap the message around the 8x8 view
        uint8_t col_data = mbs_scroll[(scroll_pos + x) % sizeof(mbs_scroll)];
        
        for (int y = 0; y < 8; y++) {
            bool pixel_on = (col_data >> y) & 0x01;
            uint32_t color = 0;

            if (pixel_on) {
                // Rainbow math for the text
                float hue = t + (x * 0.3f) + (y * 0.2f);
                uint8_t r = (uint8_t)((sinf(hue) + 1.0f) * 60.0f);
                uint8_t g = (uint8_t)((sinf(hue + 2.094f) + 1.0f) * 60.0f);
                uint8_t b = (uint8_t)((sinf(hue + 4.188f) + 1.0f) * 60.0f);
                color = ((uint32_t)g << 24) | ((uint32_t)r << 16) | ((uint32_t)b << 8);
            }

            pio_sm_put_blocking(pio, sm2, color);
        }
    }

    // Scroll speed control
    if (++sub_tick > 8) { 
        scroll_pos = (scroll_pos + 1) % sizeof(mbs_scroll);
        sub_tick = 0;
    }

    mainloop_timer_schedule(&_t2, 15); // ~66 FPS smooth motion
}
#endif

static void
dual_scroll_callback(SoftTimerElem* te)
{
    static float phase = 0.0f;
    static uint32_t seed = 999;
    
    // Fast, violent phase for MBS energy
    phase += 0.14f; 
    
    // High-frequency turbulence for the "burning" look
    seed = (uint32_t)(seed * 1103515245 + 12345);
    float flicker = ((seed % 100) / 100.0f) * 0.25f; 

    const float center_x = 3.5f;
    const float center_y = 3.5f;

    // Hollowing logic: The "Eye" of the Hurricane
    // This oscillates between 0 (solid) and 2.5 (deep hollow)
    float hollow_radius = (sinf(phase * 0.5f) + 1.0f) * 1.25f;

    for (int x = 0; x < 8; x++) {
        for (int y = 0; y < 8; y++) {
            float dx = x - center_x;
            float dy = y - center_y;
            float dist = sqrtf(dx*dx + dy*dy);
            float angle = atan2f(dy, dx);

            // Rotational spiral arms (Hurricane effect)
            float spiral = sinf(angle * 2.0f - phase * 4.0f); 
            
            // Outer boundary of the sun
            float outer_limit = 4.0f + (spiral * 0.8f) + flicker;

            // --- THE HOLLOWING LOGIC ---
            float intensity = 0;
            if (dist < outer_limit) {
                // Brightness peaks between the hollow eye and the outer edge
                // We use a smoothstep-style calculation to create the "ring"
                intensity = 1.0f - fabsf((dist - hollow_radius) / (outer_limit - hollow_radius));
                
                // Boost intensity for a sharper, more violent contrast
                intensity = powf(intensity, 0.7f); 
            }

            if (intensity < 0) intensity = 0;
            if (intensity > 1.0f) intensity = 1.0f;

            // COLOR: "MBS Solar" - White-Hot Rings to Deep Red Fringes
            uint8_t r = (uint8_t)(255.0f * intensity);
            uint8_t g = (uint8_t)(255.0f * powf(intensity, 1.5f)); 
            uint8_t b = (uint8_t)(255.0f * powf(intensity, 3.5f));

            // WS2812 GRB Format
            uint32_t color = ((uint32_t)g << 24) | ((uint32_t)r << 16) | ((uint32_t)b << 8);
            pio_sm_put_blocking(pio, sm2, color);
        }
    }

    // 10ms for maximum fluid "Hurricane" speed
    mainloop_timer_schedule(&_t2, 50); 
}


void
ws2812_rgb_init(void)
{
  uint offset = pio_add_program(pio, &ws2812_program);
  ws2812_program_init(pio, sm, offset, 23, 800000, false);
  ws2812_program_init(pio, sm2, offset, 22, 800000, false);

  soft_timer_init_elem(&_t1);
  _t1.cb = rgb_callback;

  soft_timer_init_elem(&_t2);
  _t2.cb = dual_scroll_callback;

  mainloop_timer_schedule(&_t1, RGB_INTERVAL);
  mainloop_timer_schedule(&_t2, RGB_INTERVAL);
}
