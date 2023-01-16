#include "menu.h"
#include "SSD1306.h"
#include "Font5x8.h"
#include "saric_virtual_outputs.h"


uint8_t display_menu_outputs_detail(uint8_t button_up, uint8_t button_down, uint8_t button_enter)
{
    uint8_t klik = false;
    uint8_t skip = false;
    struct_output output;
    uint8_t id = menu_params_2[INDEX_MENU_SET_OUTPUTS_DETAIL];
    uint8_t pos = 0;
    char str1[64];
    char str2[32];
    output_get_all(id, &output);
    uint8_t map_type[3] = {OUTPUT_TYPE_NONE, OUTPUT_TYPE_RS485, OUTPUT_TYPE_HW};
    uint8_t map_mode[5] = {OUTPUT_REAL_MODE_NONE, OUTPUT_REAL_MODE_STATE, OUTPUT_REAL_MODE_PWM, OUTPUT_REAL_MODE_DELAY, OUTPUT_REAL_MODE_TRISTATE}; 

    if (menu_params_3[INDEX_MENU_SET_OUTPUTS_DETAIL] == MENU_SET_OUTPUTS_HW_SHOW)
        {
        if (button_up == BUTTON_PRESSED)
            {
            klik = true;
            menu_params_1[menu_index]++;
            if (menu_params_1[menu_index] > 11)
              menu_params_1[menu_index] = 0;
            }

        if (button_down == BUTTON_PRESSED)
            {
            klik = true;
            if (menu_params_1[menu_index] == 0)
                menu_params_1[menu_index] = 11;
            menu_params_1[menu_index]--;
            }

        if (button_enter == BUTTON_PRESSED)
            {
            /// tlacitko zpet
            if (menu_params_1[menu_index] == 0)
                {
                menu_index = INDEX_MENU_SET_OUTPUTS;
                button_enter = BUTTON_NONE;
                skip = true;
                goto display_menu_outputs_detail_jump_1;
                }
            /// nastaveni nazvu
	    if (menu_params_1[menu_index] == 9)
		{
		menu_index = INDEX_MENU_SET_OUTPUT_NAME;
                menu_params_4[menu_index] = id;
		}
            /// nastaveni aktivni/neaktivni
            if (menu_params_1[menu_index] == 1)
                {
                if (output.used == 1)
            	    {
            	    output.used = 0;
            	    output_set_all(id, output);
            	    ESP_LOGI(TAG, "Deaktivuji vystup %d", id);
                    goto display_menu_outputs_detail_jump_1;
            	    }
                if (output.used == 0)
            	    {
            	    output.used = 1;
            	    output_set_all(id, output);
            	    ESP_LOGI(TAG, "Aktivuji vystup %d", id);
            	    goto display_menu_outputs_detail_jump_1;
            	    }
                }
            /// nastaveni hw vystupu
            if (menu_params_1[menu_index] == 2)
                {
                menu_params_3[menu_index] = MENU_SET_OUTPUTS_HW_SETTING;
                menu_params_4[menu_index] = 0;
		button_enter = BUTTON_NONE;
                }
	    /// nastaveni typu vystupu
	    if (menu_params_1[menu_index] == 4)
		{
		uint8_t found = false;
	        for (uint8_t idx = 0; idx < OUTPUT_TYPE_COUNT; idx++)
		    {
		    if (output.type == map_type[idx])
		        {
			found = true;
			if (idx < (OUTPUT_TYPE_COUNT-1))
			    {
			    output.type = map_type[idx+1];
			    }
			else
			    {
			    output.type = map_type[0];
			    }
			break;
			}
		    }
		if (found == false)
			output.type = OUTPUT_TYPE_NONE;
	        ESP_LOGI(TAG, "Nastavuji typ vystupu %d -> %d", id, output.type);
		output_set_all(id, output);
		}
            /// nastaveni vychozi mod po startu
            if (menu_params_1[menu_index] == 7)
                {
                uint8_t found = false;
                for (uint8_t idx = 0; idx < OUTPUT_REAL_MODE_COUNT; idx++)
                    {
                    if (output.after_start_mode == map_mode[idx])
                        {
                        found = true;
                        if (idx < (OUTPUT_REAL_MODE_COUNT-1))
                            {
                            output.after_start_mode = map_mode[idx+1];
                            }
                        else
                            {
                            output.after_start_mode = map_mode[0];
                            }
                        break;
                        }
                    }
                if (found == false)
                        output.after_start_mode = OUTPUT_REAL_MODE_NONE;
                ESP_LOGI(TAG, "Nastavuji vychozi mod po startu %d -> %d", id, output.after_start_mode);
                output_set_all(id, output);
                }
	    /// nastaveni globalni ID
	    if (menu_params_1[menu_index] == 5)
		{
		menu_params_3[menu_index] = MENU_SET_OUTPUTS_HW_SETTING;
                menu_params_4[menu_index] = 0;
                button_enter = BUTTON_NONE;
		}
	    /// nastaveni periody
            if (menu_params_1[menu_index] == 6)
                {
                menu_params_3[menu_index] = MENU_SET_OUTPUTS_HW_SETTING;
                menu_params_4[menu_index] = 0;
                button_enter = BUTTON_NONE;
                }
	    /// nastaveni mod po startu
	    if (menu_params_1[menu_index] == 8)
                {
                menu_params_3[menu_index] = MENU_SET_OUTPUTS_HW_SETTING;
                menu_params_4[menu_index] = 0;
                button_enter = BUTTON_NONE;
                }
	    /// nastaveni max time state
	    if (menu_params_1[menu_index] == 10)
                {
                menu_params_3[menu_index] = MENU_SET_OUTPUTS_HW_SETTING;
                menu_params_4[menu_index] = 0;
                button_enter = BUTTON_NONE;
                }
	    /// sync eeprom
	    if (menu_params_1[menu_index] == 11)
                {
	        output_sync_to_eeprom_idx(id);
		}
	    }
    }
    /// jsem v editacnim modu max time state
    if (menu_params_3[menu_index] == MENU_SET_OUTPUTS_HW_SETTING && menu_params_1[menu_index] == 10)
        {
        if (button_enter == BUTTON_PRESSED)
            {
            /// klik zpet
            klik = true;
            menu_params_3[INDEX_MENU_SET_OUTPUTS_DETAIL] = MENU_SET_OUTPUTS_HW_SHOW;
            }
        if (button_up == BUTTON_PRESSED)
            {
            klik = true;
            output.state_time_max++;
            ESP_LOGI(TAG, "Nastavuji max time state %d %d", id, output.state_time_max);
            output_set_all(id, output);
            }
        if (button_down == BUTTON_PRESSED)
            {
            klik = true;
            output.state_time_max--;
            ESP_LOGI(TAG, "Nastavuji max time state %d %d", id, output.state_time_max);
            output_set_all(id, output);
            }
        }
    /// jsem v editacnim modu stav po startu
    if (menu_params_3[menu_index] == MENU_SET_OUTPUTS_HW_SETTING && menu_params_1[menu_index] == 8)
        {
        if (button_enter == BUTTON_PRESSED)
            {
            /// klik zpet
            klik = true;
            menu_params_3[INDEX_MENU_SET_OUTPUTS_DETAIL] = MENU_SET_OUTPUTS_HW_SHOW;
            }
        if (button_up == BUTTON_PRESSED)
            {
            klik = true;
            output.after_start_state++;
            ESP_LOGI(TAG, "Nastavuji stav po startu %d %d", id, output.after_start_state);
            output_set_all(id, output);
            }
        if (button_down == BUTTON_PRESSED)
            {
            klik = true;
            output.after_start_state--;
            ESP_LOGI(TAG, "Nastavuji stav po startu %d %d", id, output.after_start_state);
            output_set_all(id, output);
            }
        }
    /// jsem v editacnim modu periody vystutp
    if (menu_params_3[menu_index] == MENU_SET_OUTPUTS_HW_SETTING && menu_params_1[menu_index] == 6)
        {
        if (button_enter == BUTTON_PRESSED)
            {
            /// klik zpet
            klik = true;
            menu_params_3[INDEX_MENU_SET_OUTPUTS_DETAIL] = MENU_SET_OUTPUTS_HW_SHOW;
            }
        if (button_up == BUTTON_PRESSED)
            {
            klik = true;
            output.period++;
            ESP_LOGI(TAG, "Nastavuji periodu vystupu %d %d", id, output.period);
            output_set_all(id, output);
            }
        if (button_down == BUTTON_PRESSED)
            {
            klik = true;
            output.period--;
            ESP_LOGI(TAG, "Nastavuji periodu vystupu %d %d", id, output.period);
            output_set_all(id, output);
	    }
	}
    /// jsem v editacnim modu globalniho ID
    if (menu_params_3[menu_index] == MENU_SET_OUTPUTS_HW_SETTING && menu_params_1[menu_index] == 5)
	{
        if (button_enter == BUTTON_PRESSED)
            {
            /// klik zpet
            klik = true;
            menu_params_3[INDEX_MENU_SET_OUTPUTS_DETAIL] = MENU_SET_OUTPUTS_HW_SHOW;
	    }
	if (button_up == BUTTON_PRESSED)
            {
            klik = true;
	    output.id++;
	    ESP_LOGI(TAG, "Nastavuji id vystupu %d %d", id, output.id);
	    output_set_all(id, output);
	    }

	if (button_down == BUTTON_PRESSED)
            {
            klik = true;
	    output.id--;
	    ESP_LOGI(TAG, "Nastavuji id vystupu %d %d", id, output.id);
            output_set_all(id, output);
	    }
	}
    /// jsem v editacnim modu vystupu
    if (menu_params_3[menu_index] == MENU_SET_OUTPUTS_HW_SETTING && menu_params_1[menu_index] == 2)
        {
        if (button_up == BUTTON_PRESSED)
            {
            klik = true;
            menu_params_4[menu_index]++;
            if (menu_params_4[menu_index] > 8)
              menu_params_4[menu_index] = 0;
            }
        if (button_down == BUTTON_PRESSED)
            {
            klik = true;
            if (menu_params_4[menu_index] == 0)
                menu_params_4[menu_index] = 9;
            menu_params_4[menu_index]--;
            }
        if (button_enter == BUTTON_PRESSED)
            {
            /// tlacitko zpet
	    klik = true;
            if (menu_params_4[menu_index] == 0)
	       menu_params_3[INDEX_MENU_SET_OUTPUTS_DETAIL] = MENU_SET_OUTPUTS_HW_SHOW;
            
	    if (menu_params_4[menu_index] > 0 && menu_params_4[menu_index] < 9)
	        {
		pos = menu_params_4[menu_index]-1;
                if ((output.outputs & (1<<pos)) != 0)
		    {
		    cbi(output.outputs, pos);
		    ESP_LOGI(TAG, "Odstranuji vystup %d - %d", id, pos);
		    }
		else
		    {
		    sbi(output.outputs, pos);
		    ESP_LOGI(TAG, "Nastavuji vystup %d - %d", id, pos);
		    }
		output_set_all(id, output);
	        }
            }
	}


display_menu_outputs_detail_jump_1:
    if (skip == false)
        {
        GLCD_Clear();
        GLCD_GotoXY(0, 0);
        sprintf(str1, "Vystup %s", output.name);
        GLCD_PrintString(str1);
	/// zpet do menu
        if (menu_params_1[menu_index] == 0)
            {
            GLCD_GotoXY(10, 16);
            GLCD_PrintString("Zpet");
            }
	/// nastaveni aktivni/neaktivni
        if (menu_params_1[menu_index] == 1)
            {
            GLCD_GotoXY(10, 16);
	    if (output.used == 1)
                strcpy(str1, "Stav: Aktivni");
	    else
		strcpy(str1, "Stav: Neaktivni");
	    GLCD_PrintString(str1);
            }
	/// nastaveni HW vystupu
        if (menu_params_1[menu_index] == 2)
            {
            GLCD_GotoXY(10, 10);
            GLCD_PrintString("HW vystupy");

	    if (menu_params_3[menu_index] == MENU_SET_OUTPUTS_HW_SETTING && menu_params_1[menu_index] == 2)
	        {
	        GLCD_GotoXY(4, 21);
                GLCD_PrintString("<-");
                
	        if (menu_params_4[menu_index] == 0)
                    GLCD_DrawRectangle(0, 19, 18, 29, GLCD_Black);

	        if (menu_params_4[menu_index] > 0 && menu_params_4[menu_index] < 9)
	            {
	            pos = menu_params_4[menu_index] - 1;
	            GLCD_DrawRectangle(18+(pos*12), 19, 28+(pos*12), 29, GLCD_Black);
                    }
	        }
	    else
		{
		GLCD_GotoXY(4, 20);
	        GLCD_PrintString("->");
                }
            for (uint8_t idx = 0; idx < 8; idx++)
	        if ((output.outputs & (1<<idx)) != 0)
	           GLCD_FillRectangle(20+(idx*12), 21, 26+(idx*12), 27, GLCD_Black);
	        else
		   GLCD_DrawRectangle(20+(idx*12), 21, 26+(idx*12), 27, GLCD_Black);
            }
	/// nastaveni povolenych modu
        if (menu_params_1[menu_index] == 3)
            {
            GLCD_GotoXY(10, 10);
            GLCD_PrintString("Povolene mody");
	    GLCD_GotoXY(0, 20);
	    strcpy(str1, "");
	    for (uint8_t idx = 0; idx < OUTPUT_REAL_MODE_COUNT; idx++)
		if ((output.mode_enable & (1<<idx)) != 0)
		{
		switch (idx)
		    {
		    case OUTPUT_REAL_MODE_NONE:
                        strcat(str1, "NO|");
			break;
		    case OUTPUT_REAL_MODE_STATE:
			strcat(str1, "ST|");
                        break;
		    case OUTPUT_REAL_MODE_PWM:
		        strcat(str1, "PWM|");
                        break;
		    case OUTPUT_REAL_MODE_DELAY:
                        strcat(str1, "DL|");
                        break;
		    case OUTPUT_REAL_MODE_TRISTATE:
			strcat(str1, "3S|");
                        break;
		    default:
			strcpy(str1, "ERR");
			break;
		    }
		}
	    if (strlen(str1) == 0)
		    strcpy(str1,"NONE-ERR");
	    GLCD_PrintString(str1);
            }
	/// nastaveni typu vystupu
        if (menu_params_1[menu_index] == 4)
            {
            GLCD_GotoXY(10, 16);
	    switch (output.type)
	        {
		case OUTPUT_TYPE_NONE:
                    strcpy(str2, "None");
                    break;
                case OUTPUT_TYPE_RS485:
		    strcpy(str2, "RS485");
	            break;
                case OUTPUT_TYPE_HW:
		    strcpy(str2, "HW outputs");
		    break;
		default:
		    strcpy(str2, "unknow");
		    break;
	        }
            sprintf(str1,"Typ: %s", str2);
	    GLCD_PrintString(str1);
            }
	/// nastaveni globalniho ID
        if (menu_params_1[menu_index] == 5)
            {
            GLCD_GotoXY(0, 16);
	    sprintf(str1, "Globalni ID: %d", output.id);
            GLCD_PrintString(str1);
	    if (menu_params_3[menu_index] == MENU_SET_OUTPUTS_HW_SETTING)
                GLCD_DrawRectangle(75, 14, 96, 24, GLCD_Black);
            }
	/// nastaveni casovaci periody
        if (menu_params_1[menu_index] == 6)
            {
            GLCD_GotoXY(0, 16);
            sprintf(str1, "Casovaci perioda: %d", output.period);
            GLCD_PrintString(str1);
            if (menu_params_3[menu_index] == MENU_SET_OUTPUTS_HW_SETTING)
                GLCD_DrawRectangle(105, 14, 126, 24, GLCD_Black);
            }
	/// nastaveni modu po startu
        if (menu_params_1[menu_index] == 7)
            {
            GLCD_GotoXY(10, 10);
            GLCD_PrintString("Mod po startu:");
	    switch (output.after_start_mode)
	        {
                case OUTPUT_REAL_MODE_NONE:
	            strcpy(str1, "None");
		    break;
		case OUTPUT_REAL_MODE_STATE:
		    strcpy(str1, "on/off");
		    break;
		case OUTPUT_REAL_MODE_PWM:
		    strcpy(str1, "stmivac");
		    break;
		case OUTPUT_REAL_MODE_DELAY:
		    strcpy(str1, "casovac");
		    break;
		case OUTPUT_REAL_MODE_TRISTATE:
		    strcpy(str1, "3 stav");
		    break;
		default:
		    strcpy(str1, "unknow");
		    break;
		}
	    GLCD_GotoXY(20, 20);
            GLCD_PrintString(str1);
            }
	/// nastaveni stavu po startu
        if (menu_params_1[menu_index] == 8)
            {
            GLCD_GotoXY(0, 16);
            sprintf(str1, "Stav po startu %d", output.after_start_state);
            GLCD_PrintString(str1);
	    if (menu_params_3[menu_index] == MENU_SET_OUTPUTS_HW_SETTING)
                GLCD_DrawRectangle(88, 14, 110, 24, GLCD_Black);
            }
	/// nastaveni nazvu
        if (menu_params_1[menu_index] == 9)
            {
            GLCD_GotoXY(10, 16);
            GLCD_PrintString("Nastavit nazev");
            }

	/// nastaveni maximalni cas
        if (menu_params_1[menu_index] == 10)
            {
            GLCD_GotoXY(0, 16);
            sprintf(str1, "Max time state %d", output.state_time_max);
            GLCD_PrintString(str1);
            if (menu_params_3[menu_index] == MENU_SET_OUTPUTS_HW_SETTING)
                GLCD_DrawRectangle(88, 14, 110, 24, GLCD_Black);
            }
	/// sync eeprom
        if (menu_params_1[menu_index] == 11)
            {
	    GLCD_GotoXY(10, 16);
            GLCD_PrintString("Ulozit do EEPROM");
	    }
        GLCD_Render();
      }
    return klik;
}

uint8_t display_menu_outputs(uint8_t button_up, uint8_t button_down, uint8_t button_enter)
{
  uint8_t klik = false;
  uint8_t skip = false;
  struct_output output;
  uint8_t id;
  char str1[32];
  char str2[32];
  if (button_up == BUTTON_PRESSED)
    {
    klik = true;
    menu_params_1[menu_index]++;
    if (menu_params_1[menu_index] > (MAX_OUTPUT))
    	  menu_params_1[menu_index] = 0;
    }

  if (button_down == BUTTON_PRESSED)
    {
    klik = true;
    if (menu_params_1[menu_index] == 0)
    	  menu_params_1[menu_index] = (MAX_OUTPUT+1);
    menu_params_1[menu_index]--;
    }

  if (button_enter == BUTTON_PRESSED)
    {
    /// tlacitko zpet
    if (menu_params_1[menu_index] == 0)
      {
      menu_index = INDEX_MENU_DEFAULT;
      button_enter = BUTTON_NONE;
      skip = true;
      goto display_menu_outputs_jump_1;
      }
    if (menu_params_1[menu_index] > 0 && menu_params_1[menu_index] < MAX_OUTPUT+1)
      {
      id = menu_params_1[menu_index] - 1;
      menu_index = INDEX_MENU_SET_OUTPUTS_DETAIL;
      menu_params_2[menu_index] = id;
      menu_params_3[menu_index] = MENU_SET_OUTPUTS_HW_SHOW;
      menu_params_4[menu_index] = 0;
      skip = true;
      goto display_menu_outputs_jump_1; 
      }
    }
    
display_menu_outputs_jump_1:
  if (skip == false)
    {
    GLCD_Clear();
    GLCD_GotoXY(0, 0);
    GLCD_PrintString("Regulator vystupy");
    if (menu_params_1[menu_index] == 0)
      {
      GLCD_GotoXY(0, 16);
      GLCD_PrintString("Zpet");
      }

    if (menu_params_1[menu_index] > 0)
        {
	id = menu_params_1[menu_index] - 1;
	output_get_all(id, &output);
	sprintf(str2,"nazev: %s" ,output.name);
	if (output.used == 1)
            sprintf(str1, "%d - nastaven", id);
	else
           sprintf(str1, "%d - neaktivni", id);
	GLCD_GotoXY(0, 23);
        GLCD_PrintString(str2);
	GLCD_GotoXY(32, 13);
        GLCD_PrintString(str1);
	} 
    GLCD_Render();
    }

  return klik;
}
