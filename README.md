# Senior-Design-Test-files
General code used to test various components are working together properly. All code is written for the ESP32 using the Arduino IDE.

Currently includes test code for:
1. ESP32 and TFT display communication  (TFT_Meters.ino)
2. ESP32, TFT display, and TFT touch communication  (On_Off_Button.ino)
3. ESP32 and MAX31855 thermocouple (serialthermocouple.ino)
4. ESP32, TFT display, TFT touch, and 1-4 MAX31855 thermocouple sensor(s) utilizing FreeRTOS  (Display_Touch_and_sensor_using_FreeRTOS.ino)

Also includes various supporting files such as:
1. Excel file for temperature fluctuation reduction formula used in the 3rd code test file  (Temperature Fluctuation reduction.xlsx)
2. Text file with all the library-defined font datums and colors (Display fonts and colors.txt)
3. Correctly defined user setup header file for the TFT display library (User_Setup.h)
