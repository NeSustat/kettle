#include <Arduino.h>
//#include <ctime>
#include <U8g2lib.h>
#include <Wire.h> // library requires for IIC communication
#include "OneWire.h"
#include "DallasTemperature.h"

OneWire oneWire(12);  // порт подключения датчика
DallasTemperature ds(&oneWire);

/* настройки настроек для настроек */
// температура
int temp_end = 100; // максимальная температуру, когда надо отключить чайник
int end_temper = 100;
int temp_start_const = 50;
int temp_change = 5;

int cond_water = 0; // уровень воды
bool button = false; // нажата ли кнопка

// настройка со временем
int time_to_hold = 350;
int time_to_press = 30;
long long int fmillis = 0;
long long int raznitca = 0;
long long int time_when_was_first_click = 0;
int temp_start; // запоминаем температуру на момент нажатия кнопки

// флаги для лого
int power = 1;
int time_logo = 3000; // задаржка перед появление лого после последних действой
unsigned long time_start = -2500000; // запоминаем время вывода разных надписей
int power_on = 1; // проверяем были ли процессы до
// int temper_logo = 50; // температуру при которой должно выводиться лого

// текст 
char ch = 'C'; // используется для вывода на монитор, как поментка градусов
int x_text = 10; // позиция вывода текста по x
int y_text = 33; // позиция вывода текста по y

// флаги
int water_level_flag;
int button_flag = 0;
int button_flag1 = 0;
int button_mode = 0;
int flag;
/* */

// другое
int suchka;
int qwe;
double progress;

/* настройка пинов */
int button_click;
int temp = 0;
int water_level_pin = 13;
int button_pin = 2;
int rele_signal = 6;
const int kolvo_inputs = 4;
const int Vikhodi[kolvo_inputs] = {A0, A1, A2, A3};
char buffer[32]; // helper buffer to construct a string to be displayed

U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE); // initialization for the used OLED display
/* */





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
  water_level_flag = volume();
  Additional_Text(x-7, y+60, water_level_flag, "Cups of tea: %d");
	u8g2.sendBuffer();
}
/* */





/* вывод на экран */
// отрисовка текста в одну строку (координата X, координата Y, текст)
void Screen_Text(int x, int y, char Text[999999]) {
  u8g2.clearBuffer();
  //char buffer[32]; // helper buffer to construct a string to be displayed
  sprintf(buffer, Text); // construct a string with the progress variable
  u8g2.drawStr(x, y, buffer); // display the string
  u8g2.sendBuffer();
}

// отрисовка прогресс бара
void Progress_Bar(){
  progress = (100 * (temper() - temp_start)) / (end_temper - temp_start);
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

// отрисовка текста в две строки
void Additional_Text(int x, int y, int flag, char text[999999]){
  //u8g2.drawStr(x1, y1, Text);
  sprintf(buffer, text, flag); 
  u8g2.drawStr(x, y, buffer);
}
/* */





/* разные проверки */
// проверка на температуру
int temper() {
    ds.requestTemperatures(); 
    temp = ds.getTempCByIndex(0);
    Serial.println(temp);
    return (temp);
}

// проверка уровня воды
int volume(){
  digitalWrite(water_level_pin, HIGH);
  int text = 0;
  for(int i = 0; i < kolvo_inputs; i++){
    if (analogRead(Vikhodi[i]) > 300){
      text++;
    }
		//Serial.print(analogRead(Vikhodi[i] ));
  }
	//Serial.println();
  return(text);
}
/* */





// действия с кнопкой
void button_press(){
  Serial.println("buton");
    if (temper() >= end_temper){
      Screen_Text(x_text + 27, y_text, "Water ready");
      time_start = millis();
      power_on = 1;
    } else {
      power_on = 0;
    }
    if (cond_water){
      button = !button;
      delay(200);
      if (button){
        flag = volume();
        //Serial.println(flag);
        if (flag > 0){
          Screen_Text(x_text+30, y_text, "Wait for it!");
          // вывод на экран Уровня воды
          digitalWrite(rele_signal, HIGH);
          // запоминаем температуру на момент включания чайника
          temp_start = temper();
        } else{
          button = !button;
          digitalWrite(rele_signal, LOW);
          Screen_Text(x_text, y_text, "Low level of Water");
          // запоминаем время когда появилась надпись
          time_start = millis();
          power_on = 1;
        }        
      } else{
        digitalWrite(rele_signal, LOW);
        if (temper() < end_temper){
          power = 1;
        }
      }
    }
}



// смотрим что хочет пользователь (поменять или просто запустить чайник)
int what_i_need_to_do(){
  button_mode = 0;
  button_click = digitalRead(button_pin);
  if (button_click){
    if (time_when_was_first_click == 0){
      time_when_was_first_click = millis();
    }
    return 0;
  } else{
    raznitca = millis() - time_when_was_first_click;
    if (raznitca > time_to_press && time_when_was_first_click){
      button_mode = 1;
    }
    if (raznitca > time_to_hold && time_when_was_first_click){
      button_mode = 2;
      
    }
    time_when_was_first_click = 0;
    return button_mode;
  }
}



// задаем теперературы которую хочет пользователь если хочет
void konechnaya_end_temper(){
  suchka = what_i_need_to_do();
  if (suchka == 1){
    //Serial.println(100);
    button_press();
  } 
  else if (suchka == 2){
    fmillis = millis();
    end_temper = temp_start_const;
    while (true){
      /*
      if (millis() - fmillis >= 4500){
        end_temper = temp_end;
        Serial.println("end");
        break;
      }
      */
      qwe = what_i_need_to_do();
      if (qwe == 1){
        fmillis = millis();
        if (end_temper >= 100){
          end_temper = temp_start_const;
        } else{
          end_temper += temp_change;
        }
        //Serial.println(end_temper);
      }
      else if (qwe == 2){
        fmillis = millis();
        //Serial.println(end_temper);
        button_press();
        break;
      }
      u8g2.clearBuffer();
      Additional_Text(x_text, y_text, end_temper, "Degrees: %dC");
      u8g2.sendBuffer();
    }
  }
}





// настройка портов
void setup() {
	u8g2.setBitmapMode(1);
	u8g2.setFont(u8g2_font_helvB08_tr);
	u8g2.begin(); // start the u8g2 library
	Serial.begin(9600);   // инициализация монитора порта
	ds.begin();                 // инициализация датчика ds18b20
	pinMode(button_pin, INPUT_PULLUP);
	pinMode(rele_signal, OUTPUT);

	for(int i = 0; i < kolvo_inputs; i++){
		pinMode(Vikhodi[i], INPUT);
	}
	pinMode(water_level_pin, OUTPUT);
}





// просто цикл
void loop() {
  // отрисовка начального экрана
  if ((power_on && millis() - time_start >= time_logo) || power || water_level_flag != volume()){  
    power_on = 0;
    power = 0;
    Logo(37,3);
  }

  // смотрим что хочет пользователь
  konechnaya_end_temper();

/*
  // поведенте кнопуки 
  bool btnState = digitalRead(button_pin);
  if (btnState && !flag111 && millis() - btnTimer > 100) {
    flag111 = true;
    btnTimer = millis();
    button_flag = 1;
  }

  if (btnState && flag111 && millis() - btnTimer > 400) {
    btnTimer = millis();
    button_flag = 2;
  }

  if (!btnState && flag111 && millis() - btnTimer > 500) {
    flag111 = false;
    btnTimer = millis();
    //Serial.println("release");
  }
  if (millis() - btnTimer > 400){
    if (button_flag == 1){
      button_press();
      button_flag = 0;
      //Serial.println("press");
    }
    if (button_flag == 2){
      int fmillis = millis();
      temp_end = 50;
      while (millis() - fmillis != 150000){
        if (temp_end > 100){
          temp_end = 50;
        }
        bool btnState1 = digitalRead(button_pin);
        if (btnState1 && !flag1111 && millis() - btnTimer1 > 100) {
          flag1111 = true;
          btnTimer1 = millis();
          button_flag1 = 1;
          fmillis = millis();
        }

        if (btnState1 && flag1111 && millis() - btnTimer1 > 400) {
          btnTimer1 = millis();
          button_flag1 = 2;
          fmillis = millis();
        }

        if (!btnState1 && flag1111 && millis() - btnTimer1 > 500) {
          flag1111 = false;
          btnTimer1 = millis();
          fmillis = millis();
          //Serial.println("release");
        }
        if (millis() - btnTimer1 > 400){
          if (button_flag1 == 1){
            temp_end += 5;
            button_flag1 = 0;
            fmillis = millis();
            //Serial.println("press");
          }
          if (button_flag1 == 2){
            button_press();
            button_flag1 = 0;
            fmillis = millis();
            break;
          }
        }
        u8g2.clearBuffer();
        Additional_Text(x_text, y_text, temp_end, "Degrees: %dC");
        u8g2.sendBuffer();
        //Serial.println("press hold");
      }
      button_flag = 0;
    }
  }
  */

  // проверка на температуру
  if (temper() >= end_temper){
    // проверяем выводили 
    if (cond_water && button){
      digitalWrite(rele_signal, LOW); // выключаем электричество
      cond_water = 0; // флаг
      button = false;
      // вывод что вода готова
      Screen_Text(x_text + 27, y_text, "Water ready");
      // задержка перед след нажатием кнопки
      time_start = millis();
      power_on = 1;
      end_temper = temp_end; // значение температуры по умолчанию
    } 
  } else {
    //Serial.println(end_temper);
    cond_water = 1;
    Serial.println(temper());
        //Serial.println(temp_start);
    if (cond_water && button){
      if (temper() - 1 > temp_start){
        Progress_Bar();
      }
    }
  }
}