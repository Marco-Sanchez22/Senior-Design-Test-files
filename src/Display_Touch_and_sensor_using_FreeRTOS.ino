// Example of drawing a graphical "switch" and using
// the touch screen to change it's state.

// This sketch does not use the libraries button drawing
// and handling functions.

// Based on Adafruit_GFX library onoffbutton example.

// Touch handling for XPT2046 based screens is handled by
// the TFT_eSPI library.

// Calibration data is stored in SPIFFS so we need to include it
#include "FS.h"

#include <SPI.h>

#include <TFT_eSPI.h> // Hardware-specific library

TFT_eSPI tft = TFT_eSPI();       // Invoke custom library

#include "Adafruit_MAX31855.h"  // Sensor library (add to User_Setup.h)
#include <mutex>

// initialize read and write locks 
SemaphoreHandle_t serial_write_lock = xSemaphoreCreateMutex();
SemaphoreHandle_t temperature_write_read_lock = xSemaphoreCreateMutex();

// define sensor GPIO pins
#define MAXDO   13   // MISO
#define MAXCLK  14  // CLK
#define sensor1CS   12  // CS
#define sensor2CS   27  // CS
#define sensor3CS   25  // CS
#define sensor4CS   33  // CS

// define sensor indexes
#define SENSOR_1 0
#define SENSOR_2 1
#define SENSOR_3 2
#define SENSOR_4 3

// turn sensors on or off
#define ENABLE_SENSOR_1 true
#define ENABLE_SENSOR_2 true
#define ENABLE_SENSOR_3 true
#define ENABLE_SENSOR_4 true

// initialize sensor variables
double a = 0.2390572236;  // calculated via formula (see fluctuation reduction Excel file)
double b = 0.5218855528;  // calculated via formula (see fluctuation reduction Excel file)
double calc_temp[4] = { 25, 25, 25, 25 };

// initialize the Thermocouple(s)
#if ENABLE_SENSOR_1
  Adafruit_MAX31855 sensor1(MAXCLK, sensor1CS, MAXDO);
#endif

#if ENABLE_SENSOR_2
  Adafruit_MAX31855 sensor2(MAXCLK, sensor2CS, MAXDO);
#endif

#if ENABLE_SENSOR_3
  Adafruit_MAX31855 sensor3(MAXCLK, sensor3CS, MAXDO);
#endif

#if ENABLE_SENSOR_4
  Adafruit_MAX31855 sensor4(MAXCLK, sensor4CS, MAXDO);
#endif

// This is the file name used to store the touch coordinate
// calibration data. Change the name to start a new calibration.
#define CALIBRATION_FILE "/testCAL"

// Set REPEAT_CAL to true instead of false to run calibration
// again, otherwise it will only be done once.
// Repeat calibration if you change the screen rotation.
#define REPEAT_CAL false

bool SwitchOn = false;

// Comment out to stop drawing black spots
//#define BLACK_SPOT

// Switch position and size
#define FRAME_X 100
#define FRAME_Y 64
#define FRAME_W 120
#define FRAME_H 50

// Red zone size
#define REDBUTTON_X FRAME_X
#define REDBUTTON_Y FRAME_Y
#define REDBUTTON_W (FRAME_W/2)
#define REDBUTTON_H FRAME_H

// Green zone size
#define GREENBUTTON_X (REDBUTTON_X + REDBUTTON_W)
#define GREENBUTTON_Y FRAME_Y
#define GREENBUTTON_W (FRAME_W/2)
#define GREENBUTTON_H FRAME_H

// declare the functions to be used in FreeRTOS
void get_sensor_data(void *pvParameter);
void touch_display_stuff(void *pvParameter);

//------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------
void setup(void)
{
  Serial.begin(9600);
  tft.init();

  // Set the rotation before we calibrate
  tft.setRotation(1);

  // call screen calibration
  touch_calibrate();

  // clear screen
  tft.fillScreen(TFT_BLUE);

  // Draw button (this example does not use library Button class)
  redBtn();

  // EDITS below vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
  assert(serial_write_lock);
  assert(temperature_write_read_lock);

  // Thermalcouple stuff
  #if ENABLE_SENSOR_1
    Serial.println("MAX31855 test");
    // wait for MAX chip to stabilize
    delay(500);
    Serial.print("Initializing sensor 1...");
    if (!sensor1.begin()) {
      Serial.println("ERROR.");
      while (1) delay(10);
    }
    Serial.println("DONE.");
  #endif

  #if ENABLE_SENSOR_2
    Serial.println("MAX31855 test");
    // wait for MAX chip to stabilize
    delay(500);
    Serial.print("Initializing sensor 2...");
    if (!sensor2.begin()) {
      Serial.println("ERROR.");
      while (1) delay(10);
    }
    Serial.println("DONE.");
  #endif

  #if ENABLE_SENSOR_3
    Serial.println("MAX31855 test");
    // wait for MAX chip to stabilize
    delay(500);
    Serial.print("Initializing sensor 3...");
    if (!sensor3.begin()) {
      Serial.println("ERROR.");
      while (1) delay(10);
    }
    Serial.println("DONE.");
  #endif

  #if ENABLE_SENSOR_4
    Serial.println("MAX31855 test");
    // wait for MAX chip to stabilize
    delay(500);
    Serial.print("Initializing sensor 4...");
    if (!sensor4.begin()) {
      Serial.println("ERROR.");
      while (1) delay(10);
    }
    Serial.println("DONE.");
  #endif

  // declare the task for freeRTOS
  #if ENABLE_SENSOR_1 || ENABLE_SENSOR_2 || ENABLE_SENSOR_3 || ENABLE_SENSOR_4
      xTaskCreate(&get_sensor_data, // Task address
      "Read_Sensor_Data", // task name
      1000, // number of bytes to reserve for the task
      NULL, // paramters passed to the task
      1, // priority of the task
      NULL); // handle to the task
  #endif

  xTaskCreate(&touch_display_stuff, // Task address
    "Update_Display", // task name
    1500, // number of bytes to reserve for the task
    NULL, // paramters passed to the task
    1, // priority of the task
    NULL); // handle to the task
}
//------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------
void loop()
{
  // leave empty for FreeRTOS
}
//------------------------------------------------------------------------------------------

void touch_display_stuff(void *pvParameter)
{
  double trash1 = 25.0;
  double trash2 = 25.0;
  double trash3 = 25.0;
  double trash4 = 25.0;
  while(true)
  {
    uint16_t x, y;

    // See if there's any touch data
    if (tft.getTouch(&x, &y))
    {
      // Draw a block spot to show where touch was calculated to be
      #ifdef BLACK_SPOT
        tft.fillCircle(x, y, 2, TFT_BLACK);
      #endif
      xSemaphoreTake(serial_write_lock, portMAX_DELAY); // get serial write lock
      if (SwitchOn)
      {
        if ((x > REDBUTTON_X) && (x < (REDBUTTON_X + REDBUTTON_W))) {
          if ((y > REDBUTTON_Y) && (y <= (REDBUTTON_Y + REDBUTTON_H))) {
            Serial.println("Red btn hit");
            redBtn();
          }
        }
      }
      else //Record is off (SwitchOn == false)
      {
        if ((x > GREENBUTTON_X) && (x < (GREENBUTTON_X + GREENBUTTON_W))) {
          if ((y > GREENBUTTON_Y) && (y <= (GREENBUTTON_Y + GREENBUTTON_H))) {
            Serial.println("Green btn hit");
            greenBtn();
          }
        }
      }

      Serial.println(SwitchOn);
      xSemaphoreGive(serial_write_lock); // release serial write lock
    }

    // update sensors if there is any change
    if((trash1 != calc_temp[SENSOR_1]) || (trash2 != calc_temp[SENSOR_2]) || (trash3 != calc_temp[SENSOR_3]) || (trash4 != calc_temp[SENSOR_4]))
    {

      tft.setTextColor(TFT_WHITE);
      tft.setTextSize(2);
      tft.setTextDatum(ML_DATUM);

      xSemaphoreTake(temperature_write_read_lock, portMAX_DELAY);

      #if ENABLE_SENSOR_1 
        if (trash1 != calc_temp[SENSOR_1])
        {
          tft.fillRoundRect(60, 146, 100, 16, 2, TFT_BLUE);
          String display_string1 = "C = ";
          trash1 = calc_temp[SENSOR_1];
          display_string1 += calc_temp[SENSOR_1];

          tft.drawString("Sensor 1", 20, 135);
          tft.drawString(display_string1, 20, 155);
        }
      #endif

      #if ENABLE_SENSOR_2
        if (trash2 != calc_temp[SENSOR_2])
        {
          tft.fillRoundRect(60, 186, 100, 16, 2, TFT_BLUE);
          String display_string2 = "C = ";
          trash2 = calc_temp[SENSOR_2];
          display_string2 += calc_temp[SENSOR_2];

          tft.drawString("Sensor 2", 20, 175);
          tft.drawString(display_string2, 20, 195);
        }
      #endif

      #if ENABLE_SENSOR_3
        if (trash3 != calc_temp[SENSOR_3])
        {
          tft.fillRoundRect(215, 146, 100, 16, 2, TFT_BLUE);
          String display_string3 = "C = ";
          trash3 = calc_temp[SENSOR_3];
          display_string3 += calc_temp[SENSOR_3];

          tft.drawString("Sensor 3", 170, 135);
          tft.drawString(display_string3, 170, 155);
        }
        
      #endif

      #if ENABLE_SENSOR_4
        if (trash4 != calc_temp[SENSOR_4])
        {
          tft.fillRoundRect(215, 186, 100, 16, 2, TFT_BLUE);
          String display_string4 = "C = ";
          trash4 = calc_temp[SENSOR_4];
          display_string4 += calc_temp[SENSOR_4];

          tft.drawString("Sensor 4", 170, 175);
          tft.drawString(display_string4, 170, 195);
        }
        
      #endif
      
      xSemaphoreGive(temperature_write_read_lock);
    }
  }

  vTaskDelete( NULL );  // protection incase it somehow exits the loop
}

void touch_calibrate()
{
  uint16_t calData[5];
  uint8_t calDataOK = 0;

  // check file system exists
  if (!SPIFFS.begin()) {
    Serial.println("Formatting file system");
    SPIFFS.format();
    SPIFFS.begin();
  }

  // check if calibration file exists and size is correct
  if (SPIFFS.exists(CALIBRATION_FILE)) {
    if (REPEAT_CAL)
    {
      // Delete if we want to re-calibrate
      SPIFFS.remove(CALIBRATION_FILE);
    }
    else
    {
      File f = SPIFFS.open(CALIBRATION_FILE, "r");
      if (f) {
        if (f.readBytes((char *)calData, 14) == 14)
          calDataOK = 1;
        f.close();
      }
    }
  }

  if (calDataOK && !REPEAT_CAL) {
    // calibration data valid
    tft.setTouch(calData);
  } else {
    // data not valid so recalibrate
    tft.fillScreen(TFT_BLACK);
    tft.setCursor(20, 0);
    tft.setTextFont(2);
    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);

    tft.println("Touch corners as indicated");

    tft.setTextFont(1);
    tft.println();

    if (REPEAT_CAL) {
      tft.setTextColor(TFT_RED, TFT_BLACK);
      tft.println("Set REPEAT_CAL to false to stop this running again!");
    }

    tft.calibrateTouch(calData, TFT_MAGENTA, TFT_BLACK, 15);

    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.println("Calibration complete!");

    // store data
    File f = SPIFFS.open(CALIBRATION_FILE, "w");
    if (f) {
      f.write((const unsigned char *)calData, 14);
      f.close();
    }
  }
}

void drawFrame()
{
  tft.drawRect(FRAME_X, FRAME_Y, FRAME_W, FRAME_H, TFT_BLACK);
}

// Draw a red button
void redBtn()
{
  tft.fillRect(REDBUTTON_X, REDBUTTON_Y, REDBUTTON_W, REDBUTTON_H, TFT_RED);
  tft.fillRect(GREENBUTTON_X, GREENBUTTON_Y, GREENBUTTON_W, GREENBUTTON_H, TFT_DARKGREY);
  drawFrame();
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(2);
  tft.setTextDatum(MC_DATUM);
  tft.drawString("ON", GREENBUTTON_X + (GREENBUTTON_W / 2), GREENBUTTON_Y + (GREENBUTTON_H / 2));
  SwitchOn = false;
}

// Draw a green button
void greenBtn()
{
  tft.fillRect(GREENBUTTON_X, GREENBUTTON_Y, GREENBUTTON_W, GREENBUTTON_H, TFT_GREEN);
  tft.fillRect(REDBUTTON_X, REDBUTTON_Y, REDBUTTON_W, REDBUTTON_H, TFT_DARKGREY);
  drawFrame();
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(2);
  tft.setTextDatum(MC_DATUM);
  tft.drawString("OFF", REDBUTTON_X + (REDBUTTON_W / 2) + 1, REDBUTTON_Y + (REDBUTTON_H / 2));
  SwitchOn = true;
}

// Read sensor data
void get_sensor_data(void *pvParameter)
{
  double cur_temp[4] = { 25.0, 25.0, 25.0, 25.0 };
  double prev_temp[4] = { 25.0, 25.0, 25.0, 25.0 };
  while(true)
  {
    xSemaphoreTake(serial_write_lock, portMAX_DELAY); // get serial write lock
    
    Serial.println("-----------------");
    #if ENABLE_SENSOR_1
      cur_temp[SENSOR_1] = sensor1.readCelsius();
      //cur_temp[SENSOR_1] = sensor1.readInternal();    // for testing without probes

      // sensor 1 error detection
      if (isnan(cur_temp[SENSOR_1])) {
        Serial.println("sensor 1 fault(s) detected!");
        uint8_t e = sensor1.readError();
        if (e & MAX31855_FAULT_OPEN) Serial.println("FAULT: sensor 1 is open - no connections.");
        if (e & MAX31855_FAULT_SHORT_GND) Serial.println("FAULT: sensor 1 is short-circuited to GND.");
        if (e & MAX31855_FAULT_SHORT_VCC) Serial.println("FAULT: sensor 1 is short-circuited to VCC.");
      } else {
        xSemaphoreTake(temperature_write_read_lock, portMAX_DELAY);
        calculate_temp(SENSOR_1, cur_temp[SENSOR_1], prev_temp[SENSOR_1]);
        xSemaphoreGive(temperature_write_read_lock);

        Serial.print("Sensor 1\nC = ");
        Serial.println(calc_temp[SENSOR_1]);
      }
    #endif

    #if ENABLE_SENSOR_2
      cur_temp[SENSOR_2] = sensor2.readCelsius();
      //cur_temp[SENSOR_2] = sensor2.readInternal();    // for testing without probes

      // sensor 2 error detection
      if (isnan(cur_temp[SENSOR_2])) {
        Serial.println("sensor 2 fault(s) detected!");
        uint8_t e = sensor2.readError();
        if (e & MAX31855_FAULT_OPEN) Serial.println("FAULT: sensor 2 is open - no connections.");
        if (e & MAX31855_FAULT_SHORT_GND) Serial.println("FAULT: sensor 2 is short-circuited to GND.");
        if (e & MAX31855_FAULT_SHORT_VCC) Serial.println("FAULT: sensor 2 is short-circuited to VCC.");
      } else {
        xSemaphoreTake(temperature_write_read_lock, portMAX_DELAY);
        calculate_temp(SENSOR_2, cur_temp[SENSOR_2], prev_temp[SENSOR_2]);
        xSemaphoreGive(temperature_write_read_lock);

        Serial.print("Sensor 2\nC = ");
        Serial.println(calc_temp[SENSOR_2]);
      }
    #endif

    #if ENABLE_SENSOR_3
      cur_temp[SENSOR_3] = sensor3.readCelsius();
      //cur_temp[SENSOR_3] = sensor3.readInternal();    // for testing without probes

      // sensor 3 error detection
      if (isnan(cur_temp[SENSOR_3])) {
        Serial.println("sensor 3 fault(s) detected!");
        uint8_t e = sensor3.readError();
        if (e & MAX31855_FAULT_OPEN) Serial.println("FAULT: sensor 3 is open - no connections.");
        if (e & MAX31855_FAULT_SHORT_GND) Serial.println("FAULT: sensor 3 is short-circuited to GND.");
        if (e & MAX31855_FAULT_SHORT_VCC) Serial.println("FAULT: sensor 3 is short-circuited to VCC.");
      } else {
        xSemaphoreTake(temperature_write_read_lock, portMAX_DELAY);
        calculate_temp(SENSOR_3, cur_temp[SENSOR_3], prev_temp[SENSOR_3]);
        xSemaphoreGive(temperature_write_read_lock);

        Serial.print("Sensor 3\nC = ");
        Serial.println(calc_temp[SENSOR_3]);
      }
    #endif

    #if ENABLE_SENSOR_4
      cur_temp[SENSOR_4] = sensor4.readCelsius();
      //cur_temp[SENSOR_4] = sensor4.readInternal();    // for testing without probes
      
      // sensor 4 error detection
      if (isnan(cur_temp[SENSOR_4])) {
        Serial.println("sensor 4 fault(s) detected!");
        uint8_t e = sensor4.readError();
        if (e & MAX31855_FAULT_OPEN) Serial.println("FAULT: sensor 4 is open - no connections.");
        if (e & MAX31855_FAULT_SHORT_GND) Serial.println("FAULT: sensor 4 is short-circuited to GND.");
        if (e & MAX31855_FAULT_SHORT_VCC) Serial.println("FAULT: sensor 4 is short-circuited to VCC.");
      } else {
        xSemaphoreTake(temperature_write_read_lock, portMAX_DELAY);
        calculate_temp(SENSOR_4, cur_temp[SENSOR_4], prev_temp[SENSOR_4]);
        xSemaphoreGive(temperature_write_read_lock);

        Serial.print("Sensor 4\nC = ");
        Serial.println(calc_temp[SENSOR_4]);
      }
    #endif

    xSemaphoreGive(serial_write_lock); // release serial write lock
    vTaskDelay(1000 / portTICK_RATE_MS);  // read sensor data every 1 second
  }

  vTaskDelete( NULL );  // protection in case it somehow exits the loop
}

void calculate_temp( int sensor, double cur_temp, double prev_temp )
{
  calc_temp[sensor] = a * (cur_temp + prev_temp) + b * calc_temp[sensor];
  calc_temp[sensor] = round(calc_temp[sensor] * 100.0) / 100.0;
}
