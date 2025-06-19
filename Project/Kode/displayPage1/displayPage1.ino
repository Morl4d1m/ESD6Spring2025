// Page 1 on the tft display
#include <SPI.h>
#include <ILI9341_t3.h>
#include <XPT2046_Touchscreen.h>

#define TFT_CS   10
#define TFT_DC   9
#define TFT_RST  255
#define TOUCH_CS 8
#define TOUCH_IRQ 255

ILI9341_t3 tft = ILI9341_t3(TFT_CS, TFT_DC, TFT_RST);
XPT2046_Touchscreen ts(TOUCH_CS, TOUCH_IRQ);

// ---- Adjusted Layout ----
#define TITLE_Y 10

#define BUTTON_Y     34   // Moved up from 70
#define BUTTON_W     120
#define BUTTON_H     54   // Slightly taller for balance
#define BUTTON_SPAC  24
#define BUTTON_R     10
#define BUTTON_LEFT_X 32
#define BUTTON_RIGHT_X (BUTTON_LEFT_X + BUTTON_W + BUTTON_SPAC)

#define TEXTBOX_Y   100  // Moved up
#define TEXTBOX_H   138   // Taller box
#define TEXTBOX_X   2
#define TEXTBOX_W   316
#define TEXTBOX_R   8

void drawPage1() {
  tft.fillScreen(ILI9341_WHITE);

  // --- Title (smaller and higher) ---
  tft.setTextColor(ILI9341_BLACK);
  tft.setTextSize(2); // Smaller
  tft.setCursor(5, TITLE_Y); // Centered for 320px width, adjust as needed
  tft.print("What would you like to do?");

  // --- Left Button: Perform a new test (smaller text, higher button) ---
  tft.fillRoundRect(BUTTON_LEFT_X, BUTTON_Y, BUTTON_W, BUTTON_H, BUTTON_R, ILI9341_BLUE);
  tft.drawRoundRect(BUTTON_LEFT_X, BUTTON_Y, BUTTON_W, BUTTON_H, BUTTON_R, ILI9341_BLACK);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(1.5); // Smaller text
  tft.setCursor(BUTTON_LEFT_X + 14, BUTTON_Y + 22);
  tft.print("Perform a new");
  tft.setCursor(BUTTON_LEFT_X + 28, BUTTON_Y + 36);
  tft.print("test");

  // --- Right Button: Read data from a previous test (smaller text, higher button) ---
  tft.fillRoundRect(BUTTON_RIGHT_X, BUTTON_Y, BUTTON_W, BUTTON_H, BUTTON_R, ILI9341_DARKGREY);
  tft.drawRoundRect(BUTTON_RIGHT_X, BUTTON_Y, BUTTON_W, BUTTON_H, BUTTON_R, ILI9341_BLACK);
  tft.setTextColor(ILI9341_BLACK);
  tft.setTextSize(1.5); // Smaller text
  tft.setCursor(BUTTON_RIGHT_X + 8, BUTTON_Y + 18);
  tft.print("Read data from a");
  tft.setCursor(BUTTON_RIGHT_X + 6, BUTTON_Y + 32);
  tft.print("previous test");

  // --- Bottom Textbox (taller) ---
  tft.drawRoundRect(TEXTBOX_X, TEXTBOX_Y, TEXTBOX_W, TEXTBOX_H, TEXTBOX_R, ILI9341_BLACK);
  tft.setTextColor(ILI9341_BLACK);
  tft.setTextSize(1);

  // Multiline info text:
  int text_y = TEXTBOX_Y + 6;
  tft.setCursor(TEXTBOX_X + 6, text_y);
  tft.print("The impedance tube itself has been developed in");
  text_y += 13;
  tft.setCursor(TEXTBOX_X + 6, text_y);
  tft.print("accordance with the DS/ISO 10534-2:2023 standard.");
  text_y += 13;
  tft.setCursor(TEXTBOX_X + 6, text_y);
  tft.print("Algorithms up until the transfer matrix method has");
  text_y += 13;
  tft.setCursor(TEXTBOX_X + 6, text_y);
  tft.print("been succesfully implemented directly on the Teensy");
  text_y += 13;
  tft.setCursor(TEXTBOX_X + 6, text_y);
  tft.print("4.1 with audio shield attached. The report found");
  text_y += 13;
  tft.setCursor(TEXTBOX_X + 6, text_y);
  tft.print("adjacent to the tube describes the development of");
  text_y += 13;
  tft.setCursor(TEXTBOX_X + 6, text_y);
  tft.print("the impedancetube and provides links for programs,");
  text_y += 13;
  tft.setCursor(TEXTBOX_X + 6, text_y);
  tft.print("algorithm etc. The transfer matrix method has been");
  text_y += 13;
  tft.setCursor(TEXTBOX_X + 6, text_y);
  tft.print("implemented in MATLAB only, due to memory");
  text_y += 13;
  tft.setCursor(TEXTBOX_X + 6, text_y);
  tft.print("constraints of the Teensy.");
}

void setup() {
  tft.begin();
  tft.setRotation(1); // 320x240 landscape
  ts.begin();
  ts.setRotation(3);
  drawPage1();
}

void loop() {
  // Touch handling to be added after layout is approved
}
