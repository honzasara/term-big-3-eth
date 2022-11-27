#ifndef EEPROM_H_INCLUDED
#define EEPROM_H_INCLUDED

#include <inttypes.h>
#include <string.h>
#include "esp_err.h"
#include "main.h"
#include <stdio.h>
#include <stdint.h>
#include "esp_log.h"

#define EEPROM_GLOBAL_ADDRESS 0x50


esp_err_t i2c_eeprom_readByte(uint8_t deviceAddress, uint16_t address, uint8_t *data);
esp_err_t i2c_eeprom_writeByte(uint8_t deviceAddress, uint16_t address, uint8_t data);

class c_EEPROM {
   public:
	c_EEPROM(void);
	void write(uint16_t addr, uint8_t data);
        uint8_t read(uint16_t addr);
   };


extern c_EEPROM EEPROM;

#endif
 
