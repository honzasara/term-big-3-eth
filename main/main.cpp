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
#include "esp_netif.h"
#include "esp_eth.h"
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

#include "saric_tds_function.h"
#include "saric_rtds_function.h"
#include "SSD1306.h"
#include "Font5x8.h"
#include "esp32_saric_mqtt_network.h"
#include <time.h>
#include "saric_virtual_outputs.h"



i2c_port_t i2c_num;

long uptime = 0;

float internal_temp;

uint8_t use_tds = 0;
uint8_t use_rtds = 0;
uint8_t use_rtds_type_temp = 0;


static const char *TAG = "term-big-3-eth";
static httpd_handle_t start_webserver(void);
static esp_err_t prom_metrics_get_handler(httpd_req_t *req);

esp_mqtt_client_handle_t mqtt_client;

esp_netif_t *eth_netif;
uint32_t eth_link = 0;
uint32_t mqtt_link = 0;

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
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%d", base, event_id);
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
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        /*
	if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
            log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
            log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
            log_error_if_nonzero("captured as transport's socket errno",  event->error_handle->esp_transport_sock_errno);
            ESP_LOGI(TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));
        }
	*/
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






/* An HTTP GET handler */
static esp_err_t prom_metrics_get_handler(httpd_req_t *req)
{ 
    const char* resp_str = (const char*) "mrd";
    httpd_resp_set_type(req, (const char*) "text/plain");
    httpd_resp_send(req, resp_str, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}




static const httpd_uri_t prom_metrics = {
    .uri       = "/metrics",
    .method    = HTTP_GET,
    .handler   = prom_metrics_get_handler,
    .user_ctx  = 0
};



 
static httpd_handle_t start_webserver(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.lru_purge_enable = true;

    // Start the httpd server
    ESP_LOGI(TAG, "Starting server on port: '%d'", 80);
    if (httpd_start(&server, &config) == ESP_OK) {
        ESP_LOGI(TAG, "Registering URI handlers");
        httpd_register_uri_handler(server, &prom_metrics);
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


static void stop_webserver(httpd_handle_t server)
{
    // Stop the httpd server
    httpd_stop(server);
}

static void disconnect_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data)
{
    httpd_handle_t* server = (httpd_handle_t*) arg;
    if (*server) {
        ESP_LOGI(TAG, "Stopping webserver");
        stop_webserver(*server);
        *server = NULL;
    }
}

static void connect_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    httpd_handle_t* server = (httpd_handle_t*) arg;
    if (*server == NULL) {
        ESP_LOGI(TAG, "Starting webserver");
        *server = start_webserver();
    }
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
  static httpd_handle_t http_server = NULL;
 
  esp_mqtt_client_config_t mqtt_cfg = {};
  char hostname[10];

  char ip_uri[20];
  char complete_ip_uri[32];
  char str1[32];
  char str2[16];

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

  /*
  //zero-initialize the config structure.
  gpio_config_t io_conf = {};
  //disable interrupt
  io_conf.intr_type = GPIO_INTR_DISABLE;
  //set as output mode
  io_conf.mode = GPIO_MODE_INPUT;
  //bit mask of the pins that you want to set,e.g.GPIO18/19
  io_conf.pin_bit_mask = (1ULL << pin_input_sync_null);
  //disable pull-down mode
  io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
  //disable pull-up mode
  io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
  //configure GPIO with the given settings
  gpio_config(&io_conf);
  */


  gpio_set_level(pin_output_oe, 1);
  gpio_set_level(pin_output_oe, 0);

  twi_init(I2C_NUM_0);



  for (uint8_t init = 0;  init < 11; init++)
  {
    if (init == 0)
      {
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
      if (EEPROM.read(set_default_values) == 255)
         {
         ESP_LOGI(TAG, "Default values");
         EEPROM.write(set_default_values, 0);
         device.mac[0] = 2; device.mac[1] = 1; device.mac[2] = 2; device.mac[3] = 24; device.mac[4] = 25; device.mac[5] = 26;
         device.myIP[0] = 192; device.myIP[1] = 168; device.myIP[2] = 1; device.myIP[3] = 23;
         device.myMASK[0] = 255; device.myMASK[1] = 255; device.myMASK[2] = 255; device.myMASK[3] = 0;

         device.myGW[0] = 192; device.myGW[1] = 168; device.myGW[2] = 1; device.myGW[3] = 1;
         device.myDNS[0] = 192; device.myDNS[1] = 168; device.myDNS[2] = 1; device.myDNS[3] = 1;
         device.mqtt_server[0] = 192; device.mqtt_server[1] = 168; device.mqtt_server[2] = 2; device.mqtt_server[3] = 1;
         device.ntp_server[0] = 192; device.ntp_server[1] = 168; device.ntp_server[2] = 2; device.ntp_server[3] = 1;
         device.mqtt_port = 1883;
	 strcpy(device.nazev, "TERMBIG3");
         strcpy(device.mqtt_user, "saric");
         strcpy(device.mqtt_key, "no");
	 strcpy(device.bootloader_uri, "http://192.168.2.1/termbig/v3.bin");

	 save_setup_network();
         load_setup_network();
         strcpy(hostname, device.nazev);

	 for (uint8_t idx = 0; idx < HW_ONEWIRE_MAXDEVICES; idx++)
         {
          get_tds18s20(idx, &tds);
          strcpy(tds.name, "FREE");
          tds.used = 0;
          tds.offset = 0;
          tds.assigned_ds2482 = 0;
          tds.period = 10;
          for (uint8_t m = 0; m < 8; m++) tds.rom[m] = 0xff;
          set_tds18s20(idx, &tds);
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
          .rx_flow_ctrl_thresh = 122
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
      ESP_ERROR_CHECK(gpio_install_isr_service(0));
      // Initialize TCP/IP network interface (should be called only once in application)
      ESP_ERROR_CHECK(esp_netif_init());
      // Create default event loop that running in background
      ESP_ERROR_CHECK(esp_event_loop_create_default());
      esp_netif_config_t netif_cfg = ESP_NETIF_DEFAULT_ETH();
      eth_netif = esp_netif_new(&netif_cfg);
         
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

      eth_enc28j60_config_t enc28j60_config = ETH_ENC28J60_DEFAULT_CONFIG(spi_handle);
      enc28j60_config.int_gpio_num = ENC28J60_INT_GPIO;

      eth_mac_config_t mac_config = ETH_MAC_DEFAULT_CONFIG();
      esp_eth_mac_t *mac = esp_eth_mac_new_enc28j60(&enc28j60_config, &mac_config);

      eth_phy_config_t phy_config = ETH_PHY_DEFAULT_CONFIG();
      phy_config.autonego_timeout_ms = 0; // ENC28J60 doesn't support auto-negotiation
      phy_config.reset_gpio_num = 2; // ENC28J60 doesn't have a pin to reset internal PHY
      esp_eth_phy_t *phy = esp_eth_phy_new_enc28j60(&phy_config);

      esp_eth_config_t eth_config = ETH_DEFAULT_CONFIG(mac, phy);
      ESP_ERROR_CHECK(esp_eth_driver_install(&eth_config, &eth_handle));

      /* ENC28J60 doesn't burn any factory MAC address, we need to set it manually.
         02:00:00 is a Locally Administered OUI range so should not be used except when testing on a LAN under your control.
      */
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

    if (init == 7)
      {
      ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_GOT_IP, &connect_handler, &http_server));
      ESP_ERROR_CHECK(esp_event_handler_register(ETH_EVENT, ETHERNET_EVENT_DISCONNECTED, &disconnect_handler, &http_server));
      GLCD_SetFont(Font5x8, 5, 8, GLCD_Overwrite);
      GLCD_Clear();
      GLCD_GotoXY(0, 0);
      GLCD_PrintString(".. init ...");
      GLCD_GotoXY(0, 16);
      GLCD_PrintString("7. http server");
      GLCD_Render();
     }

    if (init == 8)
      {
      if (device.dhcp != 255)
	  {
          ESP_ERROR_CHECK(esp_netif_dhcpc_stop(eth_netif));
	  ESP_LOGI(TAG, "DHCP client not enabled");
          }

      ESP_ERROR_CHECK(esp_netif_set_hostname(eth_netif, device.nazev));

      esp_netif_ip_info_t ip_info;
      IP4_ADDR(&ip_info.ip, device.myIP[0], device.myIP[1], device.myIP[2], device.myIP[3]);
      IP4_ADDR(&ip_info.gw, device.myGW[0], device.myGW[1], device.myGW[2], device.myGW[3]);
      IP4_ADDR(&ip_info.netmask, device.myMASK[0], device.myMASK[1], device.myMASK[2], device.myMASK[3]);
      esp_netif_set_ip_info(eth_netif, &ip_info);

      ESP_LOGI(TAG, "Ethernet SET IP Address");
      ESP_LOGI(TAG, "~~~~~~~~~~~");
      ESP_LOGI(TAG, "set ETHIP:" IPSTR, IP2STR(&ip_info.ip));
      ESP_LOGI(TAG, "set ETHMASK:" IPSTR, IP2STR(&ip_info.netmask));
      ESP_LOGI(TAG, "set ETHGW:" IPSTR, IP2STR(&ip_info.gw));
      ESP_LOGI(TAG, "~~~~~~~~~~~");
     
      /* attach Ethernet driver to TCP/IP stack */
      ESP_ERROR_CHECK(esp_netif_attach(eth_netif, esp_eth_new_netif_glue(eth_handle)));

      // Register user defined event handers
      ESP_ERROR_CHECK(esp_event_handler_register(ETH_EVENT, ESP_EVENT_ANY_ID, &eth_event_handler, NULL));
      ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_GOT_IP, &got_ip_event_handler, NULL));

      eth_duplex_t duplex = ETH_DUPLEX_FULL;
      ESP_ERROR_CHECK(esp_eth_ioctl(eth_handle, ETH_CMD_S_DUPLEX_MODE, &duplex));

      /* start Ethernet driver state machine */
      ESP_ERROR_CHECK(esp_eth_start(eth_handle));

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


  if (init == 9)
     {
      send_mqtt_set_header(termbig_header_out);
      strcpy(complete_ip_uri, "mqtt://");
      createString(ip_uri, '.', device.mqtt_server, 4, 10, 1);
      strcat(complete_ip_uri, ip_uri);
      ESP_LOGI(TAG, "mqtt_uri: %s", complete_ip_uri);

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


  vTaskDelay(500 / portTICK_PERIOD_MS);
  }
}





///void callback_1_milisec(void* arg)
void callback_1_milisec(void)
{
  uint16_t period_now, period;
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





void callback_1_sec(void* arg)
{
  ESP_LOGD(TAG, "timer 1 sec");
  uptime++;
}




void core0Task( void * pvParameters )
{
  char str1[32];
  char str2[32];

  while(1)
  {
    if ((uptime % 2) == 0)
      {
      use_rtds = count_use_rtds(&use_rtds_type_temp);
      use_tds = count_use_tds();

      mereni_hwwire();
      send_mqtt_onewire();
      send_mqtt_tds(); 
      send_mqtt_status(mqtt_client);
      update_know_mqtt_device();
      send_know_device();
      send_device_status();
      send_mqtt_remote_tds_status();
      send_mqtt_outputs();
      }   

    if ((uptime % 10) == 0)
      {
      strcpy_P(str2, termbig_subscribe);
      device_get_name(str1);
      send_mqtt_payload(mqtt_client, str2, str1);
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

  esp_err_t err = nvs_flash_init();
  if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
  }
  ESP_ERROR_CHECK(err);

  get_sha256_of_partitions();


  const esp_timer_create_args_t periodic_timer_args_1_sec = {.callback = &callback_1_sec, .name = "1_sec" };

  setup();

  xTaskCreatePinnedToCore(core0Task, "core0Task", 10000, NULL, 0, NULL, 0);   
  xTaskCreatePinnedToCore(core1Task, "core1Task", 10000, NULL, 0, NULL, 1);
  
  
  esp_timer_handle_t periodic_timer_1_sec;
  esp_timer_create(&periodic_timer_args_1_sec, &periodic_timer_1_sec);
  esp_timer_start_periodic(periodic_timer_1_sec, 1000000);


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

  ESP_ERROR_CHECK(gptimer_set_alarm_action(gptimer_1ms, &timer_alarm_config_1ms));
  ESP_ERROR_CHECK(gptimer_start(gptimer_1ms));

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



/*************************************************************************************************************************/
///
/// funkce pro odeslani informaci o 1wire zarizenich
/*
   /thermctl_out/XXXXX/1wire/count
   /thermctl_out/XXXXX/1wire/IDcko/rom
*/
void send_mqtt_onewire(void)
{
  char payload[32];
  itoa(Global_HWwirenum, payload, 10);
  send_mqtt_general_payload(mqtt_client, "1wire/count", payload);
  for (uint8_t i = 0; i < Global_HWwirenum; i++)
  {
    createString(payload, ':', w_rom[i].rom, 8, 16, 2);
    send_mqtt_message_prefix_id_topic_payload(mqtt_client, "1wire", i, "rom", payload);
    ///
    itoa(w_rom[i].assigned_ds2482, payload, 10);
    send_mqtt_message_prefix_id_topic_payload(mqtt_client, "1wire", i, "assigned", payload);
    ///
    itoa(w_rom[i].tds_idx, payload, 10);
    send_mqtt_message_prefix_id_topic_payload(mqtt_client, "1wire", i, "tds_idx", payload);
  }
}

/// funkce pro odeslani informaci o tds sensoru
//// /thermctl-out/XXXXX/tds/ID/temp
//// /thermctl-out/XXXXX/tds/ID/name
//// /thermctl-out/XXXXX/tds/ID/offset
//// /thermctl-out/XXXXX/tds/ID/online
//// /thermctl-out/XXXXX/tds/ID/rom
//// /thermctl-out/XXXXX/tds/ID/period
void send_mqtt_tds(void)
{
  struct_DDS18s20 tds;
  char payload[32];
  for (uint8_t id = 0; id < HW_ONEWIRE_MAXROMS; id++)
    if (get_tds18s20(id, &tds) == 1)
      if (tds.used == 1 && status_tds18s20[id].online == True)
      {
        itoa(status_tds18s20[id].temp, payload, 10);
        send_mqtt_message_prefix_id_topic_payload(mqtt_client, "tds", id, "temp", payload);
        itoa(status_tds18s20[id].average_temp_now , payload, 10);
        send_mqtt_message_prefix_id_topic_payload(mqtt_client, "tds", id, "temp_avg", payload);
        strcpy(payload, tds.name);
        send_mqtt_message_prefix_id_topic_payload(mqtt_client, "tds", id, "name", payload);
        itoa(tds.offset, payload, 10);
        send_mqtt_message_prefix_id_topic_payload(mqtt_client, "tds", id, "offset", payload);
        itoa(status_tds18s20[id].online, payload, 10);
        send_mqtt_message_prefix_id_topic_payload(mqtt_client, "tds", id, "online", payload);
        payload[0] = 0;
        createString(payload, ':', tds.rom, 8, 16, 2);
        send_mqtt_message_prefix_id_topic_payload(mqtt_client, "tds", id, "rom", payload);
        itoa(tds.period, payload, 10);
        send_mqtt_message_prefix_id_topic_payload(mqtt_client, "tds", id, "period", payload);
        utoa(status_tds18s20[id].period_now, payload, 10);
        send_mqtt_message_prefix_id_topic_payload(mqtt_client, "tds", id, "start_at", payload);
        itoa(status_tds18s20[id].crc_error, payload, 10);
        send_mqtt_message_prefix_id_topic_payload(mqtt_client, "tds", id, "crc_error", payload);
      }
}

void send_know_device(void)
{
  char payload[64];
  char str2[18];
  strcpy_P(str2, text_know_mqtt_device);
  for (uint8_t idx = 0; idx < MAX_KNOW_MQTT_INTERNAL_RAM; idx++)
  {
    if (know_mqtt[idx].type != TYPE_FREE)
    {
      itoa(know_mqtt[idx].type, payload, 10);
      send_mqtt_message_prefix_id_topic_payload(mqtt_client, str2, idx, "type", payload);
      itoa(know_mqtt[idx].last_update, payload, 10);
      send_mqtt_message_prefix_id_topic_payload(mqtt_client, str2, idx, "last_update", payload);
      strcpy(payload, know_mqtt[idx].device);
      send_mqtt_message_prefix_id_topic_payload(mqtt_client, str2, idx, "device", payload);
    }
  }
}


void send_device_status(void)
{
  char str_topic[32];
  char payload[16];
  long time_now=0;

  strcpy(str_topic, "status/uptime");
  ltoa(uptime, payload, 10);
  send_mqtt_general_payload(mqtt_client, str_topic, payload);

  //itoa(time_get_offset(), payload, 10);
  //send_mqtt_general_payload(mqtt_client, "status/time/ntp_offset", payload);

  //dtostrf(internal_temp, 4, 2, payload);
  //send_mqtt_general_payload(mqtt_client, "status/internal_temp", payload);

  time_now = DateTime(__DATE__, __TIME__).unixtime();
  sprintf(payload, "%ld", time_now);
  send_mqtt_general_payload(mqtt_client, "status/build_time", payload);

  itoa(esp_get_free_heap_size()/1024, payload, 10);
  send_mqtt_general_payload(mqtt_client, "status/heap", payload);

  strcpy(str_topic, "status/rtds/count");
  itoa(use_rtds, payload, 10);
  send_mqtt_general_payload(mqtt_client, str_topic, payload);
}


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

      tt = output.outputs;
      itoa(tt, payload, 10);
      send_mqtt_message_prefix_id_topic_payload(mqtt_client, "output", id, "outputs", payload);

      tt = output.mode_enable;
      itoa(tt, payload, 10);
      send_mqtt_message_prefix_id_topic_payload(mqtt_client, "output", id, "mode-enable", payload);

      tt = output.mode_now;
      itoa(tt, payload, 10);
      send_mqtt_message_prefix_id_topic_payload(mqtt_client, "output", id, "mode-now", payload);

      tt = output.type;
      itoa(tt, payload, 10);
      send_mqtt_message_prefix_id_topic_payload(mqtt_client, "output", id, "type", payload);

      tt = output.id;
      itoa(tt, payload, 10);
      send_mqtt_message_prefix_id_topic_payload(mqtt_client, "output", id, "virtual-id", payload);

      tt = output.state;
      itoa(tt, payload, 10);
      send_mqtt_message_prefix_id_topic_payload(mqtt_client, "output", id, "state", payload);

      tt = output.used;
      itoa(tt, payload, 10);
      send_mqtt_message_prefix_id_topic_payload(mqtt_client, "output", id, "used", payload);

      tt = output.period;
      itoa(tt, payload, 10);
      send_mqtt_message_prefix_id_topic_payload(mqtt_client, "output", id, "period", payload);

      tt = output.state_time_max;
      itoa(tt, payload, 10);
      send_mqtt_message_prefix_id_topic_payload(mqtt_client, "output", id, "state-time-max", payload);

      tt = output.after_start_state;
      itoa(tt, payload, 10);
      send_mqtt_message_prefix_id_topic_payload(mqtt_client, "output", id, "after-start-state", payload);
      
      tt = output.after_start_mode;
      itoa(tt, payload, 10);
      send_mqtt_message_prefix_id_topic_payload(mqtt_client, "output", id, "after-start-mode", payload);
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
  struct_DDS18s20 tds;
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
  ////
  //
  //// termbig-in/XXXXX/rtds-control/register - registruje nove vzdalene cidlo
  strcpy_P(str1, termbig_header_in);
  strcat(str1, device.nazev);
  strcat(str1, "/rtds-control/register");
  if (strncmp(str1, my_topic, strlen(str1)) == 0)
  {
    mqtt_process_message++;
    if (remote_tds_name_exist(my_payload) == 255)
    {
      id = remote_tds_find_free();
      remote_tds_set_complete(id, 1, my_payload);
      remote_tds_subscibe_topic(mqtt_client, id);
    }
    ///TODO - vratit ze jiz existuje
  }
  ///
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
  //// ziska nastaveni remote_tds
  strcpy_P(str1, thermctl_header_in);
  strcat(str1, device.nazev);
  strcat(str1, "/rtds-control/get");
  if (strncmp(str1, my_topic, strlen(str1)) == 0)
  {
    send_mqtt_remote_tds_status();
  }
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
  strcpy_P(str1, new_text_slash_rtds_control_list); /// /rtds-control/list"
  if (strncmp(str1, my_topic, strlen(str1)) == 0)
  {
  }
  ////////
  //// /thermctl-in/XXXX/tds/associate - asociace do tds si pridam mac 1wire - odpoved je pod jakem ID to mam ulozeno
  strcpy_P(str1, termbig_header_in);
  strcat(str1, device.nazev);
  strcat(str1, "/tds/associate");
  if (strcmp(str1, my_topic) == 0)
  {
    mqtt_process_message++;
    id = atoi(my_payload);
    if ( id < Global_HWwirenum)
    {
      for (uint8_t idx = 0; idx < HW_ONEWIRE_MAXDEVICES; idx++)
      {
        get_tds18s20(idx, &tds);
        if (tds.used == 0 && w_rom[id].used == 1)
        {
          tds.used = 1;
          for (uint8_t i = 0; i < 8; i++)
            tds.rom[i] = w_rom[id].rom[i];
          tds.assigned_ds2482 = w_rom[id].assigned_ds2482;
          set_tds18s20(idx, &tds);
          for (uint8_t cnt = 0; cnt < MAX_AVG_TEMP; cnt++)
            status_tds18s20[idx].average_temp[cnt] = 200;
	  status_tds18s20[idx].temp = 200;
          break;
        }
      }
    }
    else
    {
      //log_error(&mqtt_client, "tds/associate bad id");
    }
  }
  ///
  ///
  //// /termbig-in/XXXX/tds/set/IDcko/name - nastavi cidlu nazev
  //// /termbig-in/XXXX/tds/set/IDcko/offset
  //// /termbig-in/XXXX/tds/set/IDcko/period
  strcpy_P(str1, termbig_header_in);
  strcat(str1, device.nazev);
  strcat(str1, "/tds/set/");
  if (strncmp(str1, my_topic, strlen(str1)) == 0)
  {
    mqtt_process_message++;
    cnt = 0;
    for (uint8_t f = strlen(str1); f < strlen(my_topic); f++)
    {
      str1[cnt] = my_topic[f];
      str1[cnt + 1] = 0;
      cnt++;
    }
    cnt = 0;
    pch = strtok (str1, "/");
    while (pch != NULL)
    {
      if (cnt == 0) id = atoi(pch);
      if (id < HW_ONEWIRE_MAXROMS)
      {
        if ((cnt == 1) && (strcmp(pch, "name") == 0)) tds_set_name(id, my_payload);
        if ((cnt == 1) && (strcmp(pch, "offset") == 0)) tds_set_offset(id, atoi(my_payload));
        if ((cnt == 1) && (strcmp(pch, "period") == 0)) tds_set_period(id, atoi(my_payload));
      }
      else
      {
        //log_error(&mqtt_client, "tds/set bad id");
      }
      pch = strtok (NULL, "/");
      cnt++;
    }
  }
  ////////
  ////
  //// /termbig-in/XXXX/tds/clear
  strcpy_P(str1, termbig_header_in);
  strcat(str1, device.nazev);
  strcat(str1, "/tds/clear");
  if (strncmp(str1, my_topic, strlen(str1)) == 0)
  {
    mqtt_process_message++;
    id = atoi(my_payload);
    if (id < HW_ONEWIRE_MAXROMS)
      tds_set_clear(id);
    else
    {
      //log_error(&mqtt_client, "tds/clear bad id");
    }
  }


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
        if ((cnt == 1) && (strcmp(pch, "used") == 0)) {output_update_used(id, atoi(my_payload)); output_sync_to_eeprom_idx(id);}
        if ((cnt == 1) && (strcmp(pch, "mode-enable") == 0)) {output_update_mode_enable(id, atoi(my_payload)); output_sync_to_eeprom_idx(id);}
        if ((cnt == 1) && (strcmp(pch, "type") == 0)) {output_update_type(id, atoi(my_payload)); output_sync_to_eeprom_idx(id);}
        if ((cnt == 1) && (strcmp(pch, "outputs") == 0)) {output_update_outputs(id, atoi(my_payload)); output_sync_to_eeprom_idx(id);}
        if ((cnt == 1) && (strcmp(pch, "virtual-id") == 0)) {output_update_id(id, atoi(my_payload)); output_sync_to_eeprom_idx(id);}
        if ((cnt == 1) && (strcmp(pch, "name") == 0)) {output_set_name(id, my_payload); output_sync_to_eeprom_idx(id);}
        if ((cnt == 1) && (strcmp(pch, "period") == 0)) {output_update_period(id, atoi(my_payload)); output_sync_to_eeprom_idx(id);}
        if ((cnt == 1) && (strcmp(pch, "reset-period") == 0)) {output_reset_period_timer(id); output_sync_to_eeprom_idx(id);}
	if ((cnt == 1) && (strcmp(pch, "state-time-max") == 0)) {output_update_state_time_max(id, atoi(my_payload)); output_sync_to_eeprom_idx(id);}
	if ((cnt == 1) && (strcmp(pch, "sync_to_eeprom") == 0))output_sync_to_eeprom_idx(id);
	if ((cnt == 1) && (strcmp(pch, "sync_from_eeprom") == 0))output_sync_from_eeprom_idx(id);
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



  ////
  //// ziskani nastaveni site
  strcpy_P(str1, termbig_header_in);
  strcat(str1, device.nazev);
  strcat(str1, "/network/get/config");
  if (strncmp(str1, my_topic, strlen(str1)) == 0)
  {
    mqtt_process_message++;
    send_network_config(mqtt_client);
  }
  ////
  /// nastaveni site
  //// thermctl-in/XXXXX/network/set/mac
  //// thermctl-in/XXXXX/network/set/ip
  //// thermctl-in/XXXXX/network/set/netmask
  //// thermctl-in/XXXXX/network/set/gw
  //// thermctl-in/XXXXX/network/set/dns
  //// thermctl-in/XXXXX/network/set/ntp
  //// thermctl-in/XXXXX/network/set/mqtt_host
  //// thermctl-in/XXXXX/network/set/mqtt_port
  //// thermctl-in/XXXXX/network/set/mqtt_user
  //// thermctl-in/XXXXX/network/set/mqtt_key
  //// thermctl-in/XXXXX/network/set/name
  strcpy_P(str1, termbig_header_in);
  strcat(str1, device.nazev);
  strcat(str1, "/network/set/");
  if (strncmp(str1, my_topic, strlen(str1)) == 0)
  {
    mqtt_process_message++;
    cnt = 0;
    for (uint8_t f = strlen(str1); f < strlen(my_topic); f++)
    {
      str1[cnt] = my_topic[f];
      str1[cnt + 1] = 0;
      cnt++;
    }
    active = setting_network(str1, my_payload);
    save_setup_network();
    if (active == 1)
    	esp_restart();
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
    ESP_LOGI(TAG, "Starting OTA example");
    struct ifreq ifr;
    esp_netif_get_netif_impl_name(eth_netif, ifr.ifr_name);
    ESP_LOGI(TAG, "Bind interface name is %s", ifr.ifr_name);
    
    esp_http_client_config_t config = {
    .url = device.bootloader_uri,
    .cert_pem = "",
    .event_handler = http_event_handler,
    .skip_cert_common_name_check = true,
    .if_name = &ifr,
    };

    ESP_LOGI(TAG, "Attempting to download update from %s", config.url);
    esp_err_t ret = esp_https_ota(&config);
    if (ret == ESP_OK) {
        esp_restart();
    } else 
    {
        ESP_LOGE(TAG, "Firmware upgrade failed");
    }
    while (1) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

