// Copyright (c) 2021 SQ9MDD Rysiek Labus
// 
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#ifndef BOARD_HAS_PSRAM
#error "Please enable PSRAM !!!"
#endif

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "esp_adc_cal.h"
//#include <esp_task_wdt.h>
//#include "freertos/FreeRTOS.h"
//#include "firasans.h"
#include "opensans12b.h"
#include "opensans24b.h"
#include "opensans26b.h"
#include "opensans72.h"
#include "opensans56b.h"
#include "slonce_chmurka.h"
#include "slonce.h"
#include "ksiezyc_chmurka.h"
#include "ksiezyc.h"
#include "chmurka.h"
#include "deszcz.h"
#include "snieg.h"
#include "sunrise.h"
#include "sunset.h"
//#include "freertos/task.h"
#include "epd_driver.h"         // https://github.com/Xinyuan-LilyGO/LilyGo-EPD47
#include <Wire.h>
#include <ArduinoJson.h>

#define button_1 39
#define button_2 34
#define button_3 35
#define button_4 0
#define uS_TO_S_FACTOR 1000000
#define TIME_TO_SLEEP  600

String serverName = "http://" + String(JSON_IP) + "/json.htm?type=devices&rid=";
const char* DataServerTime = "2021-04-22 00:00:00";
float DataTemp = 0.0;
float DataBaro = 0.0;
int DataHumi = 0;
boolean dzien = true;
const char* DataSunrise = "00:00";
const char* DataSunset = "00:00";

String sensorReadings;
uint8_t *framebuffer = NULL;

String httpGETRequest(const char* serverName){
  HTTPClient http;
  http.begin(serverName);
  int httpResponseCode = http.GET();
  String payload = "{}"; 
  if (httpResponseCode>0){
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    payload = http.getString();
  }
  else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
    // Free resources
  http.end();
  return payload;
}

void show_today_weather(){
    int position_x = 250;
    int position_y = 160;
    String spacja = "";
    String str_temp = " ";
    int cursor_x = position_x + 0;
    int cursor_y = position_y;
    //Rect_t area = {
    //    .x = 250,
    //    .y = 60,
    //    .width = 310,
    //    .height = 260,
    //};
    if(DataTemp> 0.0 && DataTemp < 10.0){
      spacja = "   "; 
    }else{
      spacja = "";
    }
    str_temp = spacja + String(DataTemp,1) + "°C";
    writeln((GFXfont *)&opensans56B, (char *)str_temp.c_str(), &cursor_x, &cursor_y, NULL);
    cursor_x = position_x + 110;
    cursor_y += 60; 
    if(DataHumi < 10){
      spacja = "   "; 
    }else if(DataHumi > 9 && DataHumi < 99){
      spacja = "  "; 
    }else{
      spacja = "";
    }
    str_temp = spacja + String(DataHumi) + "% Rh";
    writeln((GFXfont *)&OpenSans26B, (char *)str_temp.c_str(), &cursor_x, &cursor_y, NULL); 
    cursor_x = position_x + 50;
    cursor_y += 55; 
    if(DataBaro < 1000.0){
      spacja = " ";
    }else{
      spacja = "";
    }
    str_temp = spacja + String(DataBaro,1) + " hPa";
    writeln((GFXfont *)&OpenSans26B, (char *)str_temp.c_str(), &cursor_x, &cursor_y, NULL);  
}

void show_day(){  // sunset & sunrise
    int x = 110;
    int y = 70;
    Rect_t area = {.x = x, .y = y, .width  = sunrise_width, .height =  sunrise_height};
    epd_draw_grayscale_image(area, (uint8_t *) sunrise_data);    
    y += 50;
    area = {.x = x, .y = y, .width  = sunset_width, .height =  sunset_height};
    epd_draw_grayscale_image(area, (uint8_t *) sunset_data); 
    int cursor_x = 35;
    int cursor_y = 105; 
    writeln((GFXfont *)&OpenSans12B, DataSunrise, &cursor_x, &cursor_y, NULL);  
    cursor_x = 35;
    cursor_y = 155; 
    writeln((GFXfont *)&OpenSans12B, DataSunset, &cursor_x, &cursor_y, NULL);       
}

void draw_frames(){
    epd_draw_rect(10, 10, EPD_WIDTH - 20, 38, 0, framebuffer);
    epd_draw_grayscale_image(epd_full_screen(), framebuffer);
    int cursor_x = 380;
    int cursor_y = 37; 
    writeln((GFXfont *)&OpenSans12B, DataServerTime, &cursor_x, &cursor_y, NULL);     
}

void draw_bat(){
  float voltage = analogRead(36) / 4096.0 * 6.566 * (1100 / 1000.0);
  int cursor_x = 885;
  int cursor_y = 37; 
  String volt_str = String(voltage,1) + "V";
  writeln((GFXfont *)&OpenSans12B, (char *)volt_str.c_str(), &cursor_x, &cursor_y, NULL);  
  int percent = map((voltage*10),33,42,30,64);
  percent = constrain(percent,30,64);
  // battery icon
  epd_draw_rect(800, 14, 72, 30, 0, framebuffer);
  epd_draw_line(872,20,879,20, 0, framebuffer);
  epd_draw_line(879,20,879,38, 0,framebuffer);
  epd_draw_line(872,38,879,38, 0, framebuffer);
  epd_fill_rect(804, 18, percent, 22, 0, framebuffer);  //percent fill in
}

void draw_weather_icon(){
  char curr_hr[5];
  char sunrise_hr[5];
  char sunset_hr[5];
  curr_hr[0] =  DataServerTime[11];
  curr_hr[1] =  DataServerTime[12];
  curr_hr[2] =  DataServerTime[14];
  curr_hr[3] =  DataServerTime[15];
  curr_hr[4] =  '\0';
  sunrise_hr[0] = DataSunrise[0];
  sunrise_hr[1] = DataSunrise[1];
  sunrise_hr[2] = DataSunrise[3];
  sunrise_hr[3] = DataSunrise[4];  
  sunrise_hr[4] = '\0';
  sunset_hr[0] = DataSunset[0];
  sunset_hr[1] = DataSunset[1];
  sunset_hr[2] = DataSunset[3];
  sunset_hr[3] = DataSunset[4];
  sunset_hr[4] = '\0';
  int curr_hr_int = atoi(curr_hr);
  int sunrize_hr_int = atoi(sunrise_hr);
  int sunset_hr_int = atoi(sunset_hr);

  if(curr_hr_int > sunrize_hr_int && curr_hr_int < sunset_hr_int){
    dzien = true;
  }else{
    dzien = false;
  }

  Rect_t area = {
        .x = 650,
        .y = 90,
        .width = 220,
        .height =  180
  }; 
    if(dzien){
      if(DataHumi > 85 && DataTemp > 3.0){
        epd_draw_grayscale_image(area, (uint8_t *) deszcz_data);
      }else if(DataHumi > 85 && DataTemp < 3.0){
        epd_draw_grayscale_image(area, (uint8_t *) snieg_data);
      }else{
        if(DataBaro >= 1020.0){
            epd_draw_grayscale_image(area, (uint8_t *) slonce_data);
        }else if (DataBaro < 1020.0 && DataBaro >= 1010.0){
            epd_draw_grayscale_image(area, (uint8_t *) slonce_chmurka_data);
        }else if(DataBaro < 1010.0){
            epd_draw_grayscale_image(area, (uint8_t *) chmurka_data);
        }
      }
    }else{
      if(DataHumi > 85 && DataTemp > 3.0){
        epd_draw_grayscale_image(area, (uint8_t *) deszcz_data);
      }else if(DataHumi > 85 && DataTemp < 3.0){
        epd_draw_grayscale_image(area, (uint8_t *) snieg_data);
      }else{
        if(DataBaro >= 1020.0){
            epd_draw_grayscale_image(area, (uint8_t *) ksiezyc_data);
        }else if (DataBaro < 1020.0 && DataBaro >= 1010.0){
            epd_draw_grayscale_image(area, (uint8_t *) ksiezyc_chmurka_data);
        }else if(DataBaro < 1010.0){
            epd_draw_grayscale_image(area, (uint8_t *) chmurka_data);
        }
      }      
    }
}

void get_json_data_temp_humi_baro(){
  String url = serverName + String(JSON_IDX_TEMP_SENSOR);
  sensorReadings = httpGETRequest((const char *)url.c_str());
  StaticJsonDocument<1800> doc;
  deserializeJson(doc, sensorReadings);   
  DataServerTime = doc["ServerTime"];
  DataSunrise = doc["Sunrise"];
  DataSunset = doc["Sunset"];
  DataTemp = doc["result"][0]["Temp"];
  DataBaro = doc["result"][0]["Barometer"];
  DataHumi = doc["result"][0]["Humidity"];
}

void connect_to_wifi(){
  char* ssid = INI_SSID;
  char* password = INI_PASS;

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to: "); 
  Serial.println(ssid);   
  uint8_t i = 0;
  while (WiFi.status() != WL_CONNECTED){
    Serial.print('.');
    delay(500);
    if ((++i % 16) == 0){
        Serial.println(F(" still trying to connect"));
    }
  }

  Serial.print(F("Connected. My IP address is: "));
  Serial.println(WiFi.localIP());
  delay(500);
}

void setup() {
  Serial.begin(115200);
  Serial.println("booting...");
  connect_to_wifi();
  Serial.println("init e-paper.");
  epd_init();
  framebuffer = (uint8_t *)ps_calloc(sizeof(uint8_t), EPD_WIDTH * EPD_HEIGHT / 2);
  if (!framebuffer) {
    Serial.println("alloc memory failed !!!");
    while (1);
  }
  memset(framebuffer, 0xFF, EPD_WIDTH * EPD_HEIGHT / 2);   
  get_json_data_temp_humi_baro();

  Serial.println("drawing.");
  epd_poweron();
  epd_clear();
  draw_frames();
  draw_bat();
  show_today_weather();
  draw_weather_icon();
  show_day();

  epd_draw_grayscale_image(epd_full_screen(), framebuffer);
  delay(5);
  epd_poweroff_all();

  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  esp_deep_sleep_start();
}

void loop() {
  // hate loops
}