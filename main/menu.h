#ifndef __SARIC_DISPLAY_MENU_H
#define __SARIC_DISPLAY_MENU_H

#include "esp_http_client.h"
#include "mqtt_client.h"
#include "esp_netif.h"
#include "esp_eth.h"
#include "esp_wifi.h"
#include "esp32_saric_mqtt_network.h"
#include "driver/i2c.h"
#include "main.h"
#include "saric_virtual_outputs.h"

#include "menu-outputs.h"

#define INDEX_MENU_DEFAULT 0
#define INDEX_MENU_MAIN 1
#define INDEX_MENU_SET_IP 2
#define INDEX_MENU_SET_MASK 4
#define INDEX_MENU_SET_GW 5
#define INDEX_MENU_SET_DNS 6
#define INDEX_MENU_SET_MQTT 7
#define INDEX_MENU_SET_MQTT_USER 8
#define INDEX_MENU_SET_MQTT_PASS 9
#define INDEX_MENU_SET_BOOT_URI 10
#define INDEX_MENU_SET_DEVICE_NAME 11
#define INDEX_MENU_ABOUT_DEVICE 12
#define INDEX_MENU_SET_WIFI_ESSID 13
#define INDEX_MENU_SET_WIFI_PASS 14
#define INDEX_MENU_SET_DHCP 15
#define INDEX_MENU_SET_ENABLE_INTERFACE 16
#define INDEX_MENU_SET_OUTPUTS 17
#define INDEX_MENU_SET_OUTPUTS_DETAIL 18
#define INDEX_MENU_SET_OUTPUT_NAME 19

#define MENU_SET_NETWORK_PARAMS_SHOW 0
#define MENU_SET_NETWORK_PARAMS_SETTING 1

#define MENU_SET_OUTPUTS_HW_SHOW 0
#define MENU_SET_OUTPUTS_HW_SETTING 1

#define MAX_MENUS 30
extern uint8_t menu_index;
extern uint8_t menu_params_1[MAX_MENUS];
extern uint8_t menu_params_2[MAX_MENUS];
extern uint8_t menu_params_3[MAX_MENUS];
extern uint8_t menu_params_4[MAX_MENUS];
extern uint8_t display_redraw;


uint8_t display_menu(uint8_t button_up, uint8_t button_down, uint8_t button_enter, uint8_t redraw);
uint8_t display_menu_main(uint8_t button_up, uint8_t button_down, uint8_t button_enter);
uint8_t display_menu_default(uint8_t button_up, uint8_t button_down, uint8_t button_enter);
uint8_t display_menu_set_ip(uint8_t button_up, uint8_t button_down, uint8_t button_enter, uint8_t *ip, char *label);
uint8_t display_menu_set_name(uint8_t button_up, uint8_t button_down, uint8_t button_enter, char *name, char *label, uint8_t max_len, uint8_t return_menu_index);

uint8_t display_menu_about_device(uint8_t button_up, uint8_t button_down, uint8_t button_enter);
#endif
