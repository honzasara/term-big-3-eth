#include "esp32_saric_virtual-outputs.h"
#include "esp_log.h"
#include "main.h"


/*
 *
 virtual_output{output_id="107",type="thermctl",device="PRACOVNA",name="kuchyba",value="state"} 0.0
 virtual_output{output_id="107",type="thermctl",device="PRACOVNA",name="kuchyba",value="type"} 0.0 
 */


void virtual_outputs_metrics(char *str, uint8_t chunk_start, uint8_t chunk_stop)
{
    char str1[256];
    struct_output output;
    for (uint8_t id = chunk_start; id < chunk_stop; id++)
        {
        output_get_all(id, &output);
        if (output.used != 0)
            {
	    sprintf(str1, "virtual_output_state{device=\"%s\",name=\"%s\",output_id=\"%d\",type=\"termbig\"} %d\n", \
                   device.nazev, output.name, output.id, output.state);
	    strcat(str, str1);

	    sprintf(str1, "virtual_output_mode{device=\"%s\",name=\"%s\",output_id=\"%d\",type=\"termbig\"} %d\n", \
                   device.nazev, output.name, output.id, output.mode_now);
            strcat(str, str1);
	    }
        }
}

void virtual_outputs_dump_configuration(char *str)
{
    char itm_list[4];
    struct_output output;
    cJSON *root;
    cJSON *outputs[MAX_OUTPUT];

    root = cJSON_CreateObject();


    for (uint8_t id = 0; id < MAX_OUTPUT; id++)
    {
        output_get_all(id, &output);
	if (output.used != 0)
	    {
	    itoa(id, itm_list, 10);
	    cJSON_AddItemToObject(root, itm_list, outputs[id] = cJSON_CreateObject());
	    cJSON_AddItemToObject(outputs[id], "used", cJSON_CreateNumber(output.used));
            cJSON_AddItemToObject(outputs[id], "name", cJSON_CreateString(output.name));
	    cJSON_AddItemToObject(outputs[id], "outputs", cJSON_CreateNumber(output.outputs));
	    cJSON_AddItemToObject(outputs[id], "mode-enable", cJSON_CreateNumber(output.mode_enable));
	    cJSON_AddItemToObject(outputs[id], "type", cJSON_CreateNumber(output.type));
	    cJSON_AddItemToObject(outputs[id], "virtual-id", cJSON_CreateNumber(output.id));
	    cJSON_AddItemToObject(outputs[id], "period", cJSON_CreateNumber(output.period));
	    cJSON_AddItemToObject(outputs[id], "state-time-max", cJSON_CreateNumber(output.state_time_max));
	    cJSON_AddItemToObject(outputs[id], "after-start-state", cJSON_CreateNumber(output.after_start_state));
	    cJSON_AddItemToObject(outputs[id], "after-start-mode", cJSON_CreateNumber(output.after_start_mode));
	    }
    }
    
    char *json = cJSON_Print(root);
    cJSON_Delete(root);
    strcpy(str, json);
}


void virtual_outputs_set_all_from_json(uint8_t idx, cJSON *json_virtual_outputs)
{
    uint8_t ret = 0;
    cJSON *tmp = NULL;

    struct_output output;

    output_get_all(idx, &output);

    /// nastaveni nazvu vystupu
    tmp = cJSON_GetObjectItem(json_virtual_outputs, "name");
    if (tmp)
        {
	strncpy(output.name, tmp->valuestring, 8);
	ESP_LOGI(TAG, "Set new name %d: %s", idx, output.name);
	ret = 1;
	}
    /// nastaveni jestli je aktivni
    tmp = cJSON_GetObjectItem(json_virtual_outputs, "used");
    if (tmp)
        {
        output.used = tmp->valueint;
	ESP_LOGI(TAG, "Set new used %d: %d", idx, output.used);
        ret = 1;
        }
    /// asociovani fyzickych vystupu
    tmp = cJSON_GetObjectItem(json_virtual_outputs, "outputs");
    if (tmp)
        {
        output.outputs = tmp->valueint;
	ESP_LOGI(TAG, "Set new outputs %d: %d", idx, output.outputs);
        ret = 1;
        }
    /// jake mody podporuje tento vystup
    tmp = cJSON_GetObjectItem(json_virtual_outputs, "mode-enable");
    if (tmp)
        {
        output.mode_enable = tmp->valueint;
	ESP_LOGI(TAG, "Set new mode-enable %d: %d", idx, output.mode_enable);
        ret = 1;
        }
    /// typ vystupu
    tmp = cJSON_GetObjectItem(json_virtual_outputs, "type");
    if (tmp)
        {
        output.type = tmp->valueint;
	ESP_LOGI(TAG, "Set new type %d: %d", idx, output.type);
        ret = 1;
        }
    /// globalni virtualni id
    tmp = cJSON_GetObjectItem(json_virtual_outputs, "virtual-id");
    if (tmp)
        {
        output.id = tmp->valueint;
	ESP_LOGI(TAG, "Set new virtual-id %d: %d", idx, output.id);
        ret = 1;
        }
    /// perioda pro PWM vystupy
    tmp = cJSON_GetObjectItem(json_virtual_outputs, "period");
    if (tmp)
        {
        output.period = tmp->valueint;
	ESP_LOGI(TAG, "Set new period %d: %d", idx, output.period);
        ret = 1;
        }
    /// maximalni cas pro casovaci vystup
    tmp = cJSON_GetObjectItem(json_virtual_outputs, "state-time-max");
    if (tmp)
        {
        output.state_time_max = tmp->valueint;
	ESP_LOGI(TAG, "Set new state-time-max %d: %d", idx, output.state_time_max);
        ret = 1;
        }

    /// jaky je vychozi stav po startu
    tmp = cJSON_GetObjectItem(json_virtual_outputs, "after-start-state");
    if (tmp)
        {
        output.after_start_state = tmp->valueint;
	ESP_LOGI(TAG, "Set new after-start-state %d: %d", idx, output.after_start_state);
        ret = 1;
        }

    /// jaky je vychozi mod po startu
    tmp = cJSON_GetObjectItem(json_virtual_outputs, "after-start-mode");
    if (tmp)
        {
        output.after_start_mode = tmp->valueint;
	ESP_LOGI(TAG, "Set new after-start-mode %d: %d", idx, output.after_start_mode);
        ret = 1;
        }

    tmp = cJSON_GetObjectItem(json_virtual_outputs, "save-eeprom");
    if (tmp)
	{
	if (tmp->valueint == 1)
	    ret = 2;
	ESP_LOGI(TAG, "Save EEPROM %d: %d", idx, tmp->valueint);
	}

    tmp = cJSON_GetObjectItem(json_virtual_outputs, "load-eeprom");
    if (tmp)
        {
        if (tmp->valueint == 1)
            ret = 3;
        ESP_LOGI(TAG, "Load EEPROM %d: %d", idx, tmp->valueint);
        }




    if (ret == 1)
        {
	output_set_all(idx, output);
	ESP_LOGI(TAG, "Update output %d", idx);
	}

    if (ret == 2)
	{
	output_set_all(idx, output);
	output_set_all_eeprom(idx, output);
	ESP_LOGI(TAG, "Save output to EEPROM %d", idx);
	}

    if (ret == 3)
        {
        output_get_all_eeprom(idx, &output);
	output_set_all(idx, output);
        ESP_LOGI(TAG, "Load output to EEPROM %d", idx);
        }

}


