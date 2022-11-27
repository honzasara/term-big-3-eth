#include "EEPROM.h"



c_EEPROM::c_EEPROM()
{

}


void c_EEPROM::write(uint16_t addr, uint8_t data)
{
  i2c_eeprom_writeByte(EEPROM_GLOBAL_ADDRESS, addr, data);
  //ESP_LOGI("EEPROM", "eeprom write addr:%d  -> %d", addr, data);
}



uint8_t c_EEPROM::read(uint16_t addr)
{
 uint8_t data[1];
 i2c_eeprom_readByte(EEPROM_GLOBAL_ADDRESS, addr, data);
 //ESP_LOGI("EEPROM", "eeprom read addr:%d  -> %d", addr, data[0]);
 return data[0];
}

////// nacteni z i2c pameti
esp_err_t i2c_eeprom_readByte(uint8_t deviceAddress, uint16_t address, uint8_t *data)
{
  esp_err_t ret;
  i2c_cmd_handle_t cmd;

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
  i2c_master_read_byte(cmd, data, NACK_VAL);
  i2c_master_stop(cmd);
  ret = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_RATE_MS);
  i2c_cmd_link_delete(cmd);
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


