/*
Программа: электрокардиогаф
Разработчик: Берестнев Дмитрий Дмитриевич
Версия: 0.2.1 
Дата создания: 03.11.19
*/
#include <SPFD5408_Adafruit_GFX.h>
#include <SPFD5408_Adafruit_TFTLCD.h>
#include <SPFD5408_TouchScreen.h>

#if defined(__SAM3X8E__)
    #undef __FlashStringHelper::F(string_literal)
    #define F(string_literal) string_literal
#endif

#define YP A3
#define XM A2
#define YM 9
#define XP 8

#define TS_MINX 125
#define TS_MINY 85
#define TS_MAXX 965
#define TS_MAXY 905

TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);

#define LCD_CS A3
#define LCD_CD A2
#define LCD_WR A1
#define LCD_RD A0

// optional
#define LCD_RESET A4

#define  BLACK   0x0000
#define BLUE    0x001F
#define RED     0xF800
#define GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF

Adafruit_TFTLCD tft(LCD_CS, LCD_CD, LCD_WR, LCD_RD, LCD_RESET);

#define BOXSIZE 40
#define PENRADIUS 3

#define MINPRESSURE 10
#define MAXPRESSURE 1000

int max_UR = 800;
int min_UR = 200;

int URx = A6;
int URy = A7;
int sw = 24;
int ekgPin = 8;
int lo_0 = 22;
int lo_1 = 26;

const int arr_size = 2560;
int result[arr_size];

int sensor_delay = 5;

float sizeY;

float max_sensor;
float min_sensor;

int timer;
int count_arr;

float factor, size_factor;

int positionY0, positionY1, positionX0 = 0, positionX1, max_positionX;

int timeScreen = 1500;
int calibration_time = 10000;
int timer_delay = 5000;
String version = "0.2.1";

void setup() {
  pinMode(URx, INPUT);
  pinMode(URy, INPUT);
  pinMode(lo_0, INPUT); // Настройка выхода L0-
  pinMode(lo_1, INPUT); // Настройка выхода L0+
  pinMode(sw, INPUT_PULLUP);//Подтягиваем резистр

  Serial.begin(9600);
  
  tft.reset();
  tft.begin(0x9341);
  tft.setRotation(-45);
  
  max_positionX = tft.width();
  sizeY = tft.height();
  
  startScreen();
  delay(timeScreen);
  globalMenu();
}
void scan(){
  int faza = 0;
  boolean input_block = false;
  boolean lastButtonL = false;//Переменная для проверки было ли препыдущим действие - джостик в лево
  while(true){
    if(digitalRead(sw) == 0 && input_block == false){
        input_block = true;
        faza += 1;
    }
    if(digitalRead(sw) == 1){
        input_block = false;
    }
    if(timer == arr_size){
      faza = 1;
    }
    if(faza == 0){
      errors();
      if(positionX0 >= max_positionX)
        {
          clearScreen(); 
          positionX0 = 0; 
        }
       result[timer] = analogRead(ekgPin);

       if(timer > 0){
         drawForward();
       }
       Serial.println(analogRead(result[timer]));
       count_arr++;
       timer++;
       delay(sensor_delay);
    }
    if(faza == 1){
      int URy_value = analogRead(URy);
      if(URy_value > max_UR && timer - max_positionX > 0){//Джостик в лево 
         clearScreen();
         lastButtonL = true;
         //if(timer % max_positionX != 0) timer -= timer % max_positionX; //Данный вариант тоже допустим
         if(positionX1 < max_positionX) timer -= positionX1;
         positionX0 = max_positionX;
         while(positionX0 > 0){
             drawBackward();
             timer--;
         }
      }
      if(URy_value < min_UR && count_arr > timer){//Джостик в право
         clearScreen();
         if(lastButtonL)timer += max_positionX;//(Если предыдущее действие было джостик в лево тогда выполнять эту команду)
         
         lastButtonL = false;
        
         positionX0 = 0;
         while(positionX0 < max_positionX && count_arr > timer){
             drawForward();
             timer++;
         }
      }
    }
    if(faza == 2){
      globalMenu();
    }
  }
}
void coordinateCalculation(){
     positionY0 = (result[timer-1] > min_sensor)? result[timer-1] - min_sensor :  result[timer-1];
     positionY1 = (result[timer] > min_sensor)? result[timer] - min_sensor :  result[timer];
     positionY0 *= factor * size_factor;
     positionY1 *= factor * size_factor;
}
void drawForward(){
     coordinateCalculation();
     positionX1 = positionX0 + 1;//* size_factor
     tft.drawLine(positionX0,positionY0, positionX1,positionY1, GREEN);
    
     positionX0 += 1;
}
void drawBackward(){
     coordinateCalculation();
     positionX1 = positionX0 - 1;
     tft.drawLine(positionX0,positionY1, positionX1,positionY0, GREEN);
    
     positionX0 -= 1;
}
String stringScanTime(){
  return (String)((float)(sensor_delay * arr_size) / 1000) + " seconds";
}
void errors(){
  if((digitalRead(lo_0) == 1)||(digitalRead(lo_1) == 1)){
        errorScreen(); 
        while(true){
          if(digitalRead(sw) == 0){
            globalMenu();
          }
        }
  }
}

void globalMenu(){
  clearScreen();
  drawBorder();
  tft.setTextColor(WHITE);
  tft.setTextSize(2);
  tft.setCursor(35, 100);
  tft.println("Click on the joystick");
  tft.setCursor(55, 120);
  tft.println("to start scanning");
  tft.setTextColor(RED);
  tft.setTextSize(1);
  tft.setCursor(50, 200);
  tft.println("Dont forget to attack the electrodes");
  tft.setCursor(60, 215);
  tft.println("Estimated scan time: " + stringScanTime());
  while(true){
   if(digitalRead(sw) == 0){
      startScan();
    } 
  }
}
void calibrationScreen(){
  clearScreen();
  drawBorder();
  tft.setTextColor(RED);
  tft.setCursor (60, 40);
  tft.setTextSize (3);
  tft.println("Calibration");
  tft.setTextSize (2);
  tft.setTextColor(WHITE);
  tft.setCursor (50, 80);
  tft.println("Please don't move");
  tft.setCursor (20, 100);
  tft.println("calibration in progress");
  delay(timeScreen);
  Calibration();
}
void Calibration(){
 max_sensor = 0;
 min_sensor = 1023;
 int PinValue, min_sensor_f, max_sensor_f;
 for(int t = 0; t < calibration_time; t+=5){
  errors();
  PinValue = analogRead(ekgPin);
  if(PinValue > max_sensor)max_sensor = PinValue;
  if(PinValue < min_sensor)min_sensor = PinValue;
  delay(sensor_delay);
 }
 factor = sizeY / max_sensor;
 min_sensor_f = min_sensor*factor;
 max_sensor_f = max_sensor*factor;
 size_factor = sizeY / (max_sensor_f - min_sensor_f);
}
void drawBorder () {
  int width = tft.width() - 1;
  int height = tft.height() - 1;
  int border = 10;

  tft.fillScreen(RED);
  tft.fillRect(border, border, (width - border * 2), (height - border * 2), BLACK);
}
void timerScreen(){
  clearScreen();
  tft.setTextColor(RED);
  tft.setTextSize (15);
  for(int t = timer_delay/1000; t >= 0; t--){
    clearScreen();
    tft.setCursor (130, 70);
    tft.println(t);
    delay(1000);
  }
}
void errorScreen(){
  clearScreen();
  drawBorder();
  tft.setTextColor(RED);
  tft.setCursor (120, 40);
  tft.setTextSize (3);
  tft.println("Error");
  tft.setTextSize (2);
  tft.setTextColor(WHITE);
  tft.setCursor (50, 80);
  tft.println("One of the sensors");
  tft.setCursor (60, 100);
  tft.println("is not connected.");
  tft.setCursor (30, 140);
  tft.println("Correct the sensor and");
  tft.setCursor (35, 160);
  tft.println("Click on the joystick");
  tft.setCursor (25, 180);
  tft.println("to start scanning again");
}
void startScreen(){
   drawBorder();
   tft.setTextColor(RED);
   tft.setCursor (50, 70);
   tft.setTextSize (2);
   tft.println("Electrocardiograph");
   tft.setTextColor(WHITE);
   tft.setCursor (30, 130);
   tft.println("Author: Berestnev");
   tft.setCursor (30, 150);
   tft.println("Dmitry Dmitrievich");
   tft.setCursor (30, 200);
   tft.setTextSize (1);
   tft.println("version: "+version);
}
void startScan(){
  count_arr = 0;
  timer = 0;
  positionX0 = 0; 
  timerScreen();
  calibrationScreen();
  clearScreen();
  scan();
}
void clearScreen(){
  tft.fillScreen(BLACK);
}
