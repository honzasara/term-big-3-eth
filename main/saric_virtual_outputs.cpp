#include "saric_virtual_outputs.h"

uint8_t index_update_output;
uint8_t output_state[MAX_OUTPUT];
uint8_t output_state_timer[MAX_OUTPUT];
uint16_t output_period_timer[MAX_OUTPUT];
uint16_t output_set_period_timer[MAX_OUTPUT];
uint8_t output_mode_now[MAX_OUTPUT];
uint8_t output_last = 255;

struct_output output_sync[MAX_OUTPUT];




void output_sync_to_eeprom_idx(uint8_t idx)
{
   struct_output tmp_output;
   strcpy(tmp_output.name, output_sync[idx].name);
   tmp_output.used = output_sync[idx].used;
   tmp_output.outputs = output_sync[idx].outputs;
   tmp_output.mode_enable = output_sync[idx].mode_enable;
   tmp_output.mode_now = output_sync[idx].mode_now;
   tmp_output.period = output_sync[idx].period;
   tmp_output.period_timer_now = output_sync[idx].period_timer_now;
   tmp_output.state = output_sync[idx].state;
   tmp_output.state_timer_now = output_sync[idx].state_timer_now;
   tmp_output.state_time_max = output_sync[idx].state_time_max;
   tmp_output.id = output_sync[idx].id;
   tmp_output.type = output_sync[idx].type;
   tmp_output.after_start_state = output_sync[idx].after_start_state;
   tmp_output.after_start_mode = output_sync[idx].after_start_mode;
   output_set_all_eeprom(idx, tmp_output);
}

void output_sync_to_eeprom(void)
{
   for (uint8_t idx = 0; idx< MAX_OUTPUT; idx++)
   {
   output_sync_to_eeprom_idx(idx);
   }
}


void output_sync_from_eeprom_idx(uint8_t idx)
{
  struct_output tmp_output;
  output_get_all_eeprom(idx, &tmp_output);
  strcpy(output_sync[idx].name, tmp_output.name);
  output_sync[idx].used = tmp_output.used;
  output_sync[idx].outputs = tmp_output.outputs;
  output_sync[idx].mode_enable = tmp_output.mode_enable;
  output_sync[idx].mode_now = tmp_output.mode_now;
  output_sync[idx].period = tmp_output.period;
  output_sync[idx].period_timer_now = tmp_output.period_timer_now;
  output_sync[idx].state = tmp_output.state;
  output_sync[idx].state_timer_now = tmp_output.state_timer_now;
  output_sync[idx].state_time_max = tmp_output.state_time_max;
  output_sync[idx].id = tmp_output.id;
  output_sync[idx].type = tmp_output.type;
  output_sync[idx].after_start_state = tmp_output.after_start_state;
  output_sync[idx].after_start_mode = tmp_output.after_start_mode;
}

void output_sync_from_eeprom(void)
{
  for (uint8_t idx = 0; idx< MAX_OUTPUT; idx++)
    {      
    output_sync_from_eeprom_idx(idx);
    }
}


void outputs_variable_init(void)
{
   for (uint8_t idx = 0; idx < MAX_OUTPUT; idx++)
      {
       output_state[idx] = 0;
       output_mode_now[idx] = 0;
       output_state_timer[idx] = 0;
       output_period_timer[idx] = 0;
      }
}



void output_get_name_eeprom(uint8_t idx, char *name)
{
  for (uint8_t i = 0; i < 9; i++)
  {
    name[i] = EEPROM.read(output0 + (idx * output_store_size_byte) + i);
    name[i + 1] = 0;
  }
}
void output_set_name_eeprom(uint8_t idx, char *name)
{
  for (uint8_t i = 0; i < 10; i++)
  {
    EEPROM.write((output0 + (idx * output_store_size_byte) + i), name[i]);
  }
}

void output_get_name(uint8_t idx, char *name)
{
strcpy(name, output_sync[idx].name);
}

void output_set_name(uint8_t idx, char *name)
{
strcpy(output_sync[idx].name, name);
}
/////////////////////////////////////////////////////////////////////



///////////////////
void output_get_variables_eeprom(uint8_t idx, uint8_t *used, uint8_t *mode_enable, uint8_t *type, uint8_t *outputs, uint8_t *id, uint16_t *period, uint8_t *state_time_max, uint8_t *after_start_state, uint8_t *after_start_mode)
{
  *used = EEPROM.read((uint16_t) output0 + (idx * output_store_size_byte) + 10);
  *outputs = EEPROM.read((uint16_t) output0 + (idx * output_store_size_byte) + 11);
  *mode_enable = EEPROM.read((uint16_t) output0 + (idx * output_store_size_byte) + 12);
  *period = (EEPROM.read((uint16_t) output0 + (idx * output_store_size_byte) + 15) << 8) + EEPROM.read((uint16_t) output0 + (idx * output_store_size_byte) + 14);
  *type = EEPROM.read((uint16_t) output0 + (idx * output_store_size_byte) + 16);
  *id = EEPROM.read((uint16_t) output0 + (idx * output_store_size_byte) + 17);
  *state_time_max = EEPROM.read((uint16_t) output0 + (idx * output_store_size_byte) + 18);
  *after_start_state = EEPROM.read((uint16_t) output0 + (idx * output_store_size_byte) + 19);
  *after_start_mode = EEPROM.read((uint16_t) output0 + (idx * output_store_size_byte) + 20);
}

void output_set_variables_eeprom(uint8_t idx, uint8_t used, uint8_t mode_enable, uint8_t type, uint8_t outputs, uint8_t id, uint16_t period, uint8_t state_time_max, uint8_t after_start_state, uint8_t after_start_mode)
{
  EEPROM.write((uint16_t)(output0 + (idx * output_store_size_byte) + 10), used);
  EEPROM.write((uint16_t)(output0 + (idx * output_store_size_byte) + 11), outputs);
  EEPROM.write((uint16_t)(output0 + (idx * output_store_size_byte) + 12), mode_enable);
  EEPROM.write((uint16_t)(output0 + (idx * output_store_size_byte) + 14), period);
  EEPROM.write((uint16_t)(output0 + (idx * output_store_size_byte) + 15), (period >> 8) & 0xff);
  EEPROM.write((uint16_t)(output0 + (idx * output_store_size_byte) + 16), type);
  EEPROM.write((uint16_t)(output0 + (idx * output_store_size_byte) + 17), id);
  EEPROM.write((uint16_t)(output0 + (idx * output_store_size_byte) + 18), state_time_max);
  EEPROM.write((uint16_t)(output0 + (idx * output_store_size_byte) + 19), after_start_state);
  EEPROM.write((uint16_t)(output0 + (idx * output_store_size_byte) + 20), after_start_mode);
}


void output_get_variables(uint8_t idx, uint8_t *used, uint8_t *mode_enable, uint8_t *type, uint8_t *outputs, uint8_t *id, uint16_t *period, uint8_t *state_time_max, uint8_t *after_start_state, uint8_t *after_start_mode)
{
  *used = output_sync[idx].used;
  *outputs = output_sync[idx].outputs;
  *mode_enable = output_sync[idx].mode_enable;
  *period = output_sync[idx].period;
  *type = output_sync[idx].type;
  *id = output_sync[idx].id;
  *state_time_max = output_sync[idx].state_time_max;
  *after_start_state = output_sync[idx].after_start_state;
  *after_start_mode = output_sync[idx].after_start_mode;
}

void output_set_variables(uint8_t idx, uint8_t used, uint8_t mode_enable, uint8_t type, uint8_t outputs, uint8_t id, uint16_t period, uint8_t state_time_max, uint8_t after_start_state, uint8_t after_start_mode)
{
  output_sync[idx].used = used;
  output_sync[idx].outputs = outputs;
  output_sync[idx].mode_enable = mode_enable;
  output_sync[idx].period = period;
  output_sync[idx].type = type;
  output_sync[idx].id = id;
  output_sync[idx].state_time_max = state_time_max;
  output_sync[idx].after_start_state = after_start_state;
  output_sync[idx].after_start_mode = after_start_mode;
}

////////////////


////////////////
void output_get_all(uint8_t idx, struct_output *output)
{
  output_get_name(idx, output->name);
  output_get_variables(idx, &output->used, &output->mode_enable, &output->type, &output->outputs, &output->id, &output->period, &output->state_time_max, &output->after_start_state, &output->after_start_mode);
  output_set_set_period_time(idx, output->period); /// proc toto volam?
  output->state = output_state[idx];
  output->mode_now = output_mode_now[idx];
  output->period_timer_now = output_get_period_time(idx);
  output->state_timer_now = output_get_state_timer(idx);
}

void output_get_all_eeprom(uint8_t idx, struct_output *output)
{
  output_get_name_eeprom(idx, output->name);
  output_get_variables_eeprom(idx, &output->used, &output->mode_enable, &output->type,\
		              &output->outputs, &output->id, &output->period, &output->state_time_max, &output->after_start_state, &output->after_start_mode);
  output_set_set_period_time(idx, output->period);
  output->state = output_state[idx];
  output->mode_now = output_mode_now[idx];
  output->period_timer_now = output_get_period_time(idx);
  output->state_timer_now = output_get_state_timer(idx);
}



void output_get_all_no_name(uint8_t idx, struct_output *output)
{
  strcpy(output->name, "");
  output_get_variables(idx, &output->used, &output->mode_enable, &output->type,\
		       &output->outputs, &output->id, &output->period, &output->state_time_max, &output->after_start_state, &output->after_start_mode);
  output_set_set_period_time(idx, output->period);
  output->state = output_state[idx];
  output->mode_now = output_mode_now[idx];
  output->period_timer_now = output_get_period_time(idx);
  output->state_timer_now = output_get_state_timer(idx);
}



void output_set_mode_now(uint8_t idx, uint8_t mode)
{
  output_mode_now[idx] = mode;
}

void output_set_state(uint8_t idx, uint8_t state)
{
  output_state[idx] = state;
}

void output_inc_state_timer(uint8_t idx)
{
  output_state_timer[idx]++;
}

uint8_t output_get_state_timer(uint8_t idx)
{
  return output_state_timer[idx];
}

void output_reset_state_timer(uint8_t idx)
{
 output_state_timer[idx] = 0;
}

//////
void output_get_period_for_pwm(uint8_t idx, uint16_t *period, uint16_t *period_now)
{
  *period = output_get_set_period_time(idx);
  *period_now = output_get_period_time(idx);
}
///
uint16_t output_get_set_period_time(uint8_t idx)
{
  return output_set_period_timer[idx];
}
void output_set_set_period_time(uint8_t idx, uint16_t period)
{
  output_set_period_timer[idx] = period;
}

//
uint16_t output_get_period_time(uint8_t idx)
{
  return output_period_timer[idx];
}
///
void output_inc_period_timer(uint8_t idx)
{
  output_period_timer[idx]++;
}
///
void output_reset_period_timer(uint8_t idx)
{
  output_period_timer[idx] = 0;
}
/////



void output_set_all(uint8_t idx, struct_output output)
{
  output_set_name(idx, output.name);
  output_set_variables(idx, output.used, output.mode_enable, output.type, output.outputs, output.id, output.period, output.state_time_max, output.after_start_state, output.after_start_mode);
  output_set_state(idx, output.state);
  output_set_set_period_time(idx, output.period);
}

void output_set_all_eeprom(uint8_t idx, struct_output output)
{
  output_set_name_eeprom(idx, output.name);
  output_set_variables_eeprom(idx, output.used, output.mode_enable, output.type, output.outputs, output.id, output.period, output.state_time_max, output.after_start_state, output.after_start_mode);
  output_set_state(idx, output.state);
  output_set_set_period_time(idx, output.period);
}


void output_set_all_no_name(uint8_t idx, struct_output output)
{
  output_set_variables(idx, output.used, output.mode_enable, output.type, output.outputs, output.id, output.period, output.state_time_max, output.after_start_state, output.after_start_mode);
  output_set_state(idx, output.state);
  output_set_set_period_time(idx, output.period);
}

//////////


//////////
void output_update_used(uint8_t idx, uint8_t used)
{
  struct_output output;
  output_get_all(idx, &output);
  output.used = used;
  output_set_all(idx, output);
}
void output_update_mode_enable(uint8_t idx, uint8_t mode)
{
  struct_output output;
  output_get_all(idx, &output);
  output.mode_enable = mode;
  output_set_all(idx, output);
}
void output_update_type(uint8_t idx, uint8_t type)
{
  struct_output output;
  output_get_all(idx, &output);
  output.type = type;
  output_set_all(idx, output);
}
void output_update_outputs(uint8_t idx, uint8_t outputs)
{
  struct_output output;
  output_get_all(idx, &output);
  output.outputs = outputs;
  output_set_all(idx, output);
}
void output_update_id(uint8_t idx, uint8_t id)
{
  struct_output output;
  output_get_all(idx, &output);
  output.id = id;
  output_set_all(idx, output);
}
void output_update_period(uint8_t idx, uint16_t period)
{
  struct_output output;
  output_get_all(idx, &output);
  output.period = period;
  output_set_all(idx, output);
}


void output_update_state_time_max(uint8_t idx, uint8_t state_time_max)
{
  struct_output output;
  output_get_all(idx, &output);
  output.state_time_max = state_time_max;
  output_set_all(idx, output);
}

void output_reset(uint8_t idx)
{
  char str1[32];
  char str2[16];
  strcpy(str1, "OUT");
  itoa(idx, str2, 10);
  strcat(str1, str2);
  output_set_name(idx, str1);
  output_set_variables(idx, 0, OUTPUT_REAL_MODE_NONE, 0, 0, 0, PERIOD_50_MS, 0, 0, 0);
  output_set_set_period_time(idx, PERIOD_50_MS);
}



/// nastavi registr vystupniho stavu
void phy_set_output(uint8_t bite, uint8_t state)
{
  //printf("bite: %d, state %d\n", bite, state);
  if (state == 0)
    cbi(output_last, bite);
  else
    sbi(output_last, bite);

  ///shiftout(output_last);
}


///// zpetne ziska registr vystupniho stavu
uint8_t phy_get_output(uint8_t bite)
{
  return (output_last & (1 << bite));
}




void update_phy_output(void)
{
  struct_output output;
  uint8_t cnt = 0;
  for (uint8_t idx = 0; idx < MAX_OUTPUT; idx++)
  {
    output_get_all_no_name(idx, &output);

    if (output.mode_now == OUTPUT_REAL_MODE_STATE && output.used == 1)
    {
      if (output.type == OUTPUT_TYPE_HW)
      {
        for (uint8_t oid = 0; oid < 8; oid++)
          if ((output.outputs & (1 << oid)) != 0)
          {
            if (output.state == POWER_OUTPUT_OFF) phy_set_output(oid, 1);
            if (output.state == POWER_OUTPUT_ERR) phy_set_output(oid, 1);
            if (output.state == POWER_OUTPUT_HEAT_MAX) phy_set_output(oid, 0);
            if (output.state == POWER_OUTPUT_COOL_MAX) phy_set_output(oid, 0);
            if (output.state == POWER_OUTPUT_ON) phy_set_output(oid, 0);
            if (output.state == POWER_OUTPUT_FAN_MAX) phy_set_output(oid, 0);
            if (output.state == POWER_OUTPUT_NEGATE)
            {
              if (phy_get_output(oid) == 0)
              {
                output.state = POWER_OUTPUT_OFF;
              }
              else
              {
                output.state = POWER_OUTPUT_ON;
              }
	      output_set_all_no_name(idx, output);
            }
          }
      shiftout(output_last);
      }
    }

    if (output.mode_now == OUTPUT_REAL_MODE_PWM && output.used == 1)
    {
      if (output.type == OUTPUT_TYPE_HW)
      {
        for (uint8_t oid = 0; oid < 8; oid++)
          if ((output.outputs & (1 << oid)) != 0)
          {
            if (output.state >= output.state_timer_now)
              phy_set_output(oid, 0); /// vystupy do zapnuto
            else
              phy_set_output(oid, 1); /// vystupy do vypnuto
          }
      shiftout(output_last);
      }
    }

    if (output.mode_now == OUTPUT_REAL_MODE_TRISTATE && output.used == 1)
    {
      if (output.type == OUTPUT_TYPE_HW)
      {
	 /// funkce 3 stavovy vypinac s casovacem. Po prekroceni casu se automaticky vystup vypina
	 if (output.state_timer_now > output.state_time_max && output.state == POWER_OUTPUT_FIRST_TIMER && output.state == POWER_OUTPUT_SECOND_TIMER)
	    {
	     output.state = POWER_OUTPUT_OFF;
	     output_set_all_no_name(idx, output);
	    }
	 /// funkce 3 stavovy vypinac bez casovace
         if (output.state == POWER_OUTPUT_OFF)
            for (uint8_t oid = 0; oid < 8; oid++)
               if ((output.outputs & (1 << oid)) != 0)
               {
               phy_set_output(oid, 1);
               }
	 /// zapnuty vystup s indexem 0
         if (output.state == POWER_OUTPUT_FIRST || output.state == POWER_OUTPUT_FIRST_TIMER)
            {
            cnt = 0;
            for (uint8_t oid = 0; oid < 8; oid++)
              if ((output.outputs & (1 << oid)) != 0)
              {
                if (cnt == 0)
                  {
                  phy_set_output(oid, 0);
                  cnt++;
                  }
                else
                  {
                  phy_set_output(oid, 1);
                  }
              }
            }
	 /// zapnuty vystup s indexem 1
         if (output.state == POWER_OUTPUT_SECOND || output.state == POWER_OUTPUT_SECOND_TIMER)
            {
            cnt = 0;
            for (uint8_t oid = 0; oid < 8; oid++)
              if ((output.outputs & (1 << oid)) != 0)
              {
              if (cnt == 1)
                {
                phy_set_output(oid, 0);
                }
              else
                {
                phy_set_output(oid, 1);
                cnt++;
                }
              }
            }
         shiftout(output_last);
      }
    }
    /////
    //// zpozdovac
    /*
      if (output.type == OUTPUT_REAL_MODE_DELAY && output.used == 1)
      {
      if (output.type == OUTPUT_TYPE_HW)
      {
      for (uint8_t oid = 0; oid < 8; oid++)
          if (output.outputs & (1 << idx) != 0)
          {
          /// ted akce
          /// zastavit citac
          }
      }
      }
    */
  }
}


uint8_t mode_in_mode_enable(uint8_t mode_enable, uint8_t mode)
{
  uint8_t ret = 0;
  for (uint8_t idx = 0; idx < 8; idx++)
    if  ((mode_enable & (1 << idx)) != 0  && (mode & (1 << idx)) != 0)
    {
      ret = 1;
      break;
    }
  return ret;
}


///////////////////
void output_set_function(uint8_t id, uint8_t mode, uint8_t args)
{
  struct_output output;
  char str1[16];
  for (uint8_t idx = 0; idx < MAX_OUTPUT; idx++)
  {
    output_get_all_no_name(idx, &output);
    if (output.used != 0 && output.id == id && mode_in_mode_enable(output.mode_enable, mode) == 1)
    {
      if (mode == OUTPUT_REAL_MODE_STATE)
      {
        output_set_state(idx, args);
        output_set_mode_now(idx, mode);
        strcpy(str1, "set-state");
        send_mqtt_output_quick_state(output.id, str1, args);
      }
      if (mode == OUTPUT_REAL_MODE_PWM)
      {
        output_set_state(idx, args);
        output_set_mode_now(idx, mode);
        strcpy(str1, "set-pwm");
        send_mqtt_output_quick_state(output.id, str1, args);
      }
      if (mode == OUTPUT_REAL_MODE_TRISTATE)
      {
        output_set_state(idx, args);
        output_set_mode_now(idx, mode);
	output_reset_state_timer(idx);
        strcpy(str1, "set-tristate");
        send_mqtt_output_quick_state(output.id, str1, args);
      }
      /*
        if (mode == OUTPUT_REAL_MODE_DELAY)
        {
        output_set_state(idx, args);
        output_set_mode_now(idx, mode);
        strcpy(str1, "delay");
        send_mqtt_output_quick_state(output.id, str1, args);
        }
      */
    }
  }
}

