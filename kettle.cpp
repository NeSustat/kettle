#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>
#include "OneWire.h"
#include "DallasTemperature.h"
#include "GyverPID.h"

// Константы для экономии памяти
const uint8_t ONE_WIRE_PIN = 12;
const uint8_t WATER_LEVEL_PIN = 13;
const uint8_t BUTTON_PIN = 3;
const uint8_t RELAY_PIN = 6;
const uint8_t INPUT_PINS[] = {A0, A1, A2, A3};
const uint8_t NUM_INPUTS = 4;

// Настройки температуры
const uint8_t DEFAULT_TEMP_END = 100;
const uint8_t TEMP_START_CONST = 50;
const uint8_t TEMP_CHANGE_STEP = 5;

// Настройки времени
const uint16_t HOLD_TIME = 200;
const uint16_t PRESS_TIME = 20;
const uint16_t LOGO_DISPLAY_TIME = 180000;

// flags
volatile bool intFlag = false;   // флаг

OneWire oneWire(ONE_WIRE_PIN);
DallasTemperature ds(&oneWire);
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);


// PID
GyverPID regulator(0.1, 0.05, 0.01, 10);  // коэф. П, коэф. И, коэф. Д, период дискретизации dt (мс)
// или так:
// GyverPID regulator(0.1, 0.05, 0.01);	// можно П, И, Д, без dt, dt будет по умолч. 100 мс


// Переменные состояния
struct {
  uint8_t end_temp = DEFAULT_TEMP_END;
  uint8_t start_temp;
  bool button_state = false;
  bool power_on = true;
  bool power = true;
  bool water_ok = false;
  uint8_t water_level = 0;
  uint8_t button_mode = 0;
} state;

// Временные переменные
struct {
  uint32_t first_click_time = 0;
  uint32_t display_time = 0;
  uint32_t setting_time = 0;
  uint32_t end_settings = 6000;
  uint32_t last_check_level_water = 0;
  uint32_t check_change = 10000;
  uint32_t time_end = 0;
  uint16_t tine_end_const = 600000;
} timing;

// Буфер для вывода текста (уменьшенный размер)
char display_buffer[24];

// Прототипы функций
void displayLogo(int8_t x, int8_t y);
void displayText(int8_t x, int8_t y, const char* text);
void displayProgressBar();
void displayAdditionalText(int8_t x, int8_t y, const char* format, uint8_t value);
int8_t getTemperature();
uint8_t getWaterLevel();
void handleButtonPress();
uint8_t checkButtonAction();
void handleTemperatureSetting();
void buttonTick();

// Логотип - оптимизированные функции рисования
void drawF(int8_t x, int8_t y) {
	u8g2.drawLine(x+1, y, x+24, y);
	u8g2.drawBox(x, y+1, 26, 6);
	u8g2.drawLine(x, y+7, x+24, y+7);
	u8g2.drawBox(x, y+7, 8, 37);
	u8g2.drawLine(x+1, y+44, x+6, y+44);
	u8g2.drawLine(x+8, y+18, x+8, y+26);
	u8g2.drawLine(x+17, y+20, x+17, y+24);
	u8g2.drawLine(x+16, y+20, x+16, y+24);
	u8g2.drawLine(x+8, y+19, x+16, y+19);
	u8g2.drawLine(x+8, y+20, x+16, y+20);
	u8g2.drawLine(x+8, y+24, x+16, y+24);
	u8g2.drawLine(x+8, y+25, x+16, y+25);
	
	u8g2.drawLine(x+13, y+20, x+13, y+24);
}

void drawP(int8_t x, int8_t y) {
	u8g2.drawBox(x+1, y, 6, 33);
	u8g2.drawLine(x, y+1, x, y+31);
	u8g2.drawLine(x+7, y, x+7, y+31);
	u8g2.drawBox(x+8, y, 10, 8);
	u8g2.drawBox(x+11, y+1, 8, 19);
	u8g2.drawBox(x+8, y+13, 10, 8);
}

void drawT(int8_t x, int8_t y) {
	u8g2.drawLine(x+1, y, x+26, y);
	u8g2.drawBox(x, y+1, 28, 6);
	u8g2.drawLine(x+1, y+7, x+26, y+7);
	u8g2.drawBox(x+10, y+7, 8, 37);
	u8g2.drawLine(x+11, y+44, x+16, y+44);
}

void displayLogo(int8_t x, int8_t y) {
  u8g2.clearBuffer();
  drawF(x, y);
  drawP(x+22, y+12);
  drawT(x+36, y);
  state.water_level = getWaterLevel();
  displayAdditionalText(x-7, y+60, "Cups of tea: %d", state.water_level);
  u8g2.sendBuffer();
}

void displayText(int8_t x, int8_t y, const char* text) {
  u8g2.clearBuffer();
  u8g2.drawStr(x, y, text);
  u8g2.sendBuffer();
}

void displayProgressBar() {
  uint8_t progress = (100 * (getTemperature() - state.start_temp)) / (state.end_temp - state.start_temp);
  u8g2.clearBuffer();
  u8g2.setBitmapMode(1);
  u8g2.drawFrame(12, 21, 104, 20);
  u8g2.drawBox(14, 23, progress, 16);
  snprintf(display_buffer, sizeof(display_buffer), "Temp: %dC", getTemperature());
  u8g2.drawStr(33, 53, display_buffer);
  u8g2.drawStr(0, 7, "Progress Bar");
  u8g2.drawLine(0, 9, 127, 9);
  u8g2.sendBuffer();
}

void displayAdditionalText(int8_t x, int8_t y, const char* format, uint8_t value) {
  snprintf(display_buffer, sizeof(display_buffer), format, value);
  u8g2.drawStr(x, y, display_buffer);
}

int8_t getTemperature() {
  ds.requestTemperatures(); 
  int8_t temp = ds.getTempCByIndex(0);
  Serial.println(temp);
  return temp;
}

uint8_t getWaterLevel() {
  digitalWrite(WATER_LEVEL_PIN, HIGH);
  uint8_t level = 0;
  for(uint8_t i = 0; i < NUM_INPUTS; i++) {
    if (analogRead(INPUT_PINS[i]) > 300) level++;
  }
  return level;
}

void handleButtonPress() {
  if (getTemperature() >= state.end_temp) {
    displayText(37, 33, "Water ready");
    timing.display_time = millis();
    state.power_on = true;
    return;
  }

  if (state.water_ok) {
    state.button_state = !state.button_state;
    delay(200);
    
    if (state.button_state) {
      uint8_t level = getWaterLevel();
      if (level > 0) {
        displayText(40, 33, "Wait for it!");
        // digitalWrite(RELAY_PIN, HIGH);
        state.start_temp = getTemperature();
      } else {
        state.button_state = false;
        digitalWrite(RELAY_PIN, LOW);
        displayText(10, 33, "Low water level");
        timing.display_time = millis();
        state.power_on = true;
      }
    } else {
      digitalWrite(RELAY_PIN, LOW);
      if (getTemperature() < state.end_temp) state.power = true;
    }
  }
}

uint8_t checkButtonAction() {
  state.button_mode = 0;
  if (intFlag) {
    if (timing.first_click_time == 0) {
      timing.first_click_time = millis();
    }
    return 0;
  }
  
  if (timing.first_click_time) {
    uint32_t elapsed = millis() - timing.first_click_time;
    timing.first_click_time = 0;
    
    if (elapsed > PRESS_TIME) state.button_mode = 1;
  
  pinMode(RELAY_PIN, OUTPUT);
    if (elapsed > HOLD_TIME) state.button_mode = 2;
  }
  
  return state.button_mode;
}

void handleTemperatureSetting() {
  uint8_t action = checkButtonAction();
  
  if (action == 1) {
    handleButtonPress();
  } 
  else if (action == 2) {
    timing.setting_time = millis();
    state.end_temp = TEMP_START_CONST;
    
    while (true) {
      action = checkButtonAction();
      if (millis() - timing.setting_time >= timing.end_settings){
        state.power = true;
        break;
      }
      if (action == 1) {
        timing.setting_time = millis();
        state.end_temp = (state.end_temp >= 100) ? TEMP_START_CONST : state.end_temp + TEMP_CHANGE_STEP;
      }
      else if (action == 2) {
        timing.setting_time = millis();
        handleButtonPress();
        break;
      }
      
      u8g2.clearBuffer();
      displayAdditionalText(10, 33, "Degrees: %dC", state.end_temp);
      u8g2.sendBuffer();
    }
  }
}

void setup() {
  u8g2.setBitmapMode(1);
  u8g2.setFont(u8g2_font_helvB08_tr);
  u8g2.begin();
  Serial.begin(9600);
  ds.begin();
  
  attachInterrupt(BUTTON_PIN, buttonTick, FALLING);
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(WATER_LEVEL_PIN, OUTPUT);
  
  for(uint8_t i = 0; i < NUM_INPUTS; i++) {
    pinMode(INPUT_PINS[i], INPUT);
  }
  regulator.setDirection(NORMAL); // направление регулирования (NORMAL/REVERSE). ПО УМОЛЧАНИЮ СТОИТ NORMAL
  regulator.setLimits(0, 255);    // пределы (ставим для 8 битного ШИМ). ПО УМОЛЧАНИЮ СТОЯТ 0 И 255
  regulator.setpoint = 50;        // сообщаем регулятору температуру, которую он должен поддерживать


}

void buttonTick() {
  intFlag = true;   // подняли флаг прерывания
}

void loop() {
  if (millis() - timing.last_check_level_water >= timing.check_change){
    timing.lregulator.input = temp;ast_check_level_water = millis();
    if (state.water_level != getWaterLevel()){
      state.power = true;
    }
  }
  // Отображение логотипа при необходимости
  if ((state.power_on && (millis() - timing.display_time >= LOGO_DISPLAY_TIME)) || 
      state.power) {
    state.power_on = false;
    state.power = false;
    displayLogo(37, 3);
  }

  // Обработка действий пользователя
  handleTemperatureSetting();

  // Проверка температуры
  if (getTemperature() >= state.end_temp) {
    if (state.water_ok && state.button_state) {
      timing.time_end = millis();
      state.water_ok = false;
      state.button_state = false;
      displayText(37, 33, "Water ready");
      timing.display_time = millis();
      state.power_on = true;
      state.end_temp = DEFAULT_TEMP_END;
    }
  } else {
    state.water_ok = true;
    regulator.input = temper();
    analogWrite(RELAY_PIN, regulator.getResultTimer()); 
    if (state.button_state) {
      if (getTemperature() - 1 > state.start_temp) {
        displayProgressBar();
      }
    }
  }
  if (timing.time_end && millis() - timing.time_end >= timing.time_end_const){
    timing.time_end = 0;
    digitalWrite(RELAY_PIN, LOW);
  }
}
