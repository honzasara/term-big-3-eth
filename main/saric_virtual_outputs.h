#ifndef __SARIC_VIRTUAL_OUTPUTS_H
#define __SARIC_VIRTUAL_OUTPUTS_H

#include <stdlib.h>
#include <inttypes.h>
#include "EEPROM.h"
#include "saric_utils.h"

#define output_store_size_byte 21

#if !defined(output0)
#define output0  1000
#endif

#if !defined(MAX_OUTPUT)
#define MAX_OUTPUT 32
#endif

#define output_store_last output0 + (output_store_size_byte * MAX_OUTPUT)


#pragma message( "output_store_last  = " XSTR(output_store_last))

#define PERIOD_50_MS 50

#define OUTPUT_REAL_MODE_NONE 0
#define OUTPUT_REAL_MODE_STATE 1
#define OUTPUT_REAL_MODE_PWM 2
#define OUTPUT_REAL_MODE_DELAY 3
#define OUTPUT_REAL_MODE_TRISTATE 4

#define OUTPUT_TYPE_HW 2
#define OUTPUT_TYPE_RS485 1

#define POWER_OUTPUT_ERR 255
#define POWER_OUTPUT_OFF 254
#define POWER_OUTPUT_NEGATE 253
#define POWER_OUTPUT_ON 252
#define POWER_OUTPUT_TEST 251

#define TERM_MODE_OFF 0
#define TERM_MODE_HEAT_MAX 1
#define TERM_MODE_PROG 2
#define TERM_MODE_MAN_HEAT 3
#define TERM_MODE_CLIMATE_MAX 4
#define TERM_MODE_MAN_COOL 5
#define TERM_MODE_FAN 6
//#define TERM_MODE_MIN 7
#define TERM_MODE_MAN 8
#define TERM_MODE_ERR 254
#define TERM_MODE_FREE 255


#define POWER_OUTPUT_HEAT_MAX 10
#define POWER_OUTPUT_COOL_MAX 11
#define POWER_OUTPUT_FAN_MAX 12

#define POWER_OUTPUT_FIRST 21
#define POWER_OUTPUT_SECOND 22
#define POWER_OUTPUT_FIRST_TIMER 23
#define POWER_OUTPUT_SECOND_TIMER 24

extern uint8_t output_last;

typedef struct struct_output
{
  uint8_t used;
  char name[8];
  uint8_t outputs;
  uint8_t mode_enable;
  uint8_t mode_now;
  uint16_t period;
  uint16_t period_timer_now;
  uint8_t state;
  uint8_t state_timer_now;
  uint8_t id;
  uint8_t type;
  uint8_t state_time_max;
  uint8_t after_start_state;
  uint8_t after_start_mode;
};


/*
uint8_t index_update_output;
uint8_t output_state[MAX_OUTPUT];
uint8_t output_state_timer[MAX_OUTPUT];

uint16_t output_period_timer[MAX_OUTPUT];
uint16_t output_set_period_timer[MAX_OUTPUT];

uint8_t output_mode_now[MAX_OUTPUT];
*/
/*
   usporadani pameti output
   0..9 -- nazev
   10 - used - vystup je aktivni/neni aktivni
   11 - outputs - 0..7 vystupu, bitove pole
   12 - mode_enable - jake jsou poveleny mody pro dany vystup
       - mode_now - aktualni nastaveny mod
       - period_timer - zakladni casovy interval pro pwm
   14 - low period - pro pwm
   15 - high period
      - state - v jakem nastavenem stavu vystup je. 0 / 1, pwm cislo
   16 - type - HW, VIRTUAL, RS485....
   17 - virtualni id
   18 - max_state_time
   19 - after_start_state
   20 - after_statr_mode
*/


void output_update_used(uint8_t idx, uint8_t used);
void output_update_period(uint8_t idx, uint16_t period);
void output_update_id(uint8_t idx, uint8_t id);
void output_update_outputs(uint8_t idx, uint8_t outputs);
void output_update_type(uint8_t idx, uint8_t type);
void output_update_mode_enable(uint8_t idx, uint8_t mode);
void output_set_all(uint8_t idx, struct_output output);
void output_reset_period_timer(uint8_t idx);
void output_inc_period_timer(uint8_t idx);
uint16_t output_get_period_time(uint8_t idx);
void output_set_set_period_time(uint8_t idx, uint16_t period);
uint16_t output_get_set_period_time(uint8_t idx);
void output_get_name(uint8_t idx, char *name);
void output_set_name(uint8_t idx, char *name);
void output_get_variables(uint8_t idx, uint8_t *used, uint8_t *mode_enable, uint8_t *type, uint8_t *outputs, uint8_t *id, uint16_t *period, uint8_t *state_time_max, uint8_t *after_start_state, uint8_t *after_start_mode);
void output_set_variables(uint8_t idx, uint8_t used, uint8_t mode_enable, uint8_t type, uint8_t outputs, uint8_t id, uint16_t period, uint8_t state_time_max, uint8_t after_start_state, uint8_t after_start_mode);
void output_get_all(uint8_t idx, struct_output *output);
void output_get_all_no_name(uint8_t idx, struct_output *output);
void output_set_mode_now(uint8_t idx, uint8_t mode);

void output_get_all_eeprom(uint8_t idx, struct_output *output);
void output_set_all_eeprom(uint8_t idx, struct_output output);


void output_set_state(uint8_t idx, uint8_t state);
void output_inc_state_timer(uint8_t idx);
uint8_t output_get_state_timer(uint8_t idx);
void output_reset_state_timer(uint8_t idx);

void output_update_state_time_max(uint8_t idx, uint8_t state_time_max);

void output_get_period_for_pwm(uint8_t idx, uint16_t *period, uint16_t *period_now);
uint16_t output_get_set_period_time(uint8_t idx);

uint16_t output_get_period_time(uint8_t idx);

void output_reset(uint8_t idx);

void update_phy_output(void);
void phy_set_output(uint8_t bite, uint8_t state);
uint8_t phy_get_output(uint8_t bite);

uint8_t mode_in_mode_enable(uint8_t mode_enable, uint8_t mode);

void output_set_function(uint8_t id, uint8_t mode, uint8_t args);

void output_sync_to_eeprom(void);
void output_sync_from_eeprom(void);
void output_sync_to_eeprom_idx(uint8_t idx);
void output_sync_from_eeprom_idx(uint8_t idx);

void outputs_variable_init(void);
#endif

