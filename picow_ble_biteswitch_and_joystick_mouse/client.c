/**
 * Copyright (c) 2023 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include "btstack.h"
#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"

#include <stdlib.h>
#include <string.h>
// #include "bsp/board_api.h"
// #include "tusb.h"
// #include "usb_descriptors.h"
#include "usb_bridge.h"

#if 1
#define DEBUG_LOG(...) printf(__VA_ARGS__)
#else
#define DEBUG_LOG(...)
#endif

#define LED_QUICK_FLASH_DELAY_MS 100
#define LED_SLOW_FLASH_DELAY_MS 1000

typedef enum {
    TC_OFF,
    TC_IDLE,
    TC_W4_SCAN_RESULT,
    TC_W4_CONNECT,
    TC_W4_SERVICE_RESULT,
    TC_W4_CHARACTERISTIC_RESULT,
    TC_W4_ENABLE_NOTIFICATIONS_COMPLETE,
    TC_W4_READY
} gc_state_t;

static btstack_packet_callback_registration_t hci_event_callback_registration;
static btstack_timer_source_t heartbeat;

// --- BITESWITCH VARIABLES ---
static gc_state_t state_biteswitch = TC_OFF;
static bd_addr_t server_addr_biteswitch;
static bd_addr_type_t server_addr_type_biteswitch;
static hci_con_handle_t connection_handle_biteswitch = HCI_CON_HANDLE_INVALID; // FIX: Init to INVALID
static gatt_client_service_t server_service_biteswitch;
static gatt_client_characteristic_t server_characteristic_biteswitch;
static bool listener_registered_biteswitch;
static gatt_client_notification_t notification_listener_biteswitch;
static bool biteswitch_discovery_pending = false; // FIX: New flag

bool biteswitch_active = false;
bool joystick_active = false;

// --- JOYSTICK VARIABLES ---
static gc_state_t state_joystick = TC_OFF;
static bd_addr_t server_addr_joystick;
static bd_addr_type_t server_addr_type_joystick;
static hci_con_handle_t connection_handle_joystick = HCI_CON_HANDLE_INVALID; // FIX: Init to INVALID
static gatt_client_service_t server_service_joystick;
static gatt_client_characteristic_t server_characteristic_joystick;
static bool listener_registered_joystick;
static gatt_client_notification_t notification_listener_joystick;
static bool joystick_discovery_pending = false; // FIX: New flag

static btstack_timer_source_t usb_timer;

int global_biteswitch_value = 0;
uint16_t global_joystick_x = 0;
uint16_t global_joystick_y = 0;

static void client_start(gc_state_t *state){
    DEBUG_LOG("Start scanning!\n");
    *state = TC_W4_SCAN_RESULT;
    gap_set_scan_parameters(1,0x0030, 0x0030);
    gap_start_scan();
}

static bool advertisement_report_contains_service(uint16_t service, uint8_t *advertisement_report){
    const uint8_t * adv_data = gap_event_advertising_report_get_data(advertisement_report);
    uint8_t adv_len  = gap_event_advertising_report_get_data_length(advertisement_report);
    ad_context_t context;
    for (ad_iterator_init(&context, adv_len, adv_data) ; ad_iterator_has_more(&context) ; ad_iterator_next(&context)){
        uint8_t data_type = ad_iterator_get_data_type(&context);
        uint8_t data_size = ad_iterator_get_data_len(&context);
        const uint8_t * data = ad_iterator_get_data(&context);
        switch (data_type){
            case BLUETOOTH_DATA_TYPE_COMPLETE_LIST_OF_16_BIT_SERVICE_CLASS_UUIDS:
            case BLUETOOTH_DATA_TYPE_INCOMPLETE_LIST_OF_16_BIT_SERVICE_CLASS_UUIDS:
                for (int i = 0; i < data_size; i += 2) {
                    uint16_t type = little_endian_read_16(data, i);
                    if (type == service) return true;
                }
            default:
                break;
        }
    }
    return false;
}

// --- FIX: Unified Handler for GATT Events ---
static void handle_gatt_client_event(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size) {
    UNUSED(packet_type);
    UNUSED(channel);
    UNUSED(size);

    uint16_t event_handle = 0;
    switch (hci_event_packet_get_type(packet)) {
        case GATT_EVENT_SERVICE_QUERY_RESULT:
            event_handle = gatt_event_service_query_result_get_handle(packet);
            if (event_handle == connection_handle_biteswitch && state_biteswitch == TC_W4_SERVICE_RESULT) {
                DEBUG_LOG("Storing biteswitch service\n");
                gatt_event_service_query_result_get_service(packet, &server_service_biteswitch);
            } else if (event_handle == connection_handle_joystick && state_joystick == TC_W4_SERVICE_RESULT) {
                DEBUG_LOG("Storing joystick service\n");
                gatt_event_service_query_result_get_service(packet, &server_service_joystick);
            }
            break;
        case GATT_EVENT_QUERY_COMPLETE: {
            event_handle = gatt_event_query_complete_get_handle(packet);
            uint8_t att_status = gatt_event_query_complete_get_att_status(packet);
            
            // --- BITESWITCH LOGIC ---
            if (event_handle == connection_handle_biteswitch) {
                if (att_status != ATT_ERROR_SUCCESS) {
                    printf("Biteswitch GATT Error 0x%02x in state %d\n", att_status, state_biteswitch);
                    // gap_disconnect(connection_handle_biteswitch); // Optional: Disconnect on error
                    break;
                }
                
                if (state_biteswitch == TC_W4_SERVICE_RESULT) {
                    state_biteswitch = TC_W4_CHARACTERISTIC_RESULT;
                    DEBUG_LOG("Search for alert level characteristic.\n");
                    gatt_client_discover_characteristics_for_service_by_uuid16(handle_gatt_client_event, connection_handle_biteswitch, &server_service_biteswitch, ORG_BLUETOOTH_CHARACTERISTIC_ALERT_LEVEL);
                } else if (state_biteswitch == TC_W4_CHARACTERISTIC_RESULT) {
                    if (!listener_registered_biteswitch) {
                        listener_registered_biteswitch = true;
                        DEBUG_LOG("Registering biteswitch notification listener for handle %x, char %x\n", connection_handle_biteswitch, server_characteristic_biteswitch.value_handle);
                        gatt_client_listen_for_characteristic_value_updates(&notification_listener_biteswitch, handle_gatt_client_event, connection_handle_biteswitch, &server_characteristic_biteswitch);
                    }
                    DEBUG_LOG("Enable notify on biteswitch characteristic.\n");
                    state_biteswitch = TC_W4_ENABLE_NOTIFICATIONS_COMPLETE;
                    gatt_client_write_client_characteristic_configuration(handle_gatt_client_event, connection_handle_biteswitch,
                        &server_characteristic_biteswitch, GATT_CLIENT_CHARACTERISTICS_CONFIGURATION_NOTIFICATION);
                } else if (state_biteswitch == TC_W4_ENABLE_NOTIFICATIONS_COMPLETE) {
                    DEBUG_LOG("Biteswitch notifications enabled, ATT status 0x%02x\n", att_status);
                    state_biteswitch = TC_W4_READY;
                }
            }
            
            // --- JOYSTICK LOGIC ---
            if (event_handle == connection_handle_joystick) {
                if (att_status != ATT_ERROR_SUCCESS) {
                    printf("Joystick GATT Error 0x%02x in state %d\n", att_status, state_joystick);
                    // gap_disconnect(connection_handle_joystick); // Optional: Disconnect on error
                    break;
                }

                if (state_joystick == TC_W4_SERVICE_RESULT) {
                    state_joystick = TC_W4_CHARACTERISTIC_RESULT;
                    DEBUG_LOG("Search for analog characteristic.\n");
                    gatt_client_discover_characteristics_for_service_by_uuid16(handle_gatt_client_event, connection_handle_joystick, &server_service_joystick, ORG_BLUETOOTH_CHARACTERISTIC_ANALOG);
                } else if (state_joystick == TC_W4_CHARACTERISTIC_RESULT) {
                    if (!listener_registered_joystick) {
                        listener_registered_joystick = true;
                        DEBUG_LOG("Registering joystick notification listener for handle %x, char %x\n", connection_handle_joystick, server_characteristic_joystick.value_handle);
                        gatt_client_listen_for_characteristic_value_updates(&notification_listener_joystick, handle_gatt_client_event, connection_handle_joystick, &server_characteristic_joystick);
                    }
                    DEBUG_LOG("Enable notify on joystick characteristic.\n");
                    state_joystick = TC_W4_ENABLE_NOTIFICATIONS_COMPLETE;
                    gatt_client_write_client_characteristic_configuration(handle_gatt_client_event, connection_handle_joystick,
                        &server_characteristic_joystick, GATT_CLIENT_CHARACTERISTICS_CONFIGURATION_NOTIFICATION);
                } else if (state_joystick == TC_W4_ENABLE_NOTIFICATIONS_COMPLETE) {
                    DEBUG_LOG("Joystick notifications enabled, ATT status 0x%02x\n", att_status);
                    state_joystick = TC_W4_READY;
                }
            }
            break;
        }
        case GATT_EVENT_CHARACTERISTIC_QUERY_RESULT:
            event_handle = gatt_event_characteristic_query_result_get_handle(packet);
            if (event_handle == connection_handle_biteswitch && state_biteswitch == TC_W4_CHARACTERISTIC_RESULT) {
                DEBUG_LOG("Storing biteswitch characteristic\n");
                gatt_event_characteristic_query_result_get_characteristic(packet, &server_characteristic_biteswitch);
            } else if (event_handle == connection_handle_joystick && state_joystick == TC_W4_CHARACTERISTIC_RESULT) {
                DEBUG_LOG("Storing joystick characteristic\n");
                gatt_event_characteristic_query_result_get_characteristic(packet, &server_characteristic_joystick);
            }
            break;
        case GATT_EVENT_NOTIFICATION: {
            event_handle = gatt_event_notification_get_handle(packet);
            uint16_t value_length = gatt_event_notification_get_value_length(packet);
            const uint8_t *value = gatt_event_notification_get_value(packet);
            if (event_handle == connection_handle_biteswitch && state_biteswitch == TC_W4_READY) {
                if (value_length == 2) {
                    int sensor_value = (int) little_endian_read_16(value, 0);
                    printf("[Biteswitch] Value: %d\n", sensor_value);
                    global_biteswitch_value = sensor_value;
                }
            } else if (event_handle == connection_handle_joystick && state_joystick == TC_W4_READY) {
                if (value_length == 4) {
                    uint16_t joystick_x = little_endian_read_16(value, 0);
                    uint16_t joystick_y = little_endian_read_16(value, 2);
                    printf("[Joystick] X: %u, Y: %u\n", joystick_x, joystick_y);
                    global_joystick_x = joystick_x;
                    global_joystick_y = joystick_y;
                }
            }
            break;
        }
    }
}

// --- FIX: Helper to retry discovery if busy ---
static void attempt_gatt_discovery(void) {
    uint8_t status;

    if (biteswitch_discovery_pending && connection_handle_biteswitch != HCI_CON_HANDLE_INVALID) {
        status = gatt_client_discover_primary_services_by_uuid16(handle_gatt_client_event, connection_handle_biteswitch, ORG_BLUETOOTH_SERVICE_BINARY_SENSOR);
        if (status == ERROR_CODE_SUCCESS) {
            printf("Biteswitch service discovery started.\n");
            biteswitch_discovery_pending = false;
        } else {
             // Print error to debug why it fails (0x1C = Busy)
             printf("Biteswitch discovery deferred (status 0x%02x)\n", status);
        }
    }

    if (joystick_discovery_pending && connection_handle_joystick != HCI_CON_HANDLE_INVALID) {
        status = gatt_client_discover_primary_services_by_uuid16(handle_gatt_client_event, connection_handle_joystick, ORG_BLUETOOTH_SERVICE_AUTOMATION_IO);
        if (status == ERROR_CODE_SUCCESS) {
            printf("Joystick service discovery started.\n");
            joystick_discovery_pending = false;
        } else {
             // Print error to debug why it fails (0x1C = Busy)
             printf("Joystick discovery deferred (status 0x%02x)\n", status);
        }
    }
}

static void hci_event_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size) {
    UNUSED(size);
    UNUSED(channel);
    bd_addr_t local_addr;
    if (packet_type != HCI_EVENT_PACKET) return;

    uint8_t event_type = hci_event_packet_get_type(packet);
    switch(event_type){
        case BTSTACK_EVENT_STATE:
            if (btstack_event_state_get_state(packet) == HCI_STATE_WORKING) {
                gap_local_bd_addr(local_addr);
                printf("BTstack up and running on %s.\n", bd_addr_to_str(local_addr));
                client_start(&state_biteswitch);
                client_start(&state_joystick);
            }
            break;
        case GAP_EVENT_ADVERTISING_REPORT:
            printf("Advertisement received.\n");
            if (state_biteswitch != TC_W4_SCAN_RESULT && state_joystick != TC_W4_SCAN_RESULT) return;
            
            if (advertisement_report_contains_service(ORG_BLUETOOTH_SERVICE_AUTOMATION_IO, packet)) {
                joystick_active = true;
            } else {
                joystick_active = false;
            }

            if (advertisement_report_contains_service(ORG_BLUETOOTH_SERVICE_BINARY_SENSOR, packet)) {
                biteswitch_active = true;
            } else {
                biteswitch_active = false;
            }

            if (biteswitch_active && state_biteswitch == TC_W4_SCAN_RESULT){
                gap_event_advertising_report_get_address(packet, server_addr_biteswitch);
                server_addr_type_biteswitch = gap_event_advertising_report_get_address_type(packet);
                state_biteswitch = TC_W4_CONNECT;
                printf("Connecting to Biteswitch %s.\n", bd_addr_to_str(server_addr_biteswitch));
                gap_connect(server_addr_biteswitch, server_addr_type_biteswitch);
            }

            if (joystick_active && state_joystick == TC_W4_SCAN_RESULT){
                gap_event_advertising_report_get_address(packet, server_addr_joystick);
                server_addr_type_joystick = gap_event_advertising_report_get_address_type(packet);
                state_joystick = TC_W4_CONNECT;
                printf("Connecting to Joystick %s.\n", bd_addr_to_str(server_addr_joystick));
                gap_connect(server_addr_joystick, server_addr_type_joystick);
            }
            break;
        case HCI_EVENT_LE_META:
            switch (hci_event_le_meta_get_subevent_code(packet)) {
                case HCI_SUBEVENT_LE_CONNECTION_COMPLETE: {
                    bd_addr_t event_addr;
                    gap_subevent_le_connection_complete_get_peer_address(packet, event_addr);
                    hci_con_handle_t event_handle = hci_subevent_le_connection_complete_get_connection_handle(packet);

                    if (state_biteswitch == TC_W4_CONNECT && memcmp(event_addr, server_addr_biteswitch, 6) == 0) {
                        connection_handle_biteswitch = event_handle;
                        DEBUG_LOG("[Connect] biteswitch handle %x\n", event_handle);
                        state_biteswitch = TC_W4_SERVICE_RESULT;
                        // FIX: Do not call discovery here. Set flag and let heartbeat handle it.
                        biteswitch_discovery_pending = true;
                    } 
                    else if (state_joystick == TC_W4_CONNECT && memcmp(event_addr, server_addr_joystick, 6) == 0) {
                        connection_handle_joystick = event_handle;
                        DEBUG_LOG("[Connect] joystick handle %x\n", event_handle);
                        state_joystick = TC_W4_SERVICE_RESULT;
                        // FIX: Do not call discovery here. Set flag and let heartbeat handle it.
                        joystick_discovery_pending = true;
                    }
                    
                    // Optimization: If both are connecting/connected, stop scanning
                    if (state_biteswitch >= TC_W4_CONNECT && state_joystick >= TC_W4_CONNECT) {
                        gap_stop_scan(); // Uncomment if you want to stop scanning after finding both
                    }
                    break;
                }
            }
            break;
        case HCI_EVENT_DISCONNECTION_COMPLETE:
            uint16_t disconnection_handle = hci_event_disconnection_complete_get_connection_handle(packet);
            if (disconnection_handle == connection_handle_biteswitch){
                printf("Disconnected Biteswitch\n");
                connection_handle_biteswitch = HCI_CON_HANDLE_INVALID;
                biteswitch_discovery_pending = false;
                client_start(&state_biteswitch);
                global_biteswitch_value = 0;
            }
            else if (disconnection_handle == connection_handle_joystick){
                printf("Disconnected Joystick\n");
                connection_handle_joystick = HCI_CON_HANDLE_INVALID;
                joystick_discovery_pending = false;
                client_start(&state_joystick);
                global_joystick_x = 0;
                global_joystick_y = 0;
            }
            break;
    }
}

static void heartbeat_handler(struct btstack_timer_source *ts) {
    // --- FIX: Attempt pending discoveries periodically ---
    attempt_gatt_discovery();
    // ---------------------------------------------------

    static bool quick_flash;
    static bool led_on = true;

    led_on = !led_on; 
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, led_on);
    
    // Quick flash if either is connected
    bool any_connected = (connection_handle_biteswitch != HCI_CON_HANDLE_INVALID) || (connection_handle_joystick != HCI_CON_HANDLE_INVALID);
    
    if (any_connected && led_on) {
        quick_flash = !quick_flash;
    } else if (!any_connected) {
        quick_flash = false;
    }

    btstack_run_loop_set_timer(ts, (led_on || quick_flash) ? LED_QUICK_FLASH_DELAY_MS : LED_SLOW_FLASH_DELAY_MS);
    btstack_run_loop_add_timer(ts);
}

// This runs every 10ms to keep USB alive
static void usb_timer_handler(btstack_timer_source_t *ts) {
    usb_bridge_task();
    // usb_bridge_send_mouse(global_biteswitch_value == 1 ? BRIDGE_MOUSE_BTN_LEFT : 0, global_joystick_x, global_joystick_y, 0, 0);
    usb_bridge_send_mouse(global_biteswitch_value, global_joystick_x, global_joystick_y, 0, 0); // global_biteswitch_value matches up with constants defined in usb_bridge.h
    
    // Reschedule for 10ms later
    btstack_run_loop_set_timer(ts, 10);
    btstack_run_loop_add_timer(ts);
}

int main() {
    stdio_init_all();
    // while(!stdio_usb_connected());

    usb_bridge_init();
    
    if (cyw43_arch_init()) {
        printf("failed to initialise cyw43_arch\n");
        return -1;
    }

    l2cap_init();
    sm_init();
    sm_set_io_capabilities(IO_CAPABILITY_NO_INPUT_NO_OUTPUT);
    att_server_init(NULL, NULL, NULL);
    gatt_client_init();

    hci_event_callback_registration.callback = &hci_event_handler;
    hci_add_event_handler(&hci_event_callback_registration);

    heartbeat.process = &heartbeat_handler;
    btstack_run_loop_set_timer(&heartbeat, LED_SLOW_FLASH_DELAY_MS);
    btstack_run_loop_add_timer(&heartbeat);

    usb_timer.process = &usb_timer_handler;
    btstack_run_loop_set_timer(&usb_timer, 10);
    btstack_run_loop_add_timer(&usb_timer);

    hci_power_control(HCI_POWER_ON);

    btstack_run_loop_execute();
    return 0;
}
