#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#pragma GCC diagnostic ignored "-Wwrite-strings"


#include "main.h"

#include "nvs.h"
#include "nvs_flash.h"
#include "esp_ota_ops.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"


#include <stdio.h>
#include <string.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_netif.h"
#include "esp_eth.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "mqtt_client.h"

#include <esp_system.h>


#include "driver/gpio.h"
#include "esp_eth_enc28j60.h"
#include "driver/spi_master.h"
#include "lwip/ip4_addr.h"
#include <esp_http_server.h>
#include "driver/uart.h"
#include "driver/i2c.h"
#include "driver/gptimer.h"
#include "esp_timer.h"

#include "saric_tds_function.h"
#include "saric_rtds_function.h"
#include "esp32_saric_tds_function.h"

#include "SSD1306.h"
#include "Font5x8.h"
#include "esp32_saric_mqtt_network.h"
#include <time.h>
#include "saric_virtual_outputs.h"
#include "esp32_saric_virtual-outputs.h"
#include "saric_metrics.h"
#include "menu.h"
#include "ezButton.h"

i2c_port_t i2c_num;

uint32_t uptime = 0;

uint32_t _millis = 0;

uint8_t wifi_rssi = 0;
uint16_t free_heap = 0;

static EventGroupHandle_t s_wifi_event_group;
uint8_t s_wifi_retry_num = 0;

float internal_temp;

uint8_t use_tds = 0;
uint8_t use_rtds = 0;
uint8_t use_rtds_type_temp = 0;


char *TAG = "term-big-3.1-eth";
static httpd_handle_t start_webserver(void);
static esp_err_t prom_metrics_get_handler(httpd_req_t *req);

esp_mqtt_client_handle_t mqtt_client;

esp_netif_t *eth_netif;
esp_netif_t *wifi_netif;

uint32_t eth_link = 0;
uint32_t mqtt_link = 0;
uint32_t wifi_link = 0;


ezButton button_up(BUTTON_UP);
ezButton button_down(BUTTON_DOWN);
ezButton button_enter(BUTTON_ENTER);



esp_err_t http_event_handler(esp_http_client_event_t *evt)
{
    switch (evt->event_id) {
    case HTTP_EVENT_ERROR:
        ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
        break;
    case HTTP_EVENT_ON_CONNECTED:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
        break;
    case HTTP_EVENT_HEADER_SENT:
        ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
        break;
    case HTTP_EVENT_ON_HEADER:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
        break;
    case HTTP_EVENT_ON_DATA:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
        break;
    case HTTP_EVENT_ON_FINISH:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
        break;
    case HTTP_EVENT_DISCONNECTED:
        ESP_LOGD(TAG, "HTTP_EVENT_DISCONNECTED");
        break;
    case HTTP_EVENT_REDIRECT:
        ESP_LOGD(TAG, "HTTP_EVENT_REDIRECT");
        break;
    }
    return ESP_OK;
}


static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%ld", base, event_id);
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t) event_data;
    //esp_mqtt_client_handle_t client = event->client;
    //int msg_id;
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
	mqtt_reconnect();
	mqtt_link = MQTT_EVENT_CONNECTED;
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
	mqtt_link = MQTT_EVENT_DISCONNECTED;
        break;
    case MQTT_EVENT_SUBSCRIBED:
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
	mqtt_callback((char *) event->topic, (uint8_t *)event->data, event->topic_len, event->data_len);
        break;
    case MQTT_EVENT_ERROR:
	mqtt_link = MQTT_EVENT_ERROR;
        ESP_LOGE(TAG, "MQTT_EVENT_ERROR");
	mqtt_error++;
        break;
    case MQTT_EVENT_BEFORE_CONNECT:
	ESP_LOGE(TAG, "MQTT_EVENT_BEFORE_CONNECT");
	break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
}




/** Event handler for Ethernet events */
static void eth_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    uint8_t mac_addr[6] = {0};
    /* we can get the ethernet driver handle from event data */
    esp_eth_handle_t eth_handle = *(esp_eth_handle_t *)event_data;
    if (event_id ==  ETHERNET_EVENT_CONNECTED)
        {
        esp_eth_ioctl(eth_handle, ETH_CMD_G_MAC_ADDR, mac_addr);
        ESP_LOGI(TAG, "Ethernet Link Up");
        ESP_LOGI(TAG, "Ethernet HW Addr %02x:%02x:%02x:%02x:%02x:%02x",
                 mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
	eth_link = ETHERNET_EVENT_CONNECTED;
        }
    if (event_id == ETHERNET_EVENT_DISCONNECTED)
        {
        ESP_LOGI(TAG, "Ethernet Link Down");
	eth_link = ETHERNET_EVENT_DISCONNECTED;
        }
    if (event_id == ETHERNET_EVENT_START)
        {
        ESP_LOGI(TAG, "Ethernet Started");
        }
    if (event_id == ETHERNET_EVENT_STOP)
        {
        ESP_LOGI(TAG, "Ethernet Stopped");
        }
}


static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect();
    } 
    else 
	if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) 
	{
            if (s_wifi_retry_num < 5) 
	    {
                esp_wifi_connect();
                s_wifi_retry_num++;
                ESP_LOGI(TAG, "retry %d to connect to the AP", s_wifi_retry_num);
            } 
	    else 
	    {
                xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
                wifi_link = WIFI_EVENT_STA_DISCONNECTED;
            }
            ESP_LOGI(TAG,"connect to the AP fail");
        } 
	else 
            if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
	    {
                ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
                ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
                s_wifi_retry_num = 0;
                xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
                wifi_link = WIFI_EVENT_STA_CONNECTED;
            }
}



/* An HTTP POST handler */
static esp_err_t configurations_post_handler(httpd_req_t *req)
{
    cJSON *root;
    char resp_str[2048];
    size_t recv_size;
    int ret;
    recv_size = MIN(req->content_len, sizeof(resp_str));
    ret = httpd_req_recv(req, resp_str, recv_size);
    if (ret <= 0)
        {
        if (ret == HTTPD_SOCK_ERR_TIMEOUT)
            {
            httpd_resp_send_408(req);
            }
        return ESP_FAIL;
        }

    root = cJSON_Parse(resp_str);
    //char *json_text = cJSON_Print(root);

    cJSON *json_network = NULL;
    json_network = cJSON_GetObjectItem(root, "network");

    if (json_network)
        {
        ret = setting_network_json(json_network);
        save_setup_network();
        }

    cJSON_Delete(root);
    httpd_resp_sendstr(req, "OK\n");
    return ESP_OK;
}



/* An HTTP GET handler */
static esp_err_t prom_metrics_get_handler(httpd_req_t *req)
{
    static char resp_str[8196];
    strcpy(resp_str, "");
    httpd_resp_set_type(req, (const char*) "text/plain");

    prom_metric_up(resp_str);
    prom_metric_device_status(resp_str);
    if (strlen(resp_str))
        httpd_resp_send_chunk(req, resp_str, HTTPD_RESP_USE_STRLEN);
     
    strcpy(resp_str, "");
    rtds_metrics(resp_str);
    if (strlen(resp_str))
        httpd_resp_send_chunk(req, resp_str, HTTPD_RESP_USE_STRLEN); 
    
    strcpy(resp_str, "");
    tds_metrics(resp_str);
    if (strlen(resp_str))
        httpd_resp_send_chunk(req, resp_str, HTTPD_RESP_USE_STRLEN);

    /// virtualni vystupy odesilam na dvakrat
    strcpy(resp_str, "");
    virtual_outputs_metrics(resp_str, 0, MAX_OUTPUT/2);
    if (strlen(resp_str))
        httpd_resp_send_chunk(req, resp_str, HTTPD_RESP_USE_STRLEN);

    strcpy(resp_str, "");
    virtual_outputs_metrics(resp_str, MAX_OUTPUT/2, MAX_OUTPUT);
    if (strlen(resp_str))
        httpd_resp_send_chunk(req, resp_str, HTTPD_RESP_USE_STRLEN);

    strcpy(resp_str, "");
    prom_metric_know_device(resp_str);
    if (strlen(resp_str))
        httpd_resp_send_chunk(req, resp_str, HTTPD_RESP_USE_STRLEN);

    strcpy(resp_str, "");
    httpd_resp_send_chunk(req, resp_str, 0);

    return ESP_OK;
}

/* An HTTP GET handler */
static esp_err_t configurations_get_handler(httpd_req_t *req)
{
    char resp_str[2048];
    strcpy(resp_str, "");
    json_device_config(resp_str);
    httpd_resp_set_type(req, (const char*) "application/json");
    httpd_resp_send(req, resp_str, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}


/* An HTTP GET handler */
static esp_err_t configurations_tds_get_handler(httpd_req_t *req)
{
    char resp_str[2048];
    strcpy(resp_str, "");
    tds_dump_configuration(resp_str);
    httpd_resp_set_type(req, (const char*) "application/json");
    httpd_resp_send(req, resp_str, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}


static esp_err_t configurations_virtual_outputs_handler(httpd_req_t *req)
{
    char resp_str[8196];
    strcpy(resp_str, "");
    virtual_outputs_dump_configuration(resp_str);
    httpd_resp_set_type(req, (const char*) "application/json");
    httpd_resp_send(req, resp_str, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}


/* An HTTP POST handler */
static esp_err_t configurations_post_virtual_outputs_handler(httpd_req_t *req)
{
    cJSON *root, *tmp_json;
    char resp_str[8196];
    char str_idx[4];
    size_t recv_size;
    int ret;
    recv_size = MIN(req->content_len, sizeof(resp_str));
    ret = httpd_req_recv(req, resp_str, recv_size);
    if (ret <= 0)
        {
        if (ret == HTTPD_SOCK_ERR_TIMEOUT)
            {
            httpd_resp_send_408(req);
            }
        return ESP_FAIL;
        }

    root = cJSON_Parse(resp_str);

    for (uint8_t idx = 0; idx < MAX_OUTPUT; idx++)
        {
        itoa(idx, str_idx, 10);
        tmp_json = cJSON_GetObjectItem(root, str_idx);
	if (tmp_json)
	    {
	    virtual_outputs_set_all_from_json(idx, tmp_json);    
	    }
	}

    cJSON_Delete(root);
    httpd_resp_sendstr(req, "OK\n");
    return ESP_OK;
}


static esp_err_t configurations_tds_delete_handler(httpd_req_t *req)
{
    cJSON *root, *tmp_tds;
    char resp_str[512];
    size_t recv_size;
    int ret;
    recv_size = MIN(req->content_len, sizeof(resp_str));
    ret = httpd_req_recv(req, resp_str, recv_size);
    if (ret <= 0)
        {
        if (ret == HTTPD_SOCK_ERR_TIMEOUT)
            {
            httpd_resp_send_408(req);
            }
        return ESP_FAIL;
        }

    root = cJSON_Parse(resp_str);
    tmp_tds = cJSON_GetObjectItem(root, "idx");
    if (tmp_tds)
	{
        tds_set_clear(tmp_tds->valueint);
	ESP_LOGI(TAG, "TDS clear IDX: %d", tmp_tds->valueint);
	}

    httpd_resp_sendstr(req, "OK\n");
    return ESP_OK;
}


static esp_err_t configurations_tds_post_handler(httpd_req_t *req)
{
    cJSON *root, *tmp_json, *tmp_tds, *tmp_1wire;
    char resp_str[8196];
    char str_idx[4];
    size_t recv_size;
    int ret;
    recv_size = MIN(req->content_len, sizeof(resp_str));
    ret = httpd_req_recv(req, resp_str, recv_size);
    if (ret <= 0)
        {
        if (ret == HTTPD_SOCK_ERR_TIMEOUT)
            {
            httpd_resp_send_408(req);
            }
        return ESP_FAIL;
        }

    root = cJSON_Parse(resp_str);
    tmp_tds = cJSON_GetObjectItem(root, "tds");
    if (tmp_tds)
        for (uint8_t idx = 0; idx < MAX_OUTPUT; idx++)
            {
            itoa(idx, str_idx, 10);
            tmp_json = cJSON_GetObjectItem(tmp_tds, str_idx);
            if (tmp_json)
                {
                tds_set_all_from_json(idx, tmp_json);
                }
            }

    tmp_1wire = cJSON_GetObjectItem(root, "1wire");
    if (tmp_1wire)
	{
	onewire_set_all_from_json(tmp_1wire);
	}

    cJSON_Delete(root);
    httpd_resp_sendstr(req, "OK\n");
    return ESP_OK;
}


static esp_err_t configurations_rtds_delete_handler(httpd_req_t *req)
{
    char resp_str[512];
    size_t recv_size;
    int ret;
    cJSON *root, *tmp_rtds;
    recv_size = MIN(req->content_len, sizeof(resp_str));
    ret = httpd_req_recv(req, resp_str, recv_size);
    if (ret <= 0)
        {
        if (ret == HTTPD_SOCK_ERR_TIMEOUT)
            {
            httpd_resp_send_408(req);
            }
        return ESP_FAIL;
        }

    root = cJSON_Parse(resp_str);
    tmp_rtds = cJSON_GetObjectItem(root, "idx");
    if (tmp_rtds)
        {
        remote_tds_clear(tmp_rtds->valueint);
        ESP_LOGI(TAG, "RTDS clear IDX: %d", tmp_rtds->valueint);
        }

    httpd_resp_sendstr(req, "OK\n");
    return ESP_OK;
}


static esp_err_t configurations_rtds_post_handler(httpd_req_t *req)
{

    char resp_str[RTDS_DEVICE_STRING_LEN];
    size_t recv_size;
    int ret;
    uint8_t id = 255;
    recv_size = MIN(req->content_len, sizeof(resp_str));
    ret = httpd_req_recv(req, resp_str, recv_size);
    if (ret <= 0)
        {
        if (ret == HTTPD_SOCK_ERR_TIMEOUT)
            {
            httpd_resp_send_408(req);
            }
        return ESP_FAIL;
        }

    if (remote_tds_name_exist(resp_str) == 255)
        {
        id = remote_tds_find_free();
        remote_tds_set_complete(id, 1, resp_str);
        remote_tds_subscibe_topic(mqtt_client, id);
        }

    if (id == 255 )
        strcpy(resp_str, "FAIL\n");
    else
	sprintf(resp_str, "OK new %d\n", id);
    httpd_resp_sendstr(req, resp_str);
    return ESP_OK;
}

/* An HTTP GET handler */
static esp_err_t configurations_rtds_get_handler(httpd_req_t *req)
{
    char resp_str[2048];
    strcpy(resp_str, "");
    rtds_dump_configuration(resp_str);
    httpd_resp_set_type(req, (const char*) "application/json");
    httpd_resp_send(req, resp_str, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}



static const httpd_uri_t prom_metrics = {
    .uri       = "/metrics",
    .method    = HTTP_GET,
    .handler   = prom_metrics_get_handler,
    .user_ctx  = 0
};

static const httpd_uri_t get_network_configurations = {
    .uri       = "/network",
    .method    = HTTP_GET,
    .handler   = configurations_get_handler,
    .user_ctx  = 0
};

static const httpd_uri_t post_network_configurations = {
        .uri = "/network",
        .method = HTTP_POST,
        .handler = configurations_post_handler,
        .user_ctx = 0
    };

static const httpd_uri_t get_tds_configurations = {
    .uri       = "/tds",
    .method    = HTTP_GET,
    .handler   = configurations_tds_get_handler,
    .user_ctx  = 0
};

static const httpd_uri_t post_tds_configurations = {
    .uri       = "/tds",
    .method    = HTTP_POST,
    .handler   = configurations_tds_post_handler,
    .user_ctx  = 0
};

static const httpd_uri_t delete_tds_configurations = {
    .uri       = "/tds",
    .method    = HTTP_DELETE,
    .handler   = configurations_tds_delete_handler,
    .user_ctx  = 0
};

static const httpd_uri_t get_rtds_configurations = {
    .uri       = "/rtds",
    .method    = HTTP_GET,
    .handler   = configurations_rtds_get_handler,
    .user_ctx  = 0
};

static const httpd_uri_t post_rtds_configurations = {
    .uri       = "/rtds",
    .method    = HTTP_POST,
    .handler   = configurations_rtds_post_handler,
    .user_ctx  = 0
};

static const httpd_uri_t delete_rtds_configurations = {
    .uri       = "/rtds",
    .method    = HTTP_DELETE,
    .handler   = configurations_rtds_delete_handler,
    .user_ctx  = 0
};


static const httpd_uri_t get_virtual_outputs_configurations = {
    .uri       = "/virtual-outputs",
    .method    = HTTP_GET,
    .handler   = configurations_virtual_outputs_handler,
    .user_ctx  = 0
};


static const httpd_uri_t post_virtual_outputs_configurations = {
    .uri       = "/virtual-outputs",
    .method    = HTTP_POST,
    .handler   = configurations_post_virtual_outputs_handler,
    .user_ctx  = 0
};

 
static httpd_handle_t start_webserver(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.stack_size = 20000;
    config.max_uri_handlers = 16;
    config.lru_purge_enable = true;
    config.server_port = device.http_port;
    // Start the httpd server
    ESP_LOGI(TAG, "Starting server on port: %d", device.http_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        ESP_LOGI(TAG, "Registering URI handlers");
        httpd_register_uri_handler(server, &prom_metrics);
	httpd_register_uri_handler(server, &get_network_configurations);
	httpd_register_uri_handler(server, &get_tds_configurations);
	httpd_register_uri_handler(server, &get_rtds_configurations);
	httpd_register_uri_handler(server, &get_virtual_outputs_configurations);
	
	httpd_register_uri_handler(server, &post_network_configurations);
	httpd_register_uri_handler(server, &post_virtual_outputs_configurations);
	httpd_register_uri_handler(server, &post_tds_configurations);
	httpd_register_uri_handler(server, &post_rtds_configurations);

	httpd_register_uri_handler(server, &delete_tds_configurations);
	httpd_register_uri_handler(server, &delete_rtds_configurations);
        return server;
    }

    ESP_LOGI(TAG, "Error starting server!");
    return NULL;
}







/** Event handler for IP_EVENT_ETH_GOT_IP */
static void got_ip_event_handler(void *arg, esp_event_base_t event_base,
                                 int32_t event_id, void *event_data)
{
    ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;
    const esp_netif_ip_info_t *ip_info = &event->ip_info;

    ESP_LOGI(TAG, "Ethernet Got IP Address");
    ESP_LOGI(TAG, "~~~~~~~~~~~");
    ESP_LOGI(TAG, "ETHIP:" IPSTR, IP2STR(&ip_info->ip));
    ESP_LOGI(TAG, "ETHMASK:" IPSTR, IP2STR(&ip_info->netmask));
    ESP_LOGI(TAG, "ETHGW:" IPSTR, IP2STR(&ip_info->gw));
    ESP_LOGI(TAG, "~~~~~~~~~~~");
}


void shiftout(uint8_t data)
{
   gpio_set_level(pin_output_strobe, 0);
   for (uint8_t i=0; i<8; i++)
     {
     if ((data & (1<<i)) !=0)
        gpio_set_level(pin_output_data, 1);
     else
	gpio_set_level(pin_output_data, 0);
      
     gpio_set_level(pin_output_clk, 0);
     gpio_set_level(pin_output_clk, 1);
     }
   gpio_set_level(pin_output_strobe, 1);
}



void setup(void)
{
  struct_DDS18s20 tds;
  esp_eth_handle_t eth_handle = NULL;
 
  esp_mqtt_client_config_t mqtt_cfg = {};
  char hostname[10];

  char ip_uri[20];
  char complete_ip_uri[32];
  char str1[32];
  char str2[16];

  esp_event_handler_instance_t instance_any_id;
  esp_event_handler_instance_t instance_got_ip;

  uint8_t eeprom_read;
  bool eeprom_ret;

  //zero-initialize the config structure.
  gpio_config_t io_conf = {};
  //disable interrupt
  io_conf.intr_type = GPIO_INTR_DISABLE;
  //set as output mode
  io_conf.mode = GPIO_MODE_OUTPUT;
  //bit mask of the pins that you want to set,e.g.GPIO18/19
  io_conf.pin_bit_mask = (1ULL<<pin_output_data | 1ULL<<pin_output_clk | 1ULL<<pin_output_strobe | 1ULL<<pin_output_oe); // | 1ULL<<pin_output_reset);
  //disable pull-down mode
  io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
  //disable pull-up mode
  io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
  //configure GPIO with the given settings
  gpio_config(&io_conf);

  //interrupt of rising edge
  io_conf.intr_type = GPIO_INTR_DISABLE;
  //bit mask of the pins, use GPIO4/5 here
  io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
  //set as input mode
  io_conf.mode = GPIO_MODE_INPUT;
  //enable pull-up mode
  io_conf.pull_up_en = (gpio_pullup_t) 1;
  gpio_config(&io_conf);


  gpio_set_level(pin_output_oe, 1);
  gpio_set_level(pin_output_oe, 0);

  twi_init(I2C_NUM_0);

   printf("EEPROM output start in %d\n", output0);
   printf("EEPROM output end in %d\n", output_store_last);

   printf("EEPROM rtds start %d\n", remote_tds_name0);
   printf("EEPROM rtds end %d\n", remote_tds_last);

   printf("EEPROM tds start %d\n", eeprom_wire_know_rom);
   printf("EEPROM tds end %d\n", eeprom_tds_next_free_bank);

  esp_err_t ret;

  for (uint8_t init = 0;  init < 14; init++)
  {
    if (init == 0)
      {
	for (uint8_t idx = 0; idx < MAX_MENUS ;idx++)
          {
          menu_params_1[idx] = 0;
          menu_params_2[idx] = 0;
          menu_params_3[idx] = 0;
          menu_params_4[idx] = 0;
          }


        GLCD_Setup();
        GLCD_SetFont(Font5x8, 5, 8, GLCD_Overwrite);
        GLCD_Clear();
        GLCD_GotoXY(0, 0);
        GLCD_PrintString("... init ...");
	GLCD_GotoXY(0, 16);
        GLCD_PrintString("0. i2c bus");
        GLCD_Render();
      }
    ///  
    if (init == 1)
      {
      //EEPROM.write(set_default_values, 255);
      eeprom_read = EEPROM.read(set_default_values, &eeprom_ret);
      if ( eeprom_ret == true && eeprom_read == 255)
         {
         ESP_LOGI(TAG, "Default values");
         EEPROM.write(set_default_values, 0);
         device.mac[0] = 2; device.mac[1] = 1; device.mac[2] = 2; device.mac[3] = 24; device.mac[4] = 25; device.mac[5] = 26;
         device.myIP[0] = 192; device.myIP[1] = 168; device.myIP[2] = 1; device.myIP[3] = 24;
         device.myMASK[0] = 255; device.myMASK[1] = 255; device.myMASK[2] = 255; device.myMASK[3] = 0;

         device.myGW[0] = 192; device.myGW[1] = 168; device.myGW[2] = 1; device.myGW[3] = 1;
         device.myDNS[0] = 192; device.myDNS[1] = 168; device.myDNS[2] = 1; device.myDNS[3] = 1;
         device.mqtt_server[0] = 192; device.mqtt_server[1] = 168; device.mqtt_server[2] = 2; device.mqtt_server[3] = 1;
         device.ntp_server[0] = 192; device.ntp_server[1] = 168; device.ntp_server[2] = 2; device.ntp_server[3] = 1;
         device.mqtt_port = 1883;
         device.http_port = 80;
         //device.via = ENABLE_CONNECT_ETH | ENABLE_CONNECT_WIFI;
         device.via = ENABLE_CONNECT_WIFI;

	 strcpy(device.nazev, "TERMBIG4");
         strcpy(device.mqtt_user, "saric");
         strcpy(device.mqtt_key, "no");
	 strcpy(device.bootloader_uri, "http://192.168.2.1/termbig/v4.bin");

	 strcpy(device.wifi_essid, "saric2g");
         strcpy(device.wifi_key, "saric111");
         /// po initu je vzdy dhcp aktivni
         device.dhcp = DHCP_ENABLE;


	 save_setup_network();
         load_setup_network();
         strcpy(hostname, device.nazev);

	 for (uint8_t idx = 0; idx < HW_ONEWIRE_MAXDEVICES; idx++)
         {
          get_tds18s20(idx, &tds);
          sprintf(tds.name, "FREE%d", idx);
          tds.used = 0;
          tds.offset = 0;
          tds.assigned_ds2482 = 0;
          tds.period = 10;
          for (uint8_t m = 0; m < 8; m++) tds.rom[m] = 0xff;
          set_tds18s20(idx, &tds);
	  set_tds18s20_eeprom(idx, &tds);
         }

	 for (uint8_t idx = 0; idx < MAX_RTDS; idx++)
	 	remote_tds_clear(idx);

	 for (uint8_t idx = 0; idx < MAX_OUTPUT; idx++)
         {
          strcpy(str1, "OUT");
          itoa(idx, str2, 10);
          strcat(str1, str2);
          output_set_name(idx, str1);
          output_set_variables(idx, 0, OUTPUT_REAL_MODE_NONE, 0, 0, 0, PERIOD_50_MS, 0, 0, 0);
          output_set_set_period_time(idx, PERIOD_50_MS);
	  output_sync_to_eeprom_idx(idx);
         }

	 strcpy(complete_ip_uri, "");
         createString(ip_uri, '.', device.mac, 6, 16, 1);
         strcat(complete_ip_uri, ip_uri);
         ESP_LOGI(TAG, "mac address: %s", complete_ip_uri);

	 strcpy(complete_ip_uri, "");
	 createString(ip_uri, '.', device.myIP, 4, 10, 1);
         strcat(complete_ip_uri, ip_uri);
         ESP_LOGI(TAG, "ip address: %s", complete_ip_uri);

	 strcpy(complete_ip_uri, "");
	 createString(ip_uri, '.', device.myMASK, 4, 10, 1);
         strcat(complete_ip_uri, ip_uri);
         ESP_LOGI(TAG, "netmask : %s", complete_ip_uri);

	 strcpy(complete_ip_uri, "");
	 createString(ip_uri, '.', device.myGW, 4, 10, 1);
         strcat(complete_ip_uri, ip_uri);
         ESP_LOGI(TAG, "default gateway: %s", complete_ip_uri);

	 strcpy(complete_ip_uri, "");
	 createString(ip_uri, '.', device.myDNS, 4, 10, 1);
         strcat(complete_ip_uri, ip_uri);
         ESP_LOGI(TAG, "dns server: %s", complete_ip_uri);

	 strcpy(complete_ip_uri, "");
	 strcpy(complete_ip_uri, "mqtt://");
         createString(ip_uri, '.', device.mqtt_server, 4, 10, 1);
         strcat(complete_ip_uri, ip_uri);
         ESP_LOGI(TAG, "mqtt uri: %s", complete_ip_uri);

         ESP_LOGI(TAG, "hostname: %s", hostname);

	 ESP_LOGI(TAG, "mqtt_user: %s", device.mqtt_user);

	 ESP_LOGI(TAG, "mqtt_key: %s", device.mqtt_key);

	 ESP_LOGI(TAG, "mqtt_port: %d", device.mqtt_port);

	 ESP_LOGI(TAG, "wifi_essid: %s", device.wifi_essid);
	 ESP_LOGI(TAG, "wifi_key: %s", device.wifi_key);

	 GLCD_SetFont(Font5x8, 5, 8, GLCD_Overwrite);
         GLCD_Clear();
         GLCD_GotoXY(0, 0);
         GLCD_PrintString(".. init ...");
         GLCD_GotoXY(0, 16);
         GLCD_PrintString("1. update default");
         GLCD_Render();
         }
      else
	 {
	 ESP_LOGI(TAG, "Default values - skip");
	 GLCD_SetFont(Font5x8, 5, 8, GLCD_Overwrite);
         GLCD_Clear();
         GLCD_GotoXY(0, 0);
         GLCD_PrintString(".. init ...");
         GLCD_GotoXY(0, 16);
         GLCD_PrintString("1. default skip");
         GLCD_Render();
	 }	
      }

    if (init == 2)
     {
     output_sync_from_eeprom();
     outputs_variable_init();

     GLCD_SetFont(Font5x8, 5, 8, GLCD_Overwrite);
     GLCD_Clear();
     GLCD_GotoXY(0, 0);
     GLCD_PrintString(".. init ...");
     GLCD_GotoXY(0, 10);
     GLCD_PrintString("2. outputs");
     GLCD_Render();
     }


    if (init == 3)
      {
        uart_config_t uart_config = {
          .baud_rate = 38400,
          .data_bits = UART_DATA_8_BITS,
          .parity    = UART_PARITY_DISABLE,
          .stop_bits = UART_STOP_BITS_1,
          .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
          .rx_flow_ctrl_thresh = 122,
          .source_clk = UART_SCLK_DEFAULT,
          };

        QueueHandle_t uart_queue;
        ESP_ERROR_CHECK(uart_param_config(UART_NUM_2, &uart_config));
        ESP_ERROR_CHECK(uart_set_pin(UART_NUM_2, RS_TXD, RS_RXD, RS_RTS, RS_CTS));
        ESP_ERROR_CHECK(uart_driver_install(UART_NUM_2, 1024  , 1024, 10, &uart_queue, 0));
        ESP_ERROR_CHECK(uart_set_mode(UART_NUM_2, UART_MODE_RS485_HALF_DUPLEX));

        GLCD_SetFont(Font5x8, 5, 8, GLCD_Overwrite);
        GLCD_Clear();
        GLCD_GotoXY(0, 0);
        GLCD_PrintString(".. init ...");
        GLCD_GotoXY(0, 16);
        GLCD_PrintString("3. rs485 bus");
        GLCD_Render();
      }
    if (init == 4)
       {
	load_setup_network();
        dump_setup_network();	
        GLCD_SetFont(Font5x8, 5, 8, GLCD_Overwrite);
        GLCD_Clear();
        GLCD_GotoXY(0, 0);
        GLCD_PrintString(".. init ...");
        GLCD_GotoXY(0, 16);
        GLCD_PrintString("4. EEPROM config");
        GLCD_Render();
       }

    ///
    if (init == 5)
      {
      ds2482_address[0].i2c_addr = 0b0011000;
      ds2482_address[0].HWwirenum = 0;
      ds2482_address[1].i2c_addr = 0b0011011;
      ds2482_address[1].HWwirenum = 0;
      ///
      for (uint8_t idx = 0; idx < HW_ONEWIRE_MAXROMS; idx++ )
        {
         status_tds18s20[idx].wait = false;
         status_tds18s20[idx].online = false;
         status_tds18s20[idx].period_now = 0;
	 status_tds18s20[idx].temp = 200;
        }
      for (uint8_t idx = 0; idx < HW_ONEWIRE_MAXROMS; idx++ )
        {
        get_tds18s20_eeprom(idx, &tds);
	set_tds18s20(idx, &tds);
        }
      Global_HWwirenum = 0;
      one_hw_search_device(0);
      one_hw_search_device(1);
      tds_update_associate();

      for (uint8_t idx = 0; idx < HW_ONEWIRE_MAXROMS; idx++)
        {
        for (uint8_t cnt = 0; cnt < MAX_AVG_TEMP; cnt++) status_tds18s20[idx].average_temp[cnt] = 200;
        status_tds18s20[idx].average_temp_now = 200;
        status_tds18s20[idx].crc_error = 0;
        status_tds18s20[idx].avg_temp_cnt = 0;
        }

      GLCD_SetFont(Font5x8, 5, 8, GLCD_Overwrite);
      GLCD_Clear();
      GLCD_GotoXY(0, 0);
      GLCD_PrintString(".. init ...");
      GLCD_GotoXY(0, 10);
      GLCD_PrintString("5. 1wire bus");
      ESP_LOGI(TAG, "found %d 1wire device", Global_HWwirenum);
      sprintf(str1, "found: %d", Global_HWwirenum);
      GLCD_GotoXY(0, 20);
      GLCD_PrintString((char*) str1);
      GLCD_Render();
      }
    ///
    if (init == 6)
      {
      ESP_ERROR_CHECK(esp_netif_init());
      ESP_ERROR_CHECK(esp_event_loop_create_default());
      GLCD_SetFont(Font5x8, 5, 8, GLCD_Overwrite);
      GLCD_Clear();
      GLCD_GotoXY(0, 0);
      GLCD_PrintString(".. init ...");
      GLCD_GotoXY(0, 16);
      GLCD_PrintString("4. Network netif ");
      GLCD_Render();
      }

    //
        if (init == 7)
      {
      //Initialize NVS
      ret = nvs_flash_init();
      if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
        {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
        }
      ESP_ERROR_CHECK(ret);

      ESP_LOGI(TAG, "ESP_WIFI_MODE_STA");

      s_wifi_event_group = xEventGroupCreate();

      wifi_netif = esp_netif_create_default_wifi_sta();

      wifi_init_config_t wifi_netif_conf = WIFI_INIT_CONFIG_DEFAULT();
      ESP_ERROR_CHECK(esp_wifi_init(&wifi_netif_conf));

      wifi_config_t wifi_config = {
          .sta = {
              .ssid = "",
              .password = "",
          },
      };

      for (uint8_t idx = 0; idx < strlen(device.wifi_essid); idx++)
          wifi_config.sta.ssid[idx] = device.wifi_essid[idx];

      for (uint8_t idx = 0; idx < strlen(device.wifi_key); idx++)
          wifi_config.sta.password[idx] = device.wifi_key[idx];

      wifi_config.sta.threshold.authmode = ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD;

      ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
      ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );

      ESP_LOGI(TAG, "wifi_init_sta finished.");

      GLCD_SetFont(Font5x8, 5, 8, GLCD_Overwrite);
      GLCD_Clear();
      GLCD_GotoXY(0, 0);
      GLCD_PrintString(".. init ...");
      GLCD_GotoXY(0, 10);
      GLCD_PrintString("5. wifi init done");
      GLCD_Render();
      }
    ///
    
    if (init == 8)
      {
      ESP_ERROR_CHECK(gpio_install_isr_service(0));
      esp_netif_config_t eth_netif_cfg = ESP_NETIF_DEFAULT_ETH();
      eth_netif = esp_netif_new(&eth_netif_cfg);
         
      spi_bus_config_t buscfg = {
          .mosi_io_num = ENC28J60_MOSI_GPIO,
          .miso_io_num = ENC28J60_MISO_GPIO,
          .sclk_io_num = ENC28J60_SCLK_GPIO,
          .quadwp_io_num = -1,
          .quadhd_io_num = -1
      };
     
      ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO));
      /* ENC28J60 ethernet driver is based on spi driver */
      spi_device_interface_config_t devcfg  = {
          .command_bits = 3,
          .address_bits = 5,
          .mode = 0,
          .cs_ena_pretrans = 0,
          .cs_ena_posttrans = enc28j60_cal_spi_cs_hold_time(ENC28J60_SPI_CLOCK_MHZ),
          .clock_speed_hz = ENC28J60_SPI_CLOCK_MHZ * 1000 * 1000,
          .spics_io_num = ENC28J60_CS_GPIO,
          .queue_size = 20,
      };
    
      spi_device_handle_t spi_handle = NULL;
      ESP_ERROR_CHECK(spi_bus_add_device(SPI2_HOST, &devcfg, &spi_handle));

      //eth_enc28j60_config_t enc28j60_config = ETH_ENC28J60_DEFAULT_CONFIG(spi_handle, &devcfg);
      eth_enc28j60_config_t enc28j60_config = ETH_ENC28J60_DEFAULT_CONFIG(SPI2_HOST, &devcfg);
      enc28j60_config.int_gpio_num = ENC28J60_INT_GPIO;

      eth_mac_config_t mac_config = ETH_MAC_DEFAULT_CONFIG();
      esp_eth_mac_t *mac = esp_eth_mac_new_enc28j60(&enc28j60_config, &mac_config);

      eth_phy_config_t phy_config = ETH_PHY_DEFAULT_CONFIG();
      phy_config.autonego_timeout_ms = 0; // ENC28J60 doesn't support auto-negotiation
      phy_config.reset_gpio_num = 2; // ENC28J60 doesn't have a pin to reset internal PHY
      esp_eth_phy_t *phy = esp_eth_phy_new_enc28j60(&phy_config);

      esp_eth_config_t eth_config = ETH_DEFAULT_CONFIG(mac, phy);
      ret = esp_eth_driver_install(&eth_config, &eth_handle);
      if (ret != ESP_OK)
          ESP_LOGE(TAG, "fail eth driver install");

      mac->set_addr(mac, device.mac);

      // ENC28J60 Errata #1 check
      if (emac_enc28j60_get_chip_info(mac) < ENC28J60_REV_B5 && ENC28J60_SPI_CLOCK_MHZ < 8) {
          ESP_LOGE(TAG, "SPI frequency must be at least 8 MHz for chip revision less than 5");
          ESP_ERROR_CHECK(ESP_FAIL);
      }

      GLCD_SetFont(Font5x8, 5, 8, GLCD_Overwrite);
      GLCD_Clear();
      GLCD_GotoXY(0, 0);
      GLCD_PrintString(".. init ...");
      GLCD_GotoXY(0, 10);
      GLCD_PrintString("6. ethernet bus");
      GLCD_Render();
      }

    if (init == 9)
      {
      start_webserver();
      GLCD_SetFont(Font5x8, 5, 8, GLCD_Overwrite);
      GLCD_Clear();
      GLCD_GotoXY(0, 0);
      GLCD_PrintString(".. init ...");
      GLCD_GotoXY(0, 16);
      GLCD_PrintString("7. http server");
      GLCD_Render();
     }

    if (init == 10)
        {
        ESP_ERROR_CHECK(esp_netif_set_hostname(eth_netif, device.nazev));
        ESP_ERROR_CHECK(esp_netif_set_hostname(wifi_netif, device.nazev));
        if (device.dhcp == DHCP_ENABLE)
            ESP_LOGI(TAG, "DHCP client enable");
        if (device.dhcp == DHCP_DISABLE)
            {
            ESP_LOGI(TAG, "DHCP client disable");
            if ((device.via & (ENABLE_CONNECT_ETH)) != 0)
                {
                ESP_ERROR_CHECK(esp_netif_dhcpc_stop(eth_netif));
                ESP_LOGI(TAG, "DHCP client for Ethernet not enabled");
                }
            if ((device.via & (ENABLE_CONNECT_WIFI)) != 0)
               {
               ESP_ERROR_CHECK(esp_netif_dhcpc_stop(wifi_netif));
               ESP_LOGI(TAG, "DHCP client for Wifi not enabled");
               }
            
            esp_netif_ip_info_t ip_info;
            IP4_ADDR(&ip_info.ip, device.myIP[0], device.myIP[1], device.myIP[2], device.myIP[3]);
            IP4_ADDR(&ip_info.gw, device.myGW[0], device.myGW[1], device.myGW[2], device.myGW[3]);
            IP4_ADDR(&ip_info.netmask, device.myMASK[0], device.myMASK[1], device.myMASK[2], device.myMASK[3]);
            esp_netif_set_ip_info(eth_netif, &ip_info);
            esp_netif_set_ip_info(wifi_netif, &ip_info);

            ESP_LOGI(TAG, "Ethernet SET IP Address");
            ESP_LOGI(TAG, "~~~~~~~~~~~");
            ESP_LOGI(TAG, "set ETHIP:" IPSTR, IP2STR(&ip_info.ip));
            ESP_LOGI(TAG, "set ETHMASK:" IPSTR, IP2STR(&ip_info.netmask));
            ESP_LOGI(TAG, "set ETHGW:" IPSTR, IP2STR(&ip_info.gw));
            ESP_LOGI(TAG, "~~~~~~~~~~~");
            }

        /* start Ethernet driver state machine */
        if ((device.via & (ENABLE_CONNECT_ETH)) != 0)
            {
            /* attach Ethernet driver to TCP/IP stack */
            ESP_ERROR_CHECK(esp_netif_attach(eth_netif, esp_eth_new_netif_glue(eth_handle)));

            // Register user defined event handers
            ESP_ERROR_CHECK(esp_event_handler_register(ETH_EVENT, ESP_EVENT_ANY_ID, &eth_event_handler, NULL));
            ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_GOT_IP, &got_ip_event_handler, NULL));

            eth_duplex_t duplex = ETH_DUPLEX_FULL;
            ESP_ERROR_CHECK(esp_eth_ioctl(eth_handle, ETH_CMD_S_DUPLEX_MODE, &duplex));
            ESP_ERROR_CHECK(esp_eth_start(eth_handle));
            ESP_LOGI(TAG, "Started Ethernet interface");
            }
        else
            {
            ESP_LOGI(TAG, "Ethernet interface disabled");
            }

        if ((device.via & (ENABLE_CONNECT_WIFI)) != 0)
            {
            ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, &instance_any_id));
            ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, &instance_got_ip));
            ESP_ERROR_CHECK(esp_wifi_start());
            ESP_LOGI(TAG, "Starting Wifi interface");
            ESP_LOGI(TAG, "Connecting to AP");
            /// cekam maximalne 1minutu
            const TickType_t xTicksToWait = 60000 / portTICK_PERIOD_MS;
            EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT, pdFALSE, pdFALSE, xTicksToWait);

            if (bits & WIFI_CONNECTED_BIT)
               {
               ESP_LOGI(TAG, "connected to ap SSID: %s", device.wifi_essid);
               }
            else
               {
               if (bits & WIFI_FAIL_BIT)
                   {
                   ESP_LOGE(TAG, "Failed to connect to SSID: %s, password: %s", device.wifi_essid, device.wifi_key);
                   }
               else
                   {
                   ESP_LOGW(TAG, "UNEXPECTED EVENT");
                   }
               }
            }
        else
           {
           ESP_LOGI(TAG, "Wifi interface disabled");
           }


        GLCD_SetFont(Font5x8, 5, 8, GLCD_Overwrite);
        GLCD_Clear();
        GLCD_GotoXY(0, 0);
        GLCD_PrintString(".. init ...");
        GLCD_GotoXY(0, 10);
        GLCD_PrintString("8. ip stack");
        
        GLCD_GotoXY(0, 22);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        if (eth_link == ETHERNET_EVENT_CONNECTED)
          {
          GLCD_PrintString("link up");
          ESP_LOGI(TAG, "ethernet link up");
          }
        else
          {
          GLCD_PrintString("link down");
          ESP_LOGI(TAG, "ethernet link down");
          }
        GLCD_Render();
        }


  if (init == 11)
     {
      send_mqtt_set_header(termbig_header_out);
      strcpy(complete_ip_uri, "mqtt://");
      createString(ip_uri, '.', device.mqtt_server, 4, 10, 1);
      strcat(complete_ip_uri, ip_uri);
      ESP_LOGI(TAG, "mqtt_uri: %s", complete_ip_uri);

      mqtt_cfg.network.timeout_ms=1000;
      mqtt_cfg.session.keepalive=2000;
      mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
      esp_mqtt_client_set_uri(mqtt_client, complete_ip_uri);

      esp_mqtt_client_register_event(mqtt_client, (esp_mqtt_event_id_t) ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
      esp_mqtt_client_start(mqtt_client);
      ESP_LOGI(TAG, "mqtt client start");
      GLCD_SetFont(Font5x8, 5, 8, GLCD_Overwrite);
      GLCD_Clear();
      GLCD_GotoXY(0, 0);
      GLCD_PrintString(".. init ...");
      GLCD_GotoXY(0, 10);
      GLCD_PrintString("9. mqtt client");
      GLCD_GotoXY(0, 22);

      vTaskDelay(2000 / portTICK_PERIOD_MS);
      if (mqtt_link == MQTT_EVENT_CONNECTED)
         GLCD_PrintString("mqtt connected");
      if (mqtt_link == MQTT_EVENT_DISCONNECTED)
	{
        GLCD_PrintString("mqtt disconnected");
	ESP_LOGE(TAG, "mqtt disconnected");
	}
      if (mqtt_link == MQTT_EVENT_ERROR)
	{
        GLCD_PrintString("mqtt error");
	ESP_LOGE(TAG, "mqtt error");
	}
      GLCD_Render();
     }

  if (init == 12)
     {
     button_up.setDebounceTime(10);
     button_down.setDebounceTime(10);
     button_enter.setDebounceTime(10);
     
     GLCD_SetFont(Font5x8, 5, 8, GLCD_Overwrite);
     GLCD_Clear();
     GLCD_GotoXY(0, 0);
     GLCD_PrintString(".. init ...");
     GLCD_GotoXY(0, 11);
     GLCD_PrintString("12. GPIO");
     GLCD_Render();
     }
  ///
  if (init == 13)
     {
     outputs_start_init();

     GLCD_SetFont(Font5x8, 5, 8, GLCD_Overwrite);
     GLCD_Clear();
     GLCD_GotoXY(0, 0);
     GLCD_PrintString(".. init ...");
     GLCD_GotoXY(0, 11);
     GLCD_PrintString("13. default out");
     GLCD_Render();
     }
  }
}





///void callback_1_milisec(void* arg)
void callback_1_milisec(void)
{
  uint16_t period_now, period;

  _millis++;

  for (uint8_t idx = 0; idx < MAX_OUTPUT; idx++)
    output_inc_period_timer(idx);
  /// pro rizeni PWM vystupu
  for (uint8_t idx = 0; idx < MAX_OUTPUT; idx++)
  {
    output_get_period_for_pwm(idx, &period, &period_now);
    if (period_now >= period)
    {
      output_reset_period_timer(idx);
      output_inc_state_timer(idx);
    }
  }
  update_phy_output();
}


uint32_t millis(void)
{
  return _millis;
}


void callback_1_sec(void* arg)
{
  ESP_LOGD(TAG, "timer 1 sec");
  uptime++;
  display_redraw = true;
  //update_phy_output();
}




void core0Task( void * pvParameters )
{
  char str1[32];
  char str2[32];

  uint8_t button_up_state = BUTTON_RELEASED;
  uint8_t button_down_state = BUTTON_RELEASED;
  uint8_t button_enter_state = BUTTON_RELEASED;

  uint8_t _button_up_state = BUTTON_RELEASED;
  uint8_t _button_down_state = BUTTON_RELEASED;
  uint8_t _button_enter_state = BUTTON_RELEASED;

  uint32_t next_1s = uptime;
  uint32_t next_10s = uptime;
  uint32_t next_20s = uptime;
  uint32_t next_60s = uptime;
  uint32_t next_12h = uptime;

  uint8_t index_update_output = 0;
  uint8_t index_update_output_cnt = 0;

  while(1)
  {
    button_up.loop();
    button_down.loop();
    button_enter.loop();


    if (button_up.isPressed() && _button_up_state == BUTTON_RELEASED)
        {
        _button_up_state = BUTTON_PRESSED;
        button_up_state = BUTTON_PRESSED;
        display_redraw = true;
        }

    if (button_down.isPressed() && _button_down_state == BUTTON_RELEASED)
        {
        _button_down_state = BUTTON_PRESSED;
        button_down_state = BUTTON_PRESSED;
        display_redraw = true;
        }

    if (button_enter.isPressed() && _button_enter_state == BUTTON_RELEASED)
        {
        _button_enter_state = BUTTON_PRESSED;
        button_enter_state = BUTTON_PRESSED;
        display_redraw = true;
        }

    if (button_up.isReleased())
        {
        _button_up_state = BUTTON_RELEASED;
        button_up_state = BUTTON_RELEASED;
        display_redraw = true;
        }

    if (button_down.isReleased())
        {
        _button_down_state = BUTTON_RELEASED;
        button_down_state = BUTTON_RELEASED;
        display_redraw = true;
        }

    if (button_enter.isReleased())
        {
        _button_enter_state = BUTTON_RELEASED;
        button_enter_state = BUTTON_RELEASED;
        display_redraw = true;
        }

    display_redraw = display_menu(button_up_state, button_down_state, button_enter_state, display_redraw);
    button_up_state = BUTTON_RELEASED;
    button_down_state = BUTTON_RELEASED;
    button_enter_state = BUTTON_RELEASED;


   
    if ((uptime - next_1s) > 1)
      {
      next_1s += 1;
      /// merim maximalne dve cidla za jednu iteraci funkce
      mereni_hwwire(2);

      /// odesilam rychlo stav vystupu
      index_update_output_cnt = 0;
      while (index_update_output_cnt < MAX_OUTPUT)
	{
	if (index_update_output < MAX_OUTPUT)
	    {
	    if (send_mqtt_outputs_virtual_id(index_update_output) == 1)
	        {
	        index_update_output++;
	        break;
	        }
	    else
               {
	       index_update_output++;
	       }
	    }
	else
	   {
	   index_update_output = 0;
	   }
	index_update_output_cnt++;
	}
      }

    if ((uptime - next_10s) > 10)
      {
      next_10s += 10;


      send_mqtt_tds_temp(); 

      send_mqtt_status(mqtt_client);

      get_device_status();

      update_know_mqtt_device();

      send_device_status();
      send_mqtt_outputs();
      }   

    if ((uptime - next_20s) > 20)
      {
      next_20s += 20;

      use_tds = count_use_tds();
      use_rtds = count_use_rtds(&use_rtds_type_temp);

      remote_tds_update_last_update();

      strcpy_P(str2, termbig_subscribe);
      device_get_name(str1);
      send_mqtt_payload(mqtt_client, str2, str1);
      }
    
    if ((uptime - next_60s) > 120)
      {
      next_60s += 120;
      send_mqtt_tds_all();
      }
  }
}



void core1Task( void * pvParameters )
{
  while(1)
  {
  
  }
}



static bool IRAM_ATTR timer_on_alarm_1ms(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_data)
{
  callback_1_milisec();
  return true;
}




extern "C" void app_main(void)
{

  //esp_log_level_set("*", ESP_LOG_INFO);

  esp_err_t err = nvs_flash_init();
  if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
  }
  ESP_ERROR_CHECK(err);

  get_sha256_of_partitions();


  const esp_timer_create_args_t periodic_timer_args_1_sec = {.callback = &callback_1_sec, .name = "1_sec" };

  setup();

  xTaskCreatePinnedToCore(core0Task, "core0Task", 15000, NULL, 0, NULL, 0);
  xTaskCreatePinnedToCore(core1Task, "core1Task", 15000, NULL, 0, NULL, 1);


  esp_timer_handle_t periodic_timer_1_sec;
  esp_timer_create(&periodic_timer_args_1_sec, &periodic_timer_1_sec);
  esp_timer_start_periodic(periodic_timer_1_sec, 1000000);


  //timer_queue_element_t ele;
  QueueHandle_t timer_queue = xQueueCreate(10, sizeof(timer_queue_element_t));
  if (!timer_queue)
  {
        ESP_LOGE(TAG, "Creating timer_queue failed");
        return;
  }

  gptimer_handle_t gptimer_1ms = NULL;
  gptimer_config_t timer_config = {
        .clk_src = GPTIMER_CLK_SRC_APB,
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = 1000000, // 1MHz, 1 tick=1us
  };
  ESP_ERROR_CHECK(gptimer_new_timer(&timer_config, &gptimer_1ms));

  gptimer_event_callbacks_t timer_event_define_callback = {
        .on_alarm = timer_on_alarm_1ms,
    };
  ESP_ERROR_CHECK(gptimer_register_event_callbacks(gptimer_1ms, &timer_event_define_callback, timer_queue));

  gptimer_alarm_config_t timer_alarm_config_1ms = {
        .alarm_count = 1000, // period = 1ms
        .reload_count = 0
    };
  timer_alarm_config_1ms.flags.auto_reload_on_alarm = true;

  ESP_LOGI(TAG, "Enable timer");
  ESP_ERROR_CHECK(gptimer_enable(gptimer_1ms));
  ESP_ERROR_CHECK(gptimer_set_alarm_action(gptimer_1ms, &timer_alarm_config_1ms));
  ESP_ERROR_CHECK(gptimer_start(gptimer_1ms));

}







void get_device_status(void)
{
   wifi_ap_record_t wifidata;
   if (esp_wifi_sta_get_ap_info(&wifidata)==0)
     {
     wifi_rssi = 255 - wifidata.rssi;
     }

   free_heap = esp_get_free_heap_size();
}






esp_err_t twi_init(i2c_port_t t_i2c_num)
{
  #define I2C_MASTER_TX_BUF_DISABLE 0                           /*!< I2C master doesn't need buffer */
  #define I2C_MASTER_RX_BUF_DISABLE 0                           /*!< I2C master doesn't need buffer */
  esp_err_t ret;
  i2c_num = t_i2c_num;
  i2c_config_t conf;
  conf.mode = I2C_MODE_MASTER;
  conf.sda_io_num = I2C_MASTER_SDA_IO;
  conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
  conf.scl_io_num = I2C_MASTER_SCL_IO;
  conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
  conf.master.clk_speed = 400000L;
  conf.clk_flags = 0;
  i2c_param_config(i2c_num, &conf);
  ret = i2c_driver_install(i2c_num, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
  if (!ret)
	  ESP_LOGI(TAG, "I2C init OK");
  else
	  ESP_LOGE(TAG, "I2C init ERR");
  return ret;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////


/// funkce pro odeslani informaci o tds sensoru
//// /thermctl-out/XXXXX/tds/ID/temp
//// /thermctl-out/XXXXX/tds/ID/name
//// /thermctl-out/XXXXX/tds/ID/offset
//// /thermctl-out/XXXXX/tds/ID/online
//// /thermctl-out/XXXXX/tds/ID/rom
//// /thermctl-out/XXXXX/tds/ID/period
void send_mqtt_tds_temp(void)
{
    struct_DDS18s20 tds;
    char payload[32];
    uint8_t active[HW_ONEWIRE_MAXROMS];
    tds_all_used(active);

    for (uint8_t id = 0; id < HW_ONEWIRE_MAXROMS; id++)
        if (active[id] == 1)
            if (get_tds18s20(id, &tds) == true)
                {
                itoa(status_tds18s20[id].temp, payload, 10);
                send_mqtt_message_prefix_id_topic_payload(mqtt_client, "xtds", id, "temp", payload);
                }
}


void send_mqtt_tds_all(void)
{
    struct_DDS18s20 tds;
    char payload[32];
    uint8_t active[HW_ONEWIRE_MAXROMS];
    tds_all_used(active);

    for (uint8_t id = 0; id < HW_ONEWIRE_MAXROMS; id++)
        if (active[id] == 1)
            if (get_tds18s20(id, &tds) == true)
                {
                itoa(status_tds18s20[id].average_temp_now , payload, 10);
                send_mqtt_message_prefix_id_topic_payload(mqtt_client, "xtds", id, "temp_avg", payload);
                strcpy(payload, tds.name);
                send_mqtt_message_prefix_id_topic_payload(mqtt_client, "xtds", id, "name", payload);
                itoa(status_tds18s20[id].online, payload, 10);
                send_mqtt_message_prefix_id_topic_payload(mqtt_client, "xtds", id, "online", payload);
                itoa(status_tds18s20[id].crc_error, payload, 10);
                send_mqtt_message_prefix_id_topic_payload(mqtt_client, "xtds", id, "crc_error", payload);
                }
}



void send_device_status(void)
{
  char str_topic[32];
  char payload[16];

  strcpy(str_topic, "status/uptime");
  ltoa(uptime, payload, 10);
  send_mqtt_general_payload(mqtt_client, str_topic, payload);

}

/*
void send_mqtt_remote_tds_status(void)
{
  uint8_t active = 0;
  char payload[RTDS_DEVICE_STRING_LEN];
  for (uint8_t idx = 0; idx < MAX_RTDS; idx++)
  {
    remote_tds_get_complete(idx, &active, payload);
    /// odeslu pouze pokud je neco aktivni, jinak ne
    if (active == 1)
    {
      send_mqtt_message_prefix_id_topic_payload(mqtt_client, "rtds", idx, "name", payload);
      itoa(active, payload, 10);
      send_mqtt_message_prefix_id_topic_payload(mqtt_client, "rtds", idx, "active", payload);
      itoa(remote_tds_get_data(idx), payload, 10);
      send_mqtt_message_prefix_id_topic_payload(mqtt_client, "rtds", idx, "value", payload);
      itoa(remote_tds_get_type(idx), payload, 10);
      send_mqtt_message_prefix_id_topic_payload(mqtt_client, "rtds", idx, "type", payload);
      itoa(remote_tds_get_last_update(idx), payload, 10);
      send_mqtt_message_prefix_id_topic_payload(mqtt_client, "rtds", idx, "last_update", payload);
    }
  }
}
*/

void send_mqtt_output_virtual(const char *topic, const char *payload, uint8_t virtual_id)
{
  char t_topic[64];
  char str1[12];
  strcpy_P(t_topic, termbig_virtual_output);
  strcat(t_topic, "/");
  itoa(virtual_id, str1, 10);
  strcat(t_topic, str1);

  strcat(t_topic, "/");
  strcat(t_topic, topic);
  send_mqtt_payload(mqtt_client, t_topic, payload);
}


uint8_t send_mqtt_outputs_virtual_id(uint8_t id)
{
    uint8_t ret = 0;
    char str1[12];
    struct_output output;
    uint8_t tt = 0;
    output_get_all(id, &output);
    if (output.used != 0)
    {
      tt = output.id;
      send_mqtt_output_virtual("name", output.name, tt);
      itoa(output.state, str1, 10);
      send_mqtt_output_virtual("state", str1, tt);
      ret = 1;
    }
    return ret;
}



void send_mqtt_outputs(void)
{
  struct_output output;
  char payload[64];
  int tt;

  for (uint8_t id = 0; id < MAX_OUTPUT; id++)
  {
    output_get_all(id, &output);
    if (output.used != 0)
    {
      strcpy(payload, output.name);
      send_mqtt_message_prefix_id_topic_payload(mqtt_client, "output", id, "name", payload);

      //tt = output.outputs;
      //itoa(tt, payload, 10);
      //send_mqtt_message_prefix_id_topic_payload(mqtt_client, "output", id, "outputs", payload);

      //tt = output.mode_enable;
      //itoa(tt, payload, 10);
      //send_mqtt_message_prefix_id_topic_payload(mqtt_client, "output", id, "mode-enable", payload);

      tt = output.mode_now;
      itoa(tt, payload, 10);
      send_mqtt_message_prefix_id_topic_payload(mqtt_client, "output", id, "mode-now", payload);

      //tt = output.type;
      //itoa(tt, payload, 10);
      //send_mqtt_message_prefix_id_topic_payload(mqtt_client, "output", id, "type", payload);

      //tt = output.id;
      //itoa(tt, payload, 10);
      //send_mqtt_message_prefix_id_topic_payload(mqtt_client, "output", id, "virtual-id", payload);

      tt = output.state;
      itoa(tt, payload, 10);
      send_mqtt_message_prefix_id_topic_payload(mqtt_client, "output", id, "state", payload);

      //tt = output.used;
      //itoa(tt, payload, 10);
      //send_mqtt_message_prefix_id_topic_payload(mqtt_client, "output", id, "used", payload);

      //tt = output.period;
      //itoa(tt, payload, 10);
      //send_mqtt_message_prefix_id_topic_payload(mqtt_client, "output", id, "period", payload);

      //tt = output.state_time_max;
      //itoa(tt, payload, 10);
      //send_mqtt_message_prefix_id_topic_payload(mqtt_client, "output", id, "state-time-max", payload);

      //tt = output.after_start_state;
      //itoa(tt, payload, 10);
      //send_mqtt_message_prefix_id_topic_payload(mqtt_client, "output", id, "after-start-state", payload);
      
      //tt = output.after_start_mode;
      //itoa(tt, payload, 10);
      //send_mqtt_message_prefix_id_topic_payload(mqtt_client, "output", id, "after-start-mode", payload);
    }
  }
}

void send_mqtt_output_quick_state(uint8_t idx, char *mode, uint8_t args)
{
  char topic[64];
  char payload[16];
  char str1[8];

  strcpy_P(topic, termbig_virtual_output);
  strcat(topic, "/");
  itoa(idx, str1, 10);
  strcat(topic, str1);
  strcat(topic, "/");
  strcat(topic, mode);
  itoa(args, payload, 10);
  send_mqtt_payload(mqtt_client, topic, payload);
}

void send_mqtt_fast_message(char *topic, char *payload)
{
  send_mqtt_payload(mqtt_client, topic, payload);
}


/*************************************************************************************************************************/
///SEKCE MQTT ///

void mqtt_callback_prepare_topic_array(char *out_str, char *in_topic)
{
  uint8_t cnt = 0;
  cnt = strlen(in_topic) - strlen(out_str);
  memcpy(out_str, in_topic + strlen(out_str), cnt);
  out_str[cnt] = 0;
}

void mqtt_callback(char* topic, uint8_t * payload, unsigned int length_topic, unsigned int length_data)
{
  char str1[64];
  char tmp1[32];
  char tmp2[32];
  char my_payload[128];
  char my_topic[128];
  uint8_t cnt = 0;
  uint8_t id = 0;
  char *pch;
  uint8_t active;

  //NTPClient timeClient(udpClient);
  //DateTime ted;
  memset(my_payload, 0, 128);
  memset(my_topic, 0, 128);
  ////
  mqtt_receive_message++; /// inkrementuji promenou celkovy pocet prijatych zprav
  strncpy(my_payload, (char*) payload, length_data);
  strncpy(my_topic, (char*) topic, length_topic);
  //ESP_LOGI(TAG, "%d:%s ; %d:%s", length_topic,my_topic, length_data, my_payload);
  ////
  /// pridam mqqt kamarada typ termbig
  strcpy_P(str1, termbig_subscribe);
  if (strcmp(str1, my_topic) == 0)
  {
    if (strcmp(device.nazev, my_payload) != 0) /// sam sebe ignoruj
    {
      mqtt_process_message++; /// inkrementuji promenou celkovy pocet zpracovanych zprav
      know_mqtt_create_or_update(my_payload, TYPE_TERMBIG);
    }
  }
  ////
  //// pridam mqtt kamarada typ room controler
  strcpy_P(str1, thermctl_subscribe);
  if (strcmp(str1, my_topic) == 0)
  {
    if (strcmp(device.nazev, my_payload) != 0) /// sam sebe ignoruj
    {
      mqtt_process_message++;
      know_mqtt_create_or_update(my_payload, TYPE_THERMCTL);
    }
  }

  //// pridam mqtt kamarada typ brana
  strcpy_P(str1, brana_subscribe);
  if (strcmp(str1, my_topic) == 0)
  {
    if (strcmp(device.nazev, my_payload) != 0) /// sam sebe ignoruj
    {
      mqtt_process_message++;
      know_mqtt_create_or_update(my_payload, TYPE_BRANA);
    }
  }

  //// pridam mqtt kamarada typ energy meter
  strcpy_P(str1, monitor_subscribe);
  if (strcmp(str1, my_topic) == 0)
  {
    if (strcmp(device.nazev, my_payload) != 0) /// sam sebe ignoruj
    {
      mqtt_process_message++;
      know_mqtt_create_or_update(my_payload, TYPE_MONITOR);
    }
  }

  ////
  ////
  //// thermctl-in/XXXXX/rtds/set/IDX/name - nastavi a udela prihlaseni
  strcpy_P(str1, termbig_header_in);
  strcat(str1, device.nazev);
  strcat(str1, "/rtds-control/set/");
  if (strncmp(str1, my_topic, strlen(str1)) == 0)
  {
    mqtt_process_message++;
    mqtt_callback_prepare_topic_array(str1, my_topic);
    cnt = 0;
    pch = strtok (str1, "/");
    while (pch != NULL)
    {
      if (cnt == 0) id = atoi(pch);
      if ((cnt == 1) && (strcmp(pch, "name") == 0))
      {
        remote_tds_get_active(id, &active);
        if (active == 0)
        {
          remote_tds_set_complete(id, 1, my_payload);
          remote_tds_subscibe_topic(mqtt_client, id);
        }
        else
        {
        }
      }
      pch = strtok (NULL, "/");
      cnt++;
    }
  }
  //// /thermctl-in/XXXX/rtds/clear index vymaze a odhlasi
  strcpy_P(str1, termbig_header_in);
  strcat(str1, device.nazev);
  strcat(str1, "/rtds-control/clear");
  if (strncmp(str1, my_topic, strlen(str1)) == 0)
  {
    mqtt_process_message++;
    id = atoi(my_payload);
    remote_tds_unsubscibe_topic(mqtt_client, id);
    /// TODO dodelat navratovou chybu
    remote_tds_clear(id);
  }

  strcpy_P(str1, termbig_header_in);
  strcat(str1, device.nazev);
  strcat(str1, "/rtds-control/reset_all");
  if (strncmp(str1, my_topic, strlen(str1)) == 0)
  {
  mqtt_process_message++;
  for (uint8_t idx = 0; idx < MAX_RTDS; idx++)
     remote_tds_clear(idx);
  }
  ///
  /*
  //// ziska nastaveni remote_tds
  strcpy_P(str1, thermctl_header_in);
  strcat(str1, device.nazev);
  strcat(str1, "/rtds-control/get");
  if (strncmp(str1, my_topic, strlen(str1)) == 0)
  {
    send_mqtt_remote_tds_status();
  }
  */

  ////
  //// rtds/NAME - hodnota, kde NAME je nazev cidla
  strcpy_P(str1, new_text_slash_rtds_slash); /// /rtds/
  if (strncmp(str1, my_topic, strlen(str1)) == 0)
  {
    mqtt_process_message++;
    mqtt_callback_prepare_topic_array(str1, my_topic);
    cnt = 0;
    pch = strtok (str1, "/");
    while (pch != NULL)
    {
      if (cnt == 0)
        strcpy(tmp1, pch);
      if (cnt == 1)
        strcpy(tmp2, pch);
      pch = strtok (NULL, "/");
      cnt++;
    }
    for (uint8_t idx = 0; idx < MAX_RTDS; idx++)
    {
      uint8_t active = 0;
      str1[0] = 0;
      remote_tds_get_complete(idx, &active, str1);
      if (active == 1 && strcmp(str1, tmp1) == 0)
      {
        if (strcmp(tmp2, "value") == 0)
          remote_tds_set_data(idx, atoi(my_payload));
        if (strcmp(tmp2, "type") == 0)
          remote_tds_set_type(idx, atoi(my_payload));
      }
    }
  }
  ///
  ///
  strcpy_P(str1, termbig_header_in);
  strcat(str1, device.nazev);
  strcat(str1, "/output/set/");
  if (strncmp(str1, my_topic, strlen(str1)) == 0)
  {
    mqtt_process_message++;
    cnt = 0;
    for (uint8_t f = strlen(str1); f < strlen(my_topic); f++)
    {
      str1[cnt] = topic[f];
      str1[cnt + 1] = 0;
      cnt++;
    }
    cnt = 0;
    pch = strtok (str1, "/");
    while (pch != NULL)
    {
      if (cnt == 0) id = atoi(pch);
      if (id < MAX_OUTPUT)
      {
        if ((cnt == 1) && (strcmp(pch, "reset-period") == 0)) {output_reset_period_timer(id);}
      }
      else
      {
      }
      pch = strtok (NULL, "/");
      cnt++;
    }
  }


  strcpy_P(str1, termbig_header_in);
  strcat(str1, device.nazev);
  strcat(str1, "/output/reset_default");
  if (strncmp(str1, my_topic, strlen(str1)) == 0)
  {
    active = atoi(my_payload); 
    strcpy(tmp1, "OUT");
    itoa(active, tmp2, 10);
    strcat(tmp1, tmp2);
    output_set_name(active, tmp1);
    output_set_variables(active, 0, OUTPUT_REAL_MODE_NONE, 0, 0, 0, PERIOD_50_MS, 0, 0, 0);
    output_set_set_period_time(active, PERIOD_50_MS);
    output_sync_to_eeprom_idx(active);
  }

  strcpy_P(str1, termbig_header_in);
  strcat(str1, device.nazev);
  strcat(str1, "/output/force");
  if (strncmp(str1, my_topic, strlen(str1)) == 0)
  {
    shiftout(atoi(my_payload));
  }

  strcpy_P(str1, termbig_header_in);
  strcat(str1, device.nazev);
  strcat(str1, "/output/reset");
  if (strncmp(str1, my_topic, strlen(str1)) == 0)
  {
    mqtt_process_message++;
    output_reset(atoi(my_payload));
  }

  //////// ovladani vystupu
  strcpy_P(str1, termbig_header_in);
  strcat(str1, "virtual-output/");
  if (strncmp(str1, my_topic, strlen(str1)) == 0)
  {
    mqtt_process_message++;
    cnt = 0;
    for (uint8_t f = strlen(str1); f < strlen(my_topic); f++)
    {
      str1[cnt] = topic[f];
      str1[cnt + 1] = 0;
      cnt++;
    }
    cnt = 0;
    pch = strtok (str1, "/");
    while (pch != NULL)
    {
      if (cnt == 0) id = atoi(pch);
      if (cnt == 1) if (strcmp(pch, "state") == 0) output_set_function(id, OUTPUT_REAL_MODE_STATE, atoi(my_payload));
      if (cnt == 1) if (strcmp(pch, "tri-state") == 0) output_set_function(id, OUTPUT_REAL_MODE_TRISTATE, atoi(my_payload));
      if (cnt == 1) if (strcmp(pch, "heat-pwm") == 0) output_set_function(id, OUTPUT_REAL_MODE_PWM, atoi(my_payload));
      if (cnt == 1) if (strcmp(pch, "cool-pwm") == 0) output_set_function(id, OUTPUT_REAL_MODE_PWM, atoi(my_payload));
      if (cnt == 1) if (strcmp(pch, "fan-pwm") == 0) output_set_function(id, OUTPUT_REAL_MODE_PWM, atoi(my_payload));
      if (cnt == 1) if (strcmp(pch, "err-pwm") == 0) output_set_function(id, OUTPUT_REAL_MODE_NONE, atoi(my_payload));
      //if (cnt == 1) if (strcmp(pch, "delay") == 0) output_set_function(id, OUTPUT_REAL_MODE_DELAY, atoi(my_payload));
      pch = strtok (NULL, "/");
      cnt++;
    }
  }

  //// thermctl-in/XXXXX/reload
  strcpy_P(str1, termbig_header_in);
  strcat(str1, device.nazev);
  strcat(str1, "/reload");
  if (strcmp(str1, my_topic) == 0)
  {
    mqtt_process_message++;
    esp_restart();
  }
  //// thermctl-in/XXXXX/bootloader
  strcpy_P(str1, termbig_header_in);
  strcat(str1, device.nazev);
  strcat(str1, "/bootloader");
  if (strcmp(str1, my_topic) == 0)
  {
    mqtt_process_message++;
    EEPROM.write(bootloader_tag, 255);
    xTaskCreate(&ota_task, "ota_task", 8192, NULL, 5, NULL);
  }
  //// /thermctl-in/XXXXX/default
  strcpy_P(str1, termbig_header_in);
  strcat(str1, device.nazev);
  strcat(str1, "/default");
  if (strcmp(str1, my_topic) == 0)
  {
    mqtt_process_message++;
    EEPROM.write(set_default_values, atoi(my_payload));
  }

}



bool mqtt_reconnect(void)
{
  char nazev[10];
  char topic[26];
  bool ret = false;
  device_get_name(nazev);
  strcpy_P(topic, termbig_header_in);
  strcat(topic, nazev);
  strcat(topic, "/#");

  esp_mqtt_client_subscribe(mqtt_client, topic, 0);

  strcpy_P(topic, termbig_header_in);
  strcat(topic, "global/#");
  esp_mqtt_client_subscribe(mqtt_client, topic, 0);

  strcpy_P(topic, thermctl_subscribe);
  esp_mqtt_client_subscribe(mqtt_client, topic, 0);

  strcpy_P(topic, termbig_subscribe);
  esp_mqtt_client_subscribe(mqtt_client, topic, 0);

  strcpy_P(topic, monitor_subscribe);
  esp_mqtt_client_subscribe(mqtt_client, topic, 0);

  strcpy_P(topic, brana_subscribe);
  esp_mqtt_client_subscribe(mqtt_client, topic, 0);

  strcpy_P(topic, termbig_header_in);
  strcat(topic, "virtual-output/#");
  esp_mqtt_client_subscribe(mqtt_client, topic, 0);

  //// /rtds/xxxxx
  for (uint8_t idx = 0; idx < MAX_RTDS; idx++)
     remote_tds_subscibe_topic(mqtt_client, idx);

  ret = true;
  return ret;
}




void print_sha256(const uint8_t *image_hash, const char *label)
{
    char hash_print[HASH_LEN * 2 + 1];
    hash_print[HASH_LEN * 2] = 0;
    for (int i = 0; i < HASH_LEN; ++i) {
        sprintf(&hash_print[i * 2], "%02x", image_hash[i]);
    }
    ESP_LOGI(TAG, "%s %s", label, hash_print);
}


void get_sha256_of_partitions(void)
{
    uint8_t sha_256[HASH_LEN] = { 0 };
    esp_partition_t partition;

    // get sha256 digest for bootloader
    partition.address   = ESP_BOOTLOADER_OFFSET;
    partition.size      = ESP_PARTITION_TABLE_OFFSET;
    partition.type      = ESP_PARTITION_TYPE_APP;
    esp_partition_get_sha256(&partition, sha_256);
    print_sha256(sha_256, "SHA-256 for bootloader: ");

    // get sha256 digest for running partition
    esp_partition_get_sha256(esp_ota_get_running_partition(), sha_256);
    print_sha256(sha_256, "SHA-256 for current firmware: ");
}




void ota_task(void *pvParameter)
{
  ESP_LOGI(TAG, "Starting OTA bootloader");
  ESP_LOGI(TAG, "Only Wifi");
  struct ifreq ifr;
  esp_netif_get_netif_impl_name(wifi_netif, ifr.ifr_name);
  ESP_LOGI(TAG, "Bind interface name is %s", ifr.ifr_name);
  //
  esp_http_client_config_t config = {
  .url = device.bootloader_uri,
  .cert_pem = "",
  .event_handler = http_event_handler,
  .skip_cert_common_name_check = true,
  .if_name = &ifr,
  };
  //
  esp_https_ota_config_t ota_config = {
        .http_config = &config,
    };
  ESP_LOGI(TAG, "Attempting to download update from %s", config.url);
  esp_err_t ret = esp_https_ota(&ota_config);
  if (ret == ESP_OK)
      {
      ESP_LOGI(TAG, "OTA Succeed, Rebooting...");
      esp_restart();
      }
     else
     {
     ESP_LOGE(TAG, "Firmware upgrade failed");
     }
  while (1)
      {
      vTaskDelay(1000 / portTICK_PERIOD_MS);
      }
}

