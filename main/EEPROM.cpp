#include "EEPROM.h"


c_EEPROM::c_EEPROM()
{

}


bool c_EEPROM::write(uint16_t addr, uint8_t data)
{
    esp_err_t ret;
    ret = i2c_eeprom_writeByte(EEPROM_GLOBAL_ADDRESS, addr, data);
    if (ret == ESP_OK)
        return true;
    else
       {
       ESP_LOGE("EEPROM", "write error %d", ret);
       return false;
       }
}

bool c_EEPROM::read_array(uint16_t addr, uint8_t count, uint8_t *data)
{
    esp_err_t ret;
    ret = i2c_eeprom_readByte(EEPROM_GLOBAL_ADDRESS, addr, count, data);
    if (ret == ESP_OK)
        {
        return true;
        }
    else
        {
        ESP_LOGE("EEPROM", "read_array return false");
        return false;
        }
}

uint8_t c_EEPROM::read(uint16_t addr, bool *retx)
{
    uint8_t data[1];
    esp_err_t ret;
    ret = i2c_eeprom_readByte(EEPROM_GLOBAL_ADDRESS, addr, 1, data);
    if (ret == ESP_OK)
        {
        *retx = true;
        return data[0];
        }
    else
        {
        *retx = false;
        ESP_LOGE("EEPROM", "read return false");
        return 0;
        }
}

////// nacteni z i2c pameti
esp_err_t i2c_eeprom_readByte(uint8_t deviceAddress, uint16_t address, uint8_t count, uint8_t *data)
{
    
    esp_err_t ret;
    i2c_cmd_handle_t cmd;

    /*
    cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, deviceAddress << 1 | WRITE_BIT, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, (address >> 8), ACK_CHECK_EN);
    i2c_master_write_byte(cmd, (address & 0xFF), ACK_CHECK_EN);
    i2c_master_stop(cmd);
    ret = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    if (ret != ESP_OK)
      return ret;

    cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, deviceAddress << 1 | READ_BIT, ACK_CHECK_EN);

    if (count > 1) 
        {
        i2c_master_read(cmd, data, count - 1, ACK_VAL);
        }
    i2c_master_read_byte(cmd, data + count - 1, NACK_VAL);

    i2c_master_stop(cmd);
    ret = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    */

    /*
    cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (deviceAddress<<1) | WRITE_BIT, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, address >> 8, ACK_CHECK_EN);
    //i2c_master_write_byte(cmd, address & 0xFF, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, address & 0xFF, I2C_MASTER_ACK);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (deviceAddress<<1) | READ_BIT, ACK_CHECK_EN);

    if (count > 1) 
        {
        i2c_master_read(cmd, data, count-1, I2C_MASTER_ACK);
        }
    i2c_master_read_byte(cmd, data + count-1, I2C_MASTER_NACK);
    i2c_master_stop(cmd);
    ret = i2c_master_cmd_begin(i2c_num, cmd, 1000/portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    */

    uint8_t addr[2];
    addr[0] = address >> 8;
    addr[1] = address & 0xFF;
    ret = i2c_master_write_read_device(i2c_num, deviceAddress, addr, 2, data, count, 2000/portTICK_PERIOD_MS); 

    return ret;
}

///// zapis do i2c pameti
esp_err_t i2c_eeprom_writeByte(uint8_t deviceAddress, uint16_t address, uint8_t data)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, deviceAddress << 1 | WRITE_BIT, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, (address >> 8), ACK_CHECK_EN);
    i2c_master_write_byte(cmd, (address & 0xFF), ACK_CHECK_EN);
    i2c_master_write_byte(cmd, data, ACK_CHECK_EN);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    if (ret != ESP_OK)
      return ret;

    uint8_t write_ok = 0;
    uint8_t wait_cnt = 0;
    while (write_ok == 0 && wait_cnt < 250)
    {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, deviceAddress << 1 | WRITE_BIT, ACK_CHECK_EN);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    if (ret == ESP_OK)
      {
        write_ok = 1;
      }
    else
      {
        wait_cnt++;
      }
    }
    return ret;
}


