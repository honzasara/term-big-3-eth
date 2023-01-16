#ifndef __SARIC_ESP32_VIRTUAL_OUTPUTS_H
#define __SARIC_ESP32_VIRTUAL_OUTPUTS_H

#include "saric_virtual_outputs.h"
#include "cJSON.h"
#include "esp32_saric_mqtt_network.h"

void virtual_outputs_dump_configuration(char *str);
void virtual_outputs_set_all_from_json(uint8_t idx, cJSON *tmp_json);

void virtual_outputs_metrics(char *str, uint8_t chunk_start, uint8_t chunk_stop);
#endif
