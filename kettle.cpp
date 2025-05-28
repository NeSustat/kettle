#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>
#include "OneWire.h"
#include "DallasTemperature.h"

// Константы для экономии памяти
const uint8_t ONE_WIRE_PIN = 12;
const uint8_t WATER_LEVEL_PIN = 13;
const uint8_t BUTTON_PIN = 2;
const uint8_t RELAY_PIN = 6;
const uint8_t INPUT_PINS[] = {A0, A1, A2, A3};
const uint8_t NUM_INPUTS = 4;

// Настройки температуры
const int8_t DEFAULT_TEMP_END = 100;
const int8_t TEMP_START_CONST = 50;
const int8_t TEMP_CHANGE_STEP = 5;

// const
struct {
  uint16_t check_change = 10000;
  uint8_t timeBottonDelay = 200;
  uint8_t constTimeCheckTemp = 0;
  uint16_t end_settings = 6000;
  const uint16_t HOLD_TIME = 450;
  const uint16_t LOGO_DISPLAY_TIME = 65056;
  uint32_t timeEndHold = 60000 * 3;
} consts;

// хуета и хуй мне в рот и род 
uint8_t countButtonClick = 0;
bool flagHueta = true;
bool flagSpermoed = false;

// Настройки времени

OneWire oneWire(ONE_WIRE_PIN);
DallasTemperature ds(&oneWire);
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

// Переменные состояния
struct {
  int8_t end_temp = DEFAULT_TEMP_END;
  uint8_t start_temp;
  bool button_state = false;
  bool power_on = true;
  bool power = true;
  uint8_t water_level = 0;
  int8_t tempStatus = 0;
} state;

// Временные переменные
struct {
  uint32_t display_time = 0;
  uint32_t last_check_level_water = 0;
  uint32_t timeButtonBounce = 0;
  uint32_t timeLastCheckTemp = 0;
  uint32_t timeEndWork = 0;
} timing;

// Буфер для вывода текста (уменьшенный размер)
char display_buffer[16];

// Прототипы функций
void displayLogo(int8_t x, int8_t y);
void displayText(int8_t x, int8_t y, const char* text);
void displayProgressBar();
void displayAdditionalText(int8_t x, int8_t y, const char* format, uint8_t value);
int8_t getTemperature();
uint8_t getWaterLevel();
void buttonTick();
void kettleOffOn();
void checkTemp();
void workKettle();
void drawfLOGO();
void tempMaintein();

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

void displayProgressBar(uint8_t tempCur) {
  uint8_t progress = (100 * (getTemperature() - state.start_temp)) / (state.end_temp - state.start_temp);
  u8g2.clearBuffer();
  u8g2.setBitmapMode(1);
  u8g2.drawFrame(12, 21, 104, 20);
  u8g2.drawBox(14, 23, progress, 16);
  snprintf(display_buffer, sizeof(display_buffer), "Temp: %dC", tempCur);
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

// работа чайника
void workKettle(){
  countButtonClick = 0;
  timing.timeLastCheckTemp = millis();
  while(state.button_state){
    if (millis() - timing.timeLastCheckTemp >= consts.constTimeCheckTemp){
      state.tempStatus = getTemperature();
      if (state.tempStatus >= state.end_temp && state.tempStatus > 0){
        //Serial.println("Hui");
        displayText(37, 33, "Water ready");
        state.power_on = true;
        state.button_state = false;
        digitalWrite(RELAY_PIN, HIGH);
        timing.timeEndWork = millis();
        flagSpermoed = false;
        timing.display_time = millis();
        break;
      }
      timing.timeLastCheckTemp = millis();
      if (state.tempStatus - 1 > state.start_temp) {
          displayProgressBar(state.tempStatus);
      }
    }
  }
}

void tempMaintein(){
    if (flagSpermoed){
        state.tempStatus = getTemperature();
        if (millis() - timing.timeEndWork <= consts.timeEndHold){
            if (state.tempStatus >= state.end_temp){
                digitalWrite(RELAY_PIN, HIGH);
            } else if (state.tempStatus <= state.end_temp - 3){
                digitalWrite(RELAY_PIN, LOW);
            }
        } else{
            flagSpermoed = false;
        }
    }
}

// check temp
void checkTemp(){
  state.tempStatus = getTemperature();
  if (state.tempStatus >= state.end_temp && state.tempStatus > 0) {
    //Serial.println(state.tempStatus);
    // Serial.println(getTemperature());
    displayText(37, 33, "Water ready");
    state.power_on = true;
    state.button_state = false;
    flagSpermoed = true;
    timing.timeEndWork = millis();
    digitalWrite(RELAY_PIN, HIGH);
  } else {    
    state.button_state = true;
    state.start_temp = state.tempStatus;
    digitalWrite(RELAY_PIN, LOW);
    displayText(40, 33, "Wait for it!");
    workKettle();
  }
  timing.display_time = millis();
  countButtonClick = 0;
  flagHueta = true;
}

// ofTебаlи
void kettleOffOn(){
  if (millis() - timing.timeButtonBounce >= consts.HOLD_TIME){
    if (countButtonClick == 1 && flagHueta){
      state.button_state = !state.button_state;
      if (state.button_state){
        state.end_temp = 100;
        checkTemp();
      } else {
        countButtonClick = 0;
        flagHueta = true;
        state.power = true;
        digitalWrite(RELAY_PIN, HIGH);
      }
      countButtonClick = 0;
    } else if (countButtonClick >= 2 || !flagHueta) {
      if (flagHueta){
        countButtonClick = 0;
      }
      flagHueta = false;
      if (millis() - timing.timeButtonBounce >= consts.end_settings){
        state.power = true;
        countButtonClick = 0;
        digitalWrite(RELAY_PIN, HIGH);
        flagHueta = true;
      }
      if (countButtonClick == 1){
        countButtonClick = 0;
        state.end_temp = (state.end_temp >= 100) ? TEMP_START_CONST : state.end_temp + TEMP_CHANGE_STEP;
      } else if (countButtonClick >= 2){
        countButtonClick = 0;
        checkTemp();
      }
      if (!flagHueta){
        u8g2.clearBuffer();
        displayAdditionalText(10, 33, "Degrees: %dC", state.end_temp);
        u8g2.sendBuffer();
      }        
    } 
  }
}

void buttonTick() {
  if (millis() - timing.timeButtonBounce >= consts.timeBottonDelay){
    countButtonClick++;
    timing.timeButtonBounce = millis();
    //Serial.println(countButtonClick);
  }
  if (countButtonClick > 3){
    digitalWrite(RELAY_PIN, HIGH);
    flagHueta = true;
    countButtonClick = 0;
  }
}

void drawfLOGO(){
  if (millis() - timing.last_check_level_water >= consts.check_change){
    timing.last_check_level_water = millis();
    if (state.water_level != getWaterLevel()){
      state.power = true;
    }
  }
  // Отображение логотипа при необходимости
  if ((flagHueta && state.power_on && (millis() - timing.display_time >= consts.LOGO_DISPLAY_TIME)) || 
      state.power && flagHueta) {
    state.power_on = false;
    state.power = false;
    displayLogo(37, 3);
  }
}

void setup() {
  u8g2.setBitmapMode(1);
  u8g2.setFont(u8g2_font_helvB08_tr);
  u8g2.begin();
  Serial.begin(9600);
  ds.begin();
  
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  attachInterrupt(0, buttonTick, FALLING);
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(WATER_LEVEL_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH);
  
  for(uint8_t i = 0; i < NUM_INPUTS; i++) {
    pinMode(INPUT_PINS[i], INPUT);
  }
}

void loop() {
  // Serial.println(getTemperature());
  kettleOffOn();
  tempMaintein();
  drawfLOGO();
}

