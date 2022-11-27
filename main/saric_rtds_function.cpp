
#include "saric_rtds_function.h"


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///   SEKCE NASTAVENI REMOTE TDS CIDEL ///
/*
  idx - index rtds
  active - ukazatel na promenou active
  name - ukazatel na nazev topicu
*/
/// ziska nazev topicu
void remote_tds_get_complete(uint8_t idx, uint8_t *active, char *name)
{
  char t;
  if (idx < MAX_RTDS)
  {
    for (uint8_t i = 0; i < RTDS_DEVICE_STRING_LEN; i++)
    {
      t = EEPROM.read(remote_tds_name0 + (RTDS_DEVICE_TOTAL_LEN * idx) + i);
      name[i] = t;
      name[i + 1] = 0;
      if (t == 0) break;
    }
    *active = EEPROM.read(remote_tds_name0 + (RTDS_DEVICE_TOTAL_LEN * idx) + RTDS_DEVICE_ACTIVE_BYTE_POS);
  }
  else
  {
    *active = 255;
  }
}
///
void remote_tds_set_complete(uint8_t idx, uint8_t active, char *name)
{
  remote_tds_set_name(idx, name);
  remote_tds_set_active(idx, active);
}

/// nastavi topic
void remote_tds_set_name(uint8_t idx,  char *name)
{
  char t;
  if (idx < MAX_RTDS)
    for (uint8_t i = 0; i < (RTDS_DEVICE_STRING_LEN); i++)
    {
      t = name[i];
      EEPROM.write(remote_tds_name0 + (RTDS_DEVICE_TOTAL_LEN * idx) + i, t);
      if (t == 0) break;
    }
}
///
/// je aktivni
void remote_tds_get_active(uint8_t idx, uint8_t *active)
{
  if (idx < MAX_RTDS)
  {
    *active = EEPROM.read(remote_tds_name0 + (RTDS_DEVICE_TOTAL_LEN * idx) + RTDS_DEVICE_ACTIVE_BYTE_POS);
  }
  else
    *active = 255;
}


void remote_tds_set_active(uint8_t idx, uint8_t active)
{
  if (idx <  MAX_RTDS)
    EEPROM.write(remote_tds_name0 + (RTDS_DEVICE_TOTAL_LEN * idx) + RTDS_DEVICE_ACTIVE_BYTE_POS, active);
}


///
void remote_tds_clear(uint8_t idx)
{
  char rtds_name[RTDS_DEVICE_STRING_LEN];
  for (uint8_t i = 0; i < RTDS_DEVICE_STRING_LEN; i++)
    rtds_name[idx] = 0;
  remote_tds_set_complete(idx, 0, rtds_name);
}
/// funkce pro nastaveni odebirani topicu vzdalenych cidel
/*
  idx - index nazvu topicu, ktery si chci subscribnout/unsubscribnout
*/


void remote_tds_subscibe_topic(esp_mqtt_client_handle_t mqtt_client, uint8_t idx)
{
  char tmp1[64];
  char tmp2[64];
  uint8_t active = 0;
  remote_tds_get_complete(idx, &active, tmp1);
  if (active == 1)
  {
    strcpy_P(tmp2, new_text_slash_rtds_slash); /// /rtds/
    strcat(tmp2, tmp1);
    strcat(tmp2, "/#");
    printf("subscribe topic %s \n", tmp2);
    esp_mqtt_client_subscribe(mqtt_client, tmp2, 0);

  }
}
///
/// funkce pro zruseni odebirani topicu vzdalenych cidel
void remote_tds_unsubscibe_topic(esp_mqtt_client_handle_t mqtt_client, uint8_t idx)
{
  char tmp1[64];
  char tmp2[64];
  uint8_t active = 0;
  remote_tds_get_complete(idx, &active, tmp1);
  if (active == 1)
  {
    strcpy_P(tmp2, new_text_slash_rtds_slash);
    strcat(tmp2, tmp1);
    strcat(tmp2, "/#");
    //mqtt_client.unsubscribe(tmp2);
    esp_mqtt_client_unsubscribe(mqtt_client, tmp2);
  }
}


/// vrati prvni idx banky ktera neni aktivni
uint8_t remote_tds_find_free(void)
{
  uint8_t ret = 255;
  uint8_t active;
  for (uint8_t idx = 0; idx < MAX_RTDS; idx++)
  {
    remote_tds_get_active(idx, &active);
    if (active == 0)
    {
      ret = idx;
      break;
    }
  }
  return ret;
}
/// hleda, zda jiz odebirame topic s timto nazvem
/*
   navratove hodnoty
   255 ... nenalezeno
   1..MAX_RTDS je idx bunky, kde mame tento nazev ulozen
*/
uint8_t remote_tds_name_exist(char *name)
{
  uint8_t active;
  uint8_t found = 255;
  char tmp_name[RTDS_DEVICE_STRING_LEN];
  for (uint8_t idx = 0; idx < MAX_RTDS; idx++)
  {
    remote_tds_get_complete(idx, &active, tmp_name);
    if (strcmp(name, tmp_name) == 0)
      found = idx;
  }
  return found;
}
///
void remote_tds_set_type(uint8_t idx, uint8_t type)
{
  if (idx < MAX_RTDS)
  {
    EEPROM.write(ram_remote_tds_store_type + (ram_remote_tds_store_size * idx), type);
  }
}


uint8_t remote_tds_get_type(uint8_t idx)
{
  uint8_t type = 255;
  if (idx < MAX_RTDS)
  {
    type = EEPROM.read(ram_remote_tds_store_type + (ram_remote_tds_store_size * idx));
  }
  return type;
}

void remote_tds_set_data(uint8_t idx, int value)
{
  if (idx < MAX_RTDS)
  {
    EEPROM.write(ram_remote_tds_store_data_low + (ram_remote_tds_store_size * idx), (value & 0xff));
    EEPROM.write(ram_remote_tds_store_data_high + (ram_remote_tds_store_size * idx), ((value >> 8) & 0xff));
    EEPROM.write(ram_remote_tds_store_last_update + (ram_remote_tds_store_size * idx), 0);
  }
}
int remote_tds_get_data(uint8_t idx)
{
  int value = 0;
  if (idx < MAX_RTDS)
  {
    value = EEPROM.read(ram_remote_tds_store_data_high + (ram_remote_tds_store_size * idx)) << 8;
    value = value + EEPROM.read(ram_remote_tds_store_data_low + (ram_remote_tds_store_size * idx));
  }
  return value;
}
uint8_t remote_tds_get_last_update(uint8_t idx)
{
  uint8_t last = 255;
  if (idx < MAX_RTDS)
  {
    last = EEPROM.read(ram_remote_tds_store_last_update + (ram_remote_tds_store_size * idx));
  }
  return last;
}

void remote_tds_set_last_update(uint8_t idx, uint8_t value)
{
  if (idx < MAX_RTDS)
  {
    EEPROM.write(ram_remote_tds_store_last_update + (ram_remote_tds_store_size * idx), value);
  }
}


void remote_tds_inc_last_update(uint8_t idx)
{
  uint8_t last = 255;
  if (idx < MAX_RTDS)
  {
    last = EEPROM.read(ram_remote_tds_store_last_update + (ram_remote_tds_store_size * idx));
    if (last < 250)
    {
      last++;
      EEPROM.write(ram_remote_tds_store_last_update + (ram_remote_tds_store_size * idx), last);
    }
  }
}

void remote_tds_update_last_update(void)
{
  uint8_t active;
  for (uint8_t idx = 0; idx < MAX_RTDS; idx++)
  {
    remote_tds_get_active(idx, &active);
    if (active == 1)
    {
      remote_tds_inc_last_update(idx);
    }
  }
}


uint8_t count_use_rtds(uint8_t *count_type_temp)
{
  uint8_t cnt = 0;
  uint8_t active = 0;
  uint8_t cc = 0;
  for (uint8_t idx = 0; idx < MAX_RTDS; idx++)
  {
    remote_tds_get_active(idx, &active);
    if (active == 1)
    {
      cnt++;
      if (remote_tds_get_type(idx) == RTDS_REMOTE_TYPE_TEMP)
      {
        cc = *count_type_temp;
        cc++;
        *count_type_temp = cc;
      }
    }
  }
  return cnt;
}

