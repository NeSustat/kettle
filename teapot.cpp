#include <Arduino.h>
//#include <ctime>
#include <U8g2lib.h>
#include <Wire.h> // library requires for IIC communication
#include "OneWire.h"
#include "DallasTemperature.h"

OneWire oneWire(12);  // порт подключения датчика
DallasTemperature ds(&oneWire);

/* общие настройки переменных */
int Water_Level = 0; // уровень воды
bool button = false; // нажата ли кнопка
int water_100 = 11;
int rele_signal = 10;
int temp_end = 70; // максимальная температуру, когда надо отключить чайник
int temp_start; // запоминаем температуру на момент нажатия кнопки
char ch = 'C'; // используется для вывода на монитор, как поментка градусов
// int temper_logo = 50; // температуру при которой должно выводиться лого
int Power = 1;
unsigned long Time_Start = -2500000; // запоминаем время вывода разных надписей
int Power_On = 1; // проверяем были ли процессы до
int Time_Logo = 3000; // задаржка перед появление лого после последних действой
int x_text = 10; // позиция вывода текста по x
int y_text = 33; // позиция вывода текста по y


/* настройка пинов */
int Water_Level_pin = 13;
int button_pin = 3;
const int kolvo_inputs = 4;
const int Vikhodi[kolvo_inputs] = {A0, A1, A2, A3};
char buffer[32]; // helper buffer to construct a string to be displayed


U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE); // initialization for the used OLED display

/* LOGO */
// chat "F"
void out_F(int x, int y){
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
	// u8g2.drawLine(x+14, y+20, x+14, y+24);
}
// char "P"
void out_P(int x, int y){
	u8g2.drawBox(x+1, y, 6, 33);
	u8g2.drawLine(x, y+1, x, y+31);
	u8g2.drawLine(x+7, y, x+7, y+31);
	u8g2.drawBox(x+8, y, 10, 8);
	u8g2.drawBox(x+11, y+1, 8, 19);
	u8g2.drawBox(x+8, y+13, 10, 8);
}
// char "T"
void out_T(int x, int y){
	u8g2.drawLine(x+1, y, x+26, y);
	u8g2.drawBox(x, y+1, 28, 6);
	u8g2.drawLine(x+1, y+7, x+26, y+7);

	u8g2.drawBox(x+10, y+7, 8, 37);
	u8g2.drawLine(x+11, y+44, x+16, y+44);
}
// полная отрисовка всех букв сразу
void Logo(int x, int y){
	u8g2.clearBuffer();
	out_F(x+0, y+0);
	out_P(x+22,y+12);
	out_T(x+36,y+0);
	u8g2.sendBuffer();
}

// проверка на температуру
int temper() {
    ds.requestTemperatures(); 
    int temp = ds.getTempCByIndex(0);
    Serial.println(temp);
    return (temp);
    
}

// проверка уровня воды
int volume(){
  digitalWrite(Water_Level_pin, HIGH);
  int text = 0;
  for(int i = 0; i < kolvo_inputs; i++){
    if (analogRead(Vikhodi[i]) > 300){
      text++;
    }
		Serial.print(analogRead(Vikhodi[i] ));
  }
	Serial.println();
  return(text);
}

// настройка портов
void setup() {
	u8g2.setBitmapMode(1);
	u8g2.setFont(u8g2_font_helvB08_tr);
	u8g2.begin(); // start the u8g2 library
	Serial.begin(9600);   // инициализация монитора порта
	ds.begin();                 // инициализация датчика ds18b20
	pinMode(water_100, OUTPUT); 
	pinMode(button_pin, INPUT_PULLUP);
	pinMode(rele_signal, OUTPUT);

	for(int i = 0; i < kolvo_inputs; i++){
		pinMode(Vikhodi[i], INPUT);
	}
	pinMode(Water_Level_pin, OUTPUT);
}

// просто цикл
void loop() {
    if ((Power_On && millis() - Time_Start >= Time_Logo) || Power){  
        Power_On = 0;
        Power = 0;
        Logo(37,10);  
    }  

    // если нажать кнопку загарается лампочка
    if (digitalRead(button_pin) == 1 && Water_Level){
        button = !button;
        delay(250);
        if (button){
            int flag = volume();
            Serial.println(flag);
            if (flag > 0){
                // выводим на экран что воды достаточно
                u8g2.clearBuffer();
                sprintf(buffer, "Water level: %d%%", flag); 
                u8g2.drawStr(x_text + 20, y_text, buffer);
                u8g2.sendBuffer();
                // вывод на экран Уровня воды
                digitalWrite(rele_signal, HIGH);
                // запоминаем температуру на момент включания чайника
                temp_start = temper();
            } else{
                button = !button;
                digitalWrite(rele_signal, LOW);
                u8g2.clearBuffer();
                sprintf(buffer, "Low level of Water"); // construct a string with the progress variable
                u8g2.drawStr(x_text, y_text, buffer); // display the string
                u8g2.sendBuffer();
                // запоминаем время когда появилась надпись
                Time_Start = millis();
                Power_On = 1;
                // вывод на экран Воды мало
            }
            
        } else{
            digitalWrite(rele_signal, LOW);
            if (temper() < temp_end){
                Power = 1;
            } else {
                u8g2.clearBuffer();
                sprintf(buffer, "Water ready"); // construct a string with the progress variable
                u8g2.drawStr(x_text + 20, y_text, buffer); // display the string
                u8g2.sendBuffer();
                Time_Start = millis();
                Power_On = 1;
            }
        }
    }
    // проверка на температуру
    if (temper() >= temp_end){
        // проверяем выводили 
        if (Water_Level && button){
            digitalWrite(rele_signal, LOW); // выключаем электричество
            Water_Level = 0; // флаг
            button = false;
            // вывод что вода готова
            u8g2.clearBuffer();
            sprintf(buffer, "Water ready"); // construct a string with the progress variable
            u8g2.drawStr(x_text +20, y_text, buffer); // display the string
            u8g2.sendBuffer();
            Time_Start = millis();
            Power_On = 1;
        } else {
            // если вода меньше temp_end то мы выключаем лампочку горячей воды
            digitalWrite(water_100, LOW);  
            u8g2.clearBuffer();        
        }
        u8g2.clearBuffer();
        
    } else {
      Water_Level = 1;
      if (Water_Level && button){
        if (temper() - 1 > temp_start){
            double progress = (100 * (temper() - temp_start)) / (temp_end - temp_start);
            u8g2.clearBuffer();
            u8g2.setBitmapMode(1);
            u8g2.drawFrame(12, 21, 104, 20);
            u8g2.drawBox(14, 23, progress, 16); // draw the progressbar fill
            sprintf(buffer, "Temper: %d%c", temper(), ch); // construct a string with the progress variable
            u8g2.drawStr(33, 53, buffer); // display the string
            u8g2.drawStr(0, 7, "Progress Bar Screen");
            u8g2.drawLine(0, 9, 127, 9);


            u8g2.sendBuffer();
        }
      }
    }
}

