#ifndef USB_BRIDGE_H
#define USB_BRIDGE_H

#include <stdint.h>
#include <stdbool.h>

// --- Definitions for Client Usage ---
#define BRIDGE_MOUSE_BTN_LEFT   0x01
#define BRIDGE_MOUSE_BTN_RIGHT  0x02
#define BRIDGE_MOUSE_BTN_MIDDLE 0x04
#define BRIDGE_MOUSE_BTN_BACK   0x08
#define BRIDGE_MOUSE_BTN_FORWARD 0x10

// --- Function Prototypes ---

// Initialize the USB stack
void usb_bridge_init(void);

// Run the USB task (call this in your main loop)
void usb_bridge_task(void);

// Send Keyboard Report
void usb_bridge_send_keyboard(uint8_t modifier, uint8_t keycode[6]);

/**
 * Send Mouse Report
 * 
 * @param buttons   Bitmask of buttons (Use BRIDGE_MOUSE_BTN_x)
 * @param x         Delta X movement (-127 to 127)
 * @param y         Delta Y movement (-127 to 127)
 * @param scroll    Vertical scroll (-127 to 127)
 * @param pan       Horizontal scroll (-127 to 127) - usually 0
 */
void usb_bridge_send_mouse(uint8_t buttons, int8_t x, int8_t y, int8_t scroll, int8_t pan);

#endif