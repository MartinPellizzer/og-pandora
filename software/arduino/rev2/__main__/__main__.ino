// LIBRARIES -------------------------------
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27, 16, 2);

#include <SPI.h>
#include <SD.h>
File file;

#include <Wire.h>
#include "RTClib.h"
RTC_DS3231 rtc_ds3231;
DateTime now;



// GLOBALS ---------------------------------
#define RELAY_FAN 22
#define RELAY_GEN 23
#define DELAY 10000



// REFACTOR ---------------------------------
unsigned long current_millis = 0;

int8_t gen_state = 1;

int32_t counter = 10;
int8_t countdown_update = 0;



// STRUCTS ---------------------------------
typedef struct sd_card_t
{
  int8_t state;
  int8_t tried_to_initialize;
  int8_t pin;
  int8_t pin_cd;
  int8_t is_inserted;
  int8_t is_inserted_prev;
  int8_t write_state;
  int16_t write_val;
} sd_card_t;
sd_card_t sd_card = {};

typedef struct rtc_t
{
  int16_t year_curr;
  int16_t year_prev;
  int16_t year_tmp;
  int8_t month_curr;
  int8_t month_prev;
  int8_t month_tmp;
  int8_t day_curr;
  int8_t day_prev;
  int8_t day_tmp;
  int8_t hour_curr;
  int8_t hour_prev;
  int8_t hour_tmp;
  int8_t minute_curr;
  int8_t minute_prev;
  int8_t minute_tmp;
  int8_t second_curr;
  int8_t second_prev;
  int8_t second_tmp;
} rtc_t;
rtc_t rtc = {};

typedef struct sensor_t
{
  int16_t ppb_curr;
  int16_t ppb_prev;
} sensor_t;
sensor_t sensor1;

typedef struct pot_t
{
  int8_t val = 1;
} pot_t;
pot_t pot;

typedef struct core_t
{
  int8_t lcd_minute_update;
  int8_t sd_minute_update;
} core_t;
core_t core = {};



uint8_t sensor_buff[9] = {};
uint8_t sensor_buff_index = 0;
uint32_t sensor_buff_timer = 0;
int8_t sensor_buffer_new_data = 0;



// ----------------------------------------------------------------------------------------------------------
// ;SD
// ----------------------------------------------------------------------------------------------------------
void sd_manager()
{
  sd_card_init();
  sd_card_wtite();
}
void sd_card_init()
{
  sd_card.is_inserted = (digitalRead(sd_card.pin_cd) == LOW) ? 1 : 0;
  if (sd_card.is_inserted_prev != sd_card.is_inserted)
  {
    sd_card.is_inserted_prev = sd_card.is_inserted;
    if (sd_card.is_inserted)
    {
      if (!sd_card.tried_to_initialize)
      {
        sd_card.tried_to_initialize = 1;
        sd_card.state = (SD.begin(sd_card.pin)) ? 1 : 0;
        lcd.setCursor(15, 1);
        lcd.print("*");
      }
    }
    else
    {
      sd_card.tried_to_initialize = 0;
      sd_card.state = 0;

      lcd.setCursor(15, 1);
      lcd.print(" ");
    }
  }
}
void sd_card_wtite()
{
  if (core.sd_minute_update)
  {
    core.sd_minute_update = 0;
    if (sd_card.state == 1)
    {
      file = SD.open("history.csv", FILE_WRITE);
      file.print(String(rtc.year_curr));
      file.write(',');
      file.print(String(rtc.month_curr));
      file.write(',');
      file.print(String(rtc.day_curr));
      file.write(',');
      file.print(String(rtc.hour_curr));
      file.write(',');
      file.print(String(rtc.minute_curr));
      file.write(',');
      file.print(String(rtc.second_curr));
      file.write(',');
      file.print(String(sensor1.ppb_curr));
      file.write(',');
      file.write('\n');
      file.close();
    }
  }
}



// ***********************************************************************************************
// ;RTC
// ***********************************************************************************************
void rtc_manager()
{
  now = rtc_ds3231.now();
  rtc.year_curr = now.year();
  rtc.month_curr = now.month();
  rtc.day_curr = now.day();
  rtc.hour_curr = now.hour();
  rtc.minute_curr = now.minute();
  rtc.second_curr = now.second();

  lcd_print_rtc(0);

  rtc_next_minute();
}

void rtc_next_minute()
{
  if (rtc.minute_prev != rtc.minute_curr)
  {
    rtc.minute_prev = rtc.minute_curr;
    core.lcd_minute_update = 1;
    core.sd_minute_update = 1;
  }
}



// LCD -------------------------------------
void lcd_manager()
{
  lcd_print_sensor();
}

void lcd_print_sensor()
{
  int val = sensor1.ppb_curr;
  if (val > 9999) val = 9999;
  int d1 = val % 10000 / 1000;
  int d2 = val % 1000 / 100;
  int d3 = val % 100 / 10;
  if (sensor1.ppb_prev != sensor1.ppb_curr)
  {
    sensor1.ppb_prev = sensor1.ppb_curr;
    lcd.setCursor(4, 0);
    lcd.print("S" + String(d1) + "." + String(d2) + String(d3) + " ");
  }
}

void lcd_print_rtc(int8_t refresh)
{
  if (refresh ||
      rtc.month_prev != rtc.month_curr)
  {
    rtc.month_prev = rtc.month_curr;
    lcd.setCursor(0, 1);
    if (rtc.month_curr < 10) lcd.print(0);
    lcd.print(rtc.month_curr);
  }
  if (refresh) lcd.print("/");
  if (refresh ||
      rtc.day_prev != rtc.day_curr)
  {
    rtc.day_prev = rtc.day_curr;
    lcd.setCursor(3, 1);
    if (rtc.day_curr < 10) lcd.print(0);
    lcd.print(rtc.day_curr);
  }

  if (refresh || rtc.hour_prev != rtc.hour_curr)
  {
    rtc.hour_prev = rtc.hour_curr;
    lcd.setCursor(6, 1);
    if (rtc.hour_curr < 10) lcd.print(0);
    lcd.print(rtc.hour_curr);
  }
  if (refresh) lcd.print(":");
  if (refresh || core.lcd_minute_update)
  {
    lcd.setCursor(9, 1);
    if (rtc.minute_curr < 10) lcd.print(0);
    lcd.print(rtc.minute_curr);
  }
  if (refresh) lcd.print(":");
  if (refresh || rtc.second_prev != rtc.second_curr)
  {
    rtc.second_prev = rtc.second_curr;
    lcd.setCursor(12, 1);
    if (rtc.second_curr < 10) lcd.print(0);
    lcd.print(rtc.second_curr);
  }
}

void lcd_init()
{
  lcd.init();
  lcd.backlight();
}
void lcd_write()
{
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Hello");
}



// ----------------------------------------------------------------------------------------
// SENSOR ---------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------
void sensor_manager()
{
  sensor1_read();
}

unsigned char get_checksum(unsigned char *i, unsigned char ln)
{
  unsigned char j, tempq = 0;
  i += 1;
  for (j = 0; j < (ln - 2); j++)
  {
    tempq += *i;
    i++;
  }
  tempq = (~tempq) + 1;
  return (tempq);
}

void sensor1_read()
{
  if (Serial1.available() > 0)
  {
    sensor_buffer_new_data = 1;
    uint8_t temp_val = Serial1.read();
    if (sensor_buff_index < 9)
    {
      sensor_buff[sensor_buff_index] = temp_val;
      sensor_buff_index++;
      sensor_buff_timer = millis();
    }
  }

  if (millis() - sensor_buff_timer > 40)
  {
    if (sensor_buffer_new_data)
    {
      sensor_buffer_new_data = 0;

      if (get_checksum(sensor_buff, 9) == sensor_buff[8])
      {
        int tmp = sensor_buff[4] * 256 + sensor_buff[5];
        if (tmp >= 0 && tmp <= 10000)
        {
          sensor1.ppb_curr = tmp;
          sensor1.ppb_prev = -1;
          lcd_print_sensor();
        }
      }

      for (int i = 0; i < 9; i++)
      {
        sensor_buff[i] = 0;
      }
      sensor_buff_index = 0;
    }
  }
}

void pot_manager()
{
  pot_read();
  lcd.setCursor(0, 0);
  lcd.print("P");
  if (pot.val < 10) lcd.print("0");
  lcd.print(pot.val);
}
void pot_read()
{
  int tmp = analogRead(A0);
  pot.val = map(tmp, 0, 1023, 1, 10);
}



// ----------------------------------------------------------------------------------------
// GEN ------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------
void gen_manager()
{ 
  if (counter <= 0)
  {
    if (gen_state)
    {
      gen_state = 0;
      counter = 3600 - pot.val * 10;
      digitalWrite(RELAY_GEN, LOW);
    }
    else
    {
      gen_state = 1;
      counter = pot.val * 10;
      digitalWrite(RELAY_GEN, HIGH);
    }
  }
}

// EXE -------------------------------------
void setup()
{
  pinMode(24, OUTPUT);
  
  pinMode(RELAY_FAN, OUTPUT);
  digitalWrite(RELAY_FAN, HIGH);

  pinMode(RELAY_GEN, OUTPUT);
  Serial1.begin(9600);



  lcd_init();
  //  lcd_write();

  // ************************
  // ;rtc init
  // ************************
  if (!rtc_ds3231.begin()) {}
  else {}
  if (rtc_ds3231.lostPower()) rtc_ds3231.adjust(DateTime(F(__DATE__), F(__TIME__)));
  //rtc_ds3231.adjust(DateTime(F(__DATE__), F(__TIME__)));
  //rtc_ds3231.adjust(DateTime(2014, 1, 21, 8, 59, 50));
  now = rtc_ds3231.now();
  rtc.year_curr = rtc.year_prev = now.year();
  rtc.month_curr = rtc.month_prev = now.month();
  rtc.day_curr = rtc.day_prev = now.day();
  rtc.hour_curr = rtc.hour_prev = now.hour();
  rtc.minute_curr = rtc.minute_prev = now.minute();
  rtc.second_curr = rtc.second_prev = now.second();
  lcd_print_rtc(1);

  // *************************
  // ;sd init
  // *************************
  sd_card.pin = 4;
  sd_card.pin_cd = 3;

  pot_read();

  // gen init
  gen_state = 1;
  counter = pot.val * 10;
  digitalWrite(RELAY_GEN, HIGH);
}

void loop()
{
  rtc_manager();
  pot_manager();
  gen_manager();
  sd_manager();
  sensor_manager();

  if (millis() - current_millis >= 1000)
  {
    current_millis = millis();
    countdown_update = 1;
  }

  if (countdown_update)
  {
    countdown_update = 0;

    counter--;

    int countdown_minutes = counter / 60;
    int countdown_seconds = counter % 60;

    lcd.setCursor(10, 0);
    if (gen_state) lcd.print("W");
    else lcd.print("R");
    
    if (countdown_minutes < 10) lcd.print(0);
    lcd.print(countdown_minutes);
    lcd.print(":");
    if (countdown_seconds < 10) lcd.print(0);
    lcd.print(countdown_seconds);
  }
}
