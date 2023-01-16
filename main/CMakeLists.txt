set(srcs "main.cpp" "SSD1306.cpp" "saric_ds2482.cpp" "ow.cpp" "saric_tds_function.cpp" "saric_utils_esp32.cpp" "EEPROM.cpp" "esp32_saric_mqtt_network.cpp" "stdlib_noniso.c" "WString.cpp"
	"saric_rtds_function.cpp" "saric_virtual_outputs.cpp" "saric_metrics.cpp" "menu.cpp" "ezButton.cpp" "esp32_saric_tds_function.cpp" 
	"menu-outputs.cpp" "esp32_saric_virtual-outputs.cpp")

idf_component_register(SRCS "${srcs}"
                       INCLUDE_DIRS ".")
