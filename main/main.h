#include "driver/i2c.h"
#include "esp_netif.h"

#ifndef GLOBALH_INCLUDED
#define GLOBALH_INCLUDED

#define HASH_LEN 32


#define ENC28J60_RESET (GPIO_NUM_2)
#define ENC28J60_SCLK_GPIO 18
#define ENC28J60_MOSI_GPIO 23
#define ENC28J60_MISO_GPIO 19
#define ENC28J60_CS_GPIO 5
#define ENC28J60_SPI_CLOCK_MHZ 16
#define ENC28J60_INT_GPIO 15

#define BUTTON_PRESSED 1
#define BUTTON_RELEASED 0
#define BUTTON_HOLD 2
#define BUTTON_NONE 0

#define BUTTON_UP GPIO_NUM_27
#define BUTTON_DOWN GPIO_NUM_14
#define BUTTON_ENTER GPIO_NUM_13

#define GPIO_INPUT_PIN_SEL  ((1ULL<<BUTTON_UP) | (1ULL<<BUTTON_DOWN) | (1ULL<<BUTTON_ENTER))
#define GPIO_OUTPUT_PIN_SEL  (0)


#define portTICK_RATE_MS portTICK_PERIOD_MS

#define I2C_MASTER_SCL_IO GPIO_NUM_22               /*!< gpio number for I2C master clock */
#define I2C_MASTER_SDA_IO GPIO_NUM_21               /*!< gpio number for I2C master data  */

#define ACK_CHECK_EN 0x1                        /*!< I2C master will check ack from slave*/
#define WRITE_BIT I2C_MASTER_WRITE              /*!< I2C master write */
#define READ_BIT I2C_MASTER_READ                /*!< I2C master read */
#define I2C_MASTER_FREQ_HZ 400000
#define ACK_VAL I2C_MASTER_ACK                             /*!< I2C ack value */
#define NACK_VAL I2C_MASTER_NACK                             /*!< I2C nack value */

#define max_output 20


#define I2C_MEMORY_ADDR 0x50
#define DS1307_ADDRESS  0x68

#define RS_TXD  (GPIO_NUM_17)
#define RS_RXD  (GPIO_NUM_16)
#define RS_RTS  (GPIO_NUM_4)
#define RS_CTS  (UART_PIN_NO_CHANGE)

#define pin_input_sync_null (GPIO_NUM_35)
#define pin_output_data	(GPIO_NUM_32)
#define pin_output_clk 	(GPIO_NUM_33)
#define pin_output_strobe 	(GPIO_NUM_25)
#define pin_output_oe 	(GPIO_NUM_26)
#define pin_output_reset 	(GPIO_NUM_34)



#define RS_BUF_SIZE 32

#define DS2482_COUNT 2


#define HW_ONEWIRE_MAXDEVICES 32
#define HW_ONEWIRE_MAXROMS HW_ONEWIRE_MAXDEVICES


#define eeprom_wire_know_rom 300

#define remote_tds_name0 1100
#define ram_remote_tds_store_first 2500

#define MAX_RTDS 20



#define MAX_OUTPUT 32
#define output0  1600


#define bootloader_tag 0
#define set_default_values 90

extern char *TAG;

extern uint32_t wifi_link;
extern uint32_t eth_link;
extern uint32_t mqtt_link;

extern esp_netif_t *eth_netif;
extern esp_netif_t *wifi_netif;

extern uint8_t use_tds;
extern uint8_t use_rtds;

uint32_t  millis(void);

extern i2c_port_t i2c_num;
esp_err_t twi_init(i2c_port_t t_i2c_num);

void shiftout(uint8_t data);

void get_device_status(void);

//void send_mqtt_onewire(void);
void send_mqtt_tds_temp(void);
void send_mqtt_tds_all(void);
void send_device_status(void);
void send_mqtt_remote_tds_status(void);
void send_mqtt_outputs(void);
void send_mqtt_output_quick_state(uint8_t idx, char *mode, uint8_t args);
void send_mqtt_fast_message(char *topic, char *payload);

void mqtt_callback(char* topic, uint8_t * payload, unsigned int length_topic, unsigned int length_data);
bool mqtt_reconnect(void);

uint8_t mode_in_mode_enable(uint8_t mode_enable, uint8_t mode);
void output_set_function(uint8_t id, uint8_t mode, uint8_t args);
uint8_t send_mqtt_outputs_virtual_id(uint8_t id);

void get_sha256_of_partitions(void);
void print_sha256(const uint8_t *image_hash, const char *label);
void ota_task(void *pvParameter);


typedef struct
  {
    uint64_t event_count;
  } timer_queue_element_t;


#endif
