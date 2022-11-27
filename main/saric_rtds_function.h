#ifndef __SARIC_RTDS_H
#define __SARIC_RTDS_H

#include "EEPROM.h"
#include <mqtt_client.h>
#include "pgmspace.h"
#include "esp32_saric_mqtt_network.h"

#if !defined(MAX_RTDS)
#define MAX_RTDS 6
#endif

#define RTDS_DEVICE_STRING_LEN 18
#define RTDS_DEVICE_TOTAL_LEN 20
#define RTDS_DEVICE_ACTIVE_BYTE_POS 19

#define RTDS_REMOTE_TYPE_TEMP 1
#define RTDS_REMOTE_TYPE_FREE 255


#if !defined(remote_tds_name0)
#define remote_tds_name0  1000
#endif

#if !defined(ram_remote_tds_store_first)
#define ram_remote_tds_store_first 100
#endif

#define ram_remote_tds_store_size 4
#define ram_remote_tds_store_data_low ram_remote_tds_store_first ////100
#define ram_remote_tds_store_data_high ram_remote_tds_store_first + 1 ///101
#define ram_remote_tds_store_last_update ram_remote_tds_store_first + 2 ///102
#define ram_remote_tds_store_type ram_remote_tds_store_first + 3 ////103

#define ram_remote_tds_store_last (ram_remote_tds_store_data_low + (ram_remote_tds_store_size * MAX_RTDS))
#define remote_tds_last remote_tds_name0 + (RTDS_DEVICE_TOTAL_LEN * MAX_RTDS)


#pragma message( "ram_remote_tds_store_last  = " XSTR(ram_remote_tds_store_last))
#pragma message( "remote_tds_last  = " XSTR(remote_tds_last))

const char new_text_slash_rtds_slash[] PROGMEM = "/rtds/";
const char new_text_slash_rtds_control_list[] PROGMEM = "/rtds-control/list";


void remote_tds_get_complete(uint8_t idx, uint8_t *active, char *name);
void remote_tds_set_complete(uint8_t idx, uint8_t active, char *name);
void remote_tds_set_name(uint8_t idx,  char *name);
void remote_tds_get_active(uint8_t idx, uint8_t *active);
void remote_tds_set_active(uint8_t idx, uint8_t active);
void remote_tds_clear(uint8_t idx);
void remote_tds_subscibe_topic(esp_mqtt_client_handle_t mqtt_client, uint8_t idx);
void remote_tds_unsubscibe_topic(esp_mqtt_client_handle_t mqtt_client, uint8_t idx);
uint8_t remote_tds_find_free(void);
uint8_t remote_tds_name_exist(char *name);
void remote_tds_set_type(uint8_t idx, uint8_t type);
uint8_t remote_tds_get_type(uint8_t idx);
void remote_tds_set_data(uint8_t idx, int value);
int remote_tds_get_data(uint8_t idx);
uint8_t remote_tds_get_last_update(uint8_t idx);
void remote_tds_set_last_update(uint8_t idx, uint8_t value);
void remote_tds_inc_last_update(uint8_t idx);
void remote_tds_update_last_update(void);

uint8_t count_use_rtds(uint8_t *count_type_temp);
#endif
