#include "saric_metrics.h"


void prom_metric_up(char *str)
{
   char str1[128];
   char str2[32];
   uint8_t conn = 0;
   sprintf(str1, "up{device=\"%s\"} 1\n", device.nazev);
   strcat(str, str1);

   strcpy(str2, "mqtt://");
   createString(str1, '.', device.mqtt_server, 4, 10, 1);
   strcat(str2, str1);

   if (mqtt_link == MQTT_EVENT_CONNECTED)
	   conn = 1;
   if (mqtt_link == MQTT_EVENT_DISCONNECTED)
           conn = 2;
   if (mqtt_link == MQTT_EVENT_ERROR)
           conn = 3;
   sprintf(str1, "mqtt_connect{device=\"%s\",mqtt_server=\"%s\"} %d\n", device.nazev, str2, conn);

   conn = 0;
   if (wifi_link == WIFI_EVENT_STA_CONNECTED)
	   conn = 1;
   if (wifi_link == WIFI_EVENT_STA_DISCONNECTED)
	   conn = 2;
   sprintf(str1, "wifi_connect{device=\"%s\", essid=\"%s\"} %d\n", device.nazev, device.wifi_essid, conn);
   strcat(str, str1);

   conn = 0;
   if (eth_link == ETHERNET_EVENT_CONNECTED)
	   conn = 1;
   if (eth_link == ETHERNET_EVENT_DISCONNECTED)
	   conn = 2;
   sprintf(str1, "eth_connect{device=\"%s\"} %d\n", device.nazev, conn);
   strcat(str, str1);

   sprintf(str1, "selftest{device=\"%s\"} 1\n", device.nazev);
   strcat(str, str1); 
}



void prom_metric_device_status(char *str)
{
   char str1[64];
   uint32_t time_now = 0;
   sprintf(str1, "uptime{device=\"%s\"} %ld\n", device.nazev, uptime);
   strcat(str, str1);

   sprintf(str1, "wifi_rssi{device=\"%s\", essid=\"%s\"} %d\n", device.nazev, device.wifi_essid, wifi_rssi);
   strcat(str, str1);

   sprintf(str1, "heap{device=\"%s\"} %d\n", device.nazev, free_heap);
   strcat(str, str1);

   time_now = DateTime(__DATE__, __TIME__).unixtime();
   sprintf(str1, "build{device=\"%s\"} %ld\n", device.nazev, time_now);
   strcat(str, str1);

   sprintf(str1, "know_devices{device=\"%s\"} %d\n", device.nazev, count_know_mqtt);
   strcat(str, str1);

   sprintf(str1, "mqtt_send_message{device=\"%s\"} %d\n", device.nazev, mqtt_send_message);
   strcat(str, str1);

   sprintf(str1, "mqtt_receive_message{device=\"%s\"} %d\n", device.nazev, mqtt_receive_message);
   strcat(str, str1);

   sprintf(str1, "mqtt_process_message{device=\"%s\"} %d\n", device.nazev, mqtt_process_message);
   strcat(str, str1);

   sprintf(str1, "mqtt_error{device=\"%s\"} %d\n", device.nazev, mqtt_error);
   strcat(str, str1);

   sprintf(str1, "rtds_count{device=\"%s\"} %d\n", device.nazev, use_rtds);
   strcat(str, str1);

   sprintf(str1, "tds_count{device=\"%s\"} %d\n", device.nazev, use_tds);
   strcat(str, str1);
}


void prom_metric_know_device(char *str)
{
  char str1[128];
  char str_type[16];
  for (uint8_t idx = 0; idx < MAX_KNOW_MQTT_INTERNAL_RAM; idx++)
    if (know_mqtt[idx].type != TYPE_FREE)
    {
    if (know_mqtt[idx].type == TYPE_FREE)
	    strcpy(str_type, "TYPE_FREE");
    if (know_mqtt[idx].type == TYPE_THERMCTL)
            strcpy(str_type, "TYPE_THERMCTL");
    if (know_mqtt[idx].type == TYPE_TERMBIG)
            strcpy(str_type, "TYPE_TERMBIG");
    if (know_mqtt[idx].type == TYPE_BRANA)
            strcpy(str_type, "TYPE_BRANA");
    if (know_mqtt[idx].type == TYPE_MASTER)
            strcpy(str_type, "TYPE_MASTER");
    if (know_mqtt[idx].type == TYPE_MONITOR)
            strcpy(str_type, "TYPE_MONITOR");
    sprintf(str1, "know_device_last_update{remote_device=\"%s\",remote_type=\"%s\"} %d\n",know_mqtt[idx].device, str_type, know_mqtt[idx].last_update);
    strcat(str, str1);
    }
}

void json_device_config(char *str)
{
    char ip_uri[32];
    uint32_t time_now = 0;
    cJSON *root, *network;

    root = cJSON_CreateObject();
 
    cJSON_AddItemToObject(root, "network", network = cJSON_CreateObject());
   
    time_now = DateTime(__DATE__, __TIME__).unixtime();
    sprintf(ip_uri, "%ld", time_now);
    cJSON_AddItemToObject(network, "version", cJSON_CreateString(ip_uri));

    cJSON_AddItemToObject(network, "name", cJSON_CreateString(device.nazev));
    cJSON_AddItemToObject(network, "wifi_essid", cJSON_CreateString(device.wifi_essid));
    cJSON_AddItemToObject(network, "wifi_key", cJSON_CreateString(device.wifi_key));
    
    strcpy(ip_uri, "");
    createString(ip_uri, '.', device.mac, 6, 16, 1);
    cJSON_AddItemToObject(network, "ETH_MAC", cJSON_CreateString(ip_uri));

    strcpy(ip_uri, "");
    createString(ip_uri, '.', device.myIP, 4, 10, 1);
    cJSON_AddItemToObject(network, "IP", cJSON_CreateString(ip_uri));

    strcpy(ip_uri, "");
    createString(ip_uri, '.', device.myMASK, 4, 10, 1);
    cJSON_AddItemToObject(network, "Mask", cJSON_CreateString(ip_uri));

    strcpy(ip_uri, "");
    createString(ip_uri, '.', device.myGW, 4, 10, 1);
    cJSON_AddItemToObject(network, "GW", cJSON_CreateString(ip_uri));

    strcpy(ip_uri, "");
    createString(ip_uri, '.', device.myDNS, 4, 10, 1);
    cJSON_AddItemToObject(network, "DNS", cJSON_CreateString(ip_uri));

    strcpy(ip_uri, "");
    createString(ip_uri, '.', device.mqtt_server, 4, 10, 1);
    cJSON_AddItemToObject(network, "MQTT_IP", cJSON_CreateString(ip_uri));

    sprintf(ip_uri, "%d", device.mqtt_port);
    cJSON_AddItemToObject(network, "MQTT_Port", cJSON_CreateString(ip_uri));

    cJSON_AddItemToObject(network, "MQTT_User", cJSON_CreateString(device.mqtt_user));
    cJSON_AddItemToObject(network, "MQTT_Key", cJSON_CreateString(device.mqtt_key));

    strcpy(ip_uri, "");
    createString(ip_uri, '.', device.ntp_server, 4, 10, 1);
    cJSON_AddItemToObject(network, "NTP_IP", cJSON_CreateString(ip_uri));

    sprintf(ip_uri, "%d", device.http_port);
    cJSON_AddItemToObject(network, "HTTP_Port", cJSON_CreateString(ip_uri));

    cJSON_AddItemToObject(network, "Bootloader_URI", cJSON_CreateString(device.bootloader_uri));

    if (device.dhcp == DHCP_ENABLE)
        cJSON_AddItemToObject(network, "DHCP", cJSON_CreateString("Enable"));
    else
        cJSON_AddItemToObject(network, "DHCP", cJSON_CreateString("Disable"));

    switch (device.via)
	{
        case ENABLE_CONNECT_DISABLE:
	    cJSON_AddItemToObject(network, "via", cJSON_CreateString("ConnectDisable"));
	    break;
        case ENABLE_CONNECT_ETH:
	    cJSON_AddItemToObject(network, "via", cJSON_CreateString("ConnectEthernet"));
            break;
	case ENABLE_CONNECT_WIFI:
	    cJSON_AddItemToObject(network, "via", cJSON_CreateString("ConnectWifi"));
            break;
	case ENABLE_CONNECT_BOTH:
	    cJSON_AddItemToObject(network, "via", cJSON_CreateString("ConnectBoth"));
            break;
	default:
	    cJSON_AddItemToObject(network, "via", cJSON_CreateString("ConnectUnknow"));
            break;
	}

    char *json = cJSON_Print(root);
    cJSON_Delete(root);
    strcpy(str, json);
}




