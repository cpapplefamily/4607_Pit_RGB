//+--------------------------------------------------------------------------
//
// NightDriver - (c) 2018 Dave Plummer.  All Rights Reserved.
//
// Description:
//
//   Draws sample effects on a an addressable strip using FastLED
//
//---------------------------------------------------------------------------
#include "BluetoothSerial.h"

#include <Arduino.h>            // Arduino Framework
#include <U8g2lib.h>            // For text on the little on-chip OLED
#define FASTLED_INTERNAL        // Suppress build banner
#include <FastLED.h>

#define OLED_CLOCK  15          // Pins for the OLED display
#define OLED_DATA   4
#define OLED_RESET  16

#define FAN_SIZE      124        // Number of LEDs in each fan
#define NUM_FANS       1        // Number of Fans
#define LED_FAN_OFFSET 0        // How far from bottom first pixel is
#define NUM_LEDS      124       // FastLED definitions
#define LED_PIN        5
#define RGB_TOGGLE_PIN 0

CRGB g_LEDs[NUM_LEDS] = {0};    // Frame buffer for FastLED

U8G2_SSD1306_128X64_NONAME_F_HW_I2C g_OLED(U8G2_R2, OLED_RESET, OLED_CLOCK, OLED_DATA);
int g_lineHeight = 0;
int g_Brightness = 255;//15;         // 0-255 LED brightness scale
int g_PowerLimit = 50000;//900;        // 900mW Power Limit

#include "ledgfx.h"
#include "comet.h"
#include "marquee.h"
#include "twinkle.h"
#include "fire.h"

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

BluetoothSerial SerialBT;
int received;// received value will be stored in this variable
char select_Show = '4';// received value will be stored as CHAR in this variable

void setup() 
{
  

  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(RGB_TOGGLE_PIN, INPUT);


  while (!Serial) { }
  Serial.println("ESP32 Startup");

  g_OLED.begin();
  g_OLED.clear();
  g_OLED.setFont(u8g2_font_profont15_tf);
  g_lineHeight = g_OLED.getFontAscent() - g_OLED.getFontDescent();        // Descent is a negative number so we add it to the total

  FastLED.addLeds<WS2812B, LED_PIN, GRB>(g_LEDs, NUM_LEDS);               // Add our LED strip to the FastLED library
  FastLED.setBrightness(g_Brightness);
  set_max_power_indicator_LED(LED_BUILTIN);                               // Light the builtin LED if we power throttle
  FastLED.setMaxPowerInMilliWatts(g_PowerLimit);                          // Set the power limit, above which brightness will be throttled
  
  SerialBT.enableSSP();
  SerialBT.setPin("1234");
  SerialBT.begin("ESP32_RGB"); //Bluetooth device name
  Serial.println("The device started, now you can pair it with bluetooth!");
  
}

int show_Pointer = 0;

void loop() 
{
  
  
  FireEffect fire(NUM_LEDS, 50, 100, 20, NUM_LEDS, true, false);    
  boolean resetPin = false;
  while (true){

    if (!digitalRead(RGB_TOGGLE_PIN)){
      if (!resetPin){
        Serial.println("Button Pressed: ");
        Serial.println(select_Show);
        switch (select_Show)
        {
        case '1':
          select_Show = '2';
          break;
        case '2':
          select_Show = '3';
          break;
        case '3':
          select_Show = '4';
          break;
        case '4':
          select_Show = '5';
          break;
        case '5':
          select_Show = '6';
          break;
        case '6':
          select_Show = '7';
          break;
        case '7':
          select_Show = '1';
          break;
        default:
          select_Show = '1';
          break;
        }
        SerialBT.print("Current Show:");// write on BT app
        SerialBT.println(select_Show);// write on BT app 
        Serial.println(select_Show);
      }
      resetPin = true;
    }else{
      resetPin = false;
    }
    
    FastLED.clear();

    if(SerialBT.available()){
      char incomingChar =(char)SerialBT.read();
      if (incomingChar != '\n' && incomingChar != '\r'){
        select_Show = incomingChar;
        SerialBT.print("Received:");// write on BT app
        SerialBT.println(select_Show);// write on BT app 
      }
    }    

    
    

    if (select_Show == '1'){
      // Sequential Color Rainbows
      show_Pointer = 1;
      static byte basehue = 0;
      byte hue = basehue;
      basehue += 4;
      for (int i = 0; i < NUM_LEDS; i++)
        DrawFanPixels(i, 1, CHSV(hue+=16, 255, 255));
      basehue += 4;
      delay(33);
    }

    // Simple Color Cycle
    if (select_Show == '2'){
      show_Pointer = 2;
      static byte hue = 0;
      for (int i = 0; i < NUM_LEDS; i++)
        DrawFanPixels(i, 1, CHSV(hue, 255, 255));
      hue += 4;
      delay(500);
      SerialBT.println(hue);
    }

    if (select_Show == '3'){
      show_Pointer = 3;
      DrawComet();
      
    }
    if (select_Show == '4'){
      show_Pointer = 4;
      fire.DrawFire();
      delay(10);
    }

    if (select_Show == '5'){
      show_Pointer = 5;
      DrawMarquee();
      delay(10);
    }

    if (select_Show == '6'){
      show_Pointer = 6;
      DrawTwinkle();
      delay(100);
    }

    FastLED.show(g_Brightness); //  Show and delay

    EVERY_N_MILLISECONDS(250)
    {
      g_OLED.clearBuffer();
      g_OLED.setCursor(0, g_lineHeight);
      g_OLED.printf("FPS  : %u", FastLED.getFPS());
      g_OLED.setCursor(0, g_lineHeight * 2);
      g_OLED.printf("Power: %u mW", calculate_unscaled_power_mW(g_LEDs, 4));
      g_OLED.setCursor(0, g_lineHeight * 3);
      g_OLED.printf("Brite: %d", calculate_max_brightness_for_power_mW(g_Brightness, g_PowerLimit));
       g_OLED.setCursor(0, g_lineHeight * 4);
      g_OLED.printf("show_Pointer: %d", show_Pointer);
      g_OLED.sendBuffer();
    }
    
  }
}
