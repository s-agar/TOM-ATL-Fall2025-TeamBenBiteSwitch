#include "usb_bridge.h"
#include "tusb.h"
#include "pico/stdlib.h"

// Required for REPORT_ID_MOUSE
#include "usb_descriptors.h" 

void usb_bridge_init(void) {
    tusb_init();
}

void usb_bridge_task(void) {
    tud_task();
}

void usb_bridge_send_keyboard(uint8_t modifier, uint8_t keycode[6]) {
    if (tud_hid_ready()) {
        tud_hid_keyboard_report(REPORT_ID_KEYBOARD, modifier, keycode);
    }
}

// NEW: Mouse Wrapper
void usb_bridge_send_mouse(uint8_t buttons, int8_t x, int8_t y, int8_t scroll, int8_t pan) {
    // Only send if USB is ready
    if (tud_hid_ready()) {
        // tud_hid_mouse_report(report_id, buttons, x, y, scroll, pan)
        tud_hid_mouse_report(REPORT_ID_MOUSE, buttons, x, y, scroll, pan);
    }
}

// --------------------------------------------------------------------+
// TinyUSB Callbacks
// --------------------------------------------------------------------+

// Invoked when received GET_REPORT control request
// Application must fill buffer report's content and return its length.
// Return zero will cause the stack to STALL request
uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen)
{
  (void) instance;
  (void) report_id;
  (void) report_type;
  (void) buffer;
  (void) reqlen;

  return 0;
}

// Invoked when received SET_REPORT control request or
// received data on OUT endpoint ( Report ID = 0, Type = 0 )
void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize)
{
  (void) instance;
  (void) report_id;
  (void) report_type;
  (void) buffer;
  (void) bufsize;
  
  // This is where you would handle things like Keyboard CAPS LOCK LEDs
}