#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27, 16, 2);

uint32_t current_millis = 0;

uint32_t timer_on_millis = 10000;
uint32_t timer_off_millis = 3600000;
//uint32_t timer_off_millis = 3000;

uint32_t one_second_current_millis = 0;

int is_on = 1;


#define FAN_PIN 4
#define GEN_PIN 5

// potentiometer
#define POT_PIN 34


uint8_t sensor_buff[9] = {};
uint8_t sensor_buff_index = 0;
uint32_t sensor_buff_timer = 0;
int8_t sensor_buffer_new_data = 0;

typedef struct sensor_t
{
  int16_t ppb_curr;
  int16_t ppb_prev;
} sensor_t;
sensor_t sensor1;

typedef struct lcd_t
{
  int16_t target_curr = 0;
  int16_t target_prev = -1;
} lcd_t;
lcd_t lcd1;

void lcd_manager()
{
  lcd_print_sensor();
  lcd_print_target();
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
    lcd.setCursor(0, 1);
    lcd.print("S: " + String(d1) + "." + String(d2) + String(d3));
  }
}

void lcd_print_target()
{
  int d1 = lcd1.target_curr % 100 / 10;
  int d2 = lcd1.target_curr % 10 / 1;
  if (lcd1.target_prev != lcd1.target_curr)
  {
    lcd1.target_prev = lcd1.target_curr;
    lcd.setCursor(0, 0);
    lcd.print("T: " + String(d1) + "." + String(d2) + "0");
  }
}

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
  if (Serial2.available() > 0)
  {
    sensor_buffer_new_data = 1;
    uint8_t temp_val = Serial2.read();
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

void setup()
{
  Serial.begin(9600);
  Serial2.begin(9600);

  pinMode(FAN_PIN, OUTPUT);
  digitalWrite(FAN_PIN, HIGH);

  pinMode(GEN_PIN, OUTPUT);
  digitalWrite(GEN_PIN, HIGH);

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Hello");
  lcd.clear();
}

int counter = 0;

void loop()
{
  sensor_manager();
  lcd_manager();

  lcd1.target_curr = analogRead(POT_PIN);
  lcd1.target_curr = map(lcd1.target_curr, 0, 4095, 10, 99);
  
  float ozone_ppm = float(lcd1.target_curr) / 10;
  float ozone_mg_m3 = 2.14;
  float cube_volume = 0.125;
  float quantita_ozono_richiesta = ozone_ppm * ozone_mg_m3 * cube_volume;
  float macchina_usate = 100;
  float rapporto_macchina_usata_e_quantita_ozono_richiesta = macchina_usate / quantita_ozono_richiesta;
  float secondi_accensione_x_minuto = 3600 / rapporto_macchina_usata_e_quantita_ozono_richiesta;

  long millis_accensione = long(secondi_accensione_x_minuto) * 1000;

  if (is_on)
  {
    if (millis() - current_millis > millis_accensione)
    {
      current_millis = millis();

      digitalWrite(GEN_PIN, LOW);
      is_on = 0;
      counter = 0;
    }
  }
  else
  {
    if (millis() - current_millis > 3600000 - millis_accensione)
    {
      current_millis = millis();

      digitalWrite(GEN_PIN, HIGH);
      is_on = 1;
      counter = 0;
    }
  }

  if (millis() - one_second_current_millis > 1000)
  {
    one_second_current_millis = millis();

    lcd.setCursor(11, 1);
    counter++;
    if (is_on) lcd.print(String(int(secondi_accensione_x_minuto) - counter) + "+    ");
    else  lcd.print(String(int(3600 - secondi_accensione_x_minuto) - counter) + "-    ");
  }


}
