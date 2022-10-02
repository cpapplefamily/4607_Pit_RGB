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
char select_Show = '1';// received value will be stored as CHAR in this variable

CRGB color2 = HeatColor(100);

void setup() 
{
  

  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);


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
  
  SerialBT.begin("ESP32_RGB"); //Bluetooth device name
  Serial.println("The device started, now you can pair it with bluetooth!");
}

void DrawMarqueeComparison()
{
  static float scroll = 0.0f;
  scroll += 0.1f;
  if (scroll > 5.0)
    scroll -= 5.0;

  for (float i = scroll; i < NUM_LEDS/2 -1; i+= 5)
  {
    DrawPixels(i, 3, CRGB::Green);
    DrawPixels(NUM_LEDS-1-(int)i, 3, CRGB::Red);
  }
}
int show_Pointer = 0;

void loop() 
{
  bool bLED = 0;
  
  FireEffect fire(NUM_LEDS, 20, 100, 3, NUM_LEDS, true, false);    

  while (true){
    FastLED.clear();

    if(SerialBT.available()){
      char incomingChar =(char)SerialBT.read();
      if (incomingChar != '\n' && incomingChar != '\r'){
        select_Show = incomingChar;
        SerialBT.print("Received:");// write on BT app
        SerialBT.println(select_Show);// write on BT app 
      }
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
      delay(10);
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
