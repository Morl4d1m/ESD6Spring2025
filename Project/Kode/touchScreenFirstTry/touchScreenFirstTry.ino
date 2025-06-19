#include <SPI.h>
#include <ILI9341_t3.h>
#include <XPT2046_Touchscreen.h>

// Teensy pins
#define TFT_CS 10
#define TFT_DC 9
#define TFT_RST 255  // 255 if not used

#define TOUCH_CS 8
#define TOUCH_IRQ 255 // 255 if unused

ILI9341_t3 tft = ILI9341_t3(TFT_CS, TFT_DC, TFT_RST);
XPT2046_Touchscreen ts(TOUCH_CS, TOUCH_IRQ);

void setup() {
  Serial.begin(115200);
  tft.begin();
  tft.setRotation(1);
  ts.begin();
  ts.setRotation(3);

  tft.fillScreen(ILI9341_BLACK);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);
  tft.setCursor(40, 120);
  tft.print("Touch the screen!");
}

void loop() {
  if (ts.touched()) {
    TS_Point p = ts.getPoint();
    Serial.print("Touch: x = "); Serial.print(p.x);
    Serial.print(", y = "); Serial.println(p.y);

    // Simple visual feedback
    tft.fillCircle(map(p.x, 0, 4095, 0, tft.width()), map(p.y, 0, 4095, 0, tft.height()), 4, ILI9341_GREEN);
    delay(150);
  }
}
