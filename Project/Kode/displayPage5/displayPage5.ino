// Page 5 on the tft display
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

// --- Geometry for 3x3 button grid ---
#define TITLE_X    10
#define TITLE_Y    10

#define GRID_TOP   44
#define GRID_LEFT  10
#define GRID_W     300
#define GRID_H     180

#define BTN_COLS   3
#define BTN_ROWS   3
#define BTN_H      52
#define BTN_W      94
#define BTN_SPACING 6

const char* btnLabels[BTN_ROWS * BTN_COLS][2] = {
  {"FFT for", "mic 1"},
  {"FFT for", "mic 2"},
  {"FFT for both", "mic 1 and 2"},
  {"Impulseresponse", "mic 1"},
  {"Impulseresponse", "mic 2"},
  {"Impulseresponse", "mic 1 and 2"},
  {"Previous page", ""},
  {"Live FFT", ""},
  {"Front page", ""}
};

void drawCenteredMultilineButtonText(const char* line1, const char* line2, int x, int y, int w, int h, uint16_t color, uint8_t size=1) {
  tft.setTextColor(color);
  tft.setTextSize(size);
  int text1_w = strlen(line1) * 6 * size;
  int text2_w = strlen(line2) * 6 * size;
  int text_h = 8 * size;
  int lines = (line2[0] != '\0') ? 2 : 1;

  int ty = y + (h - lines * text_h - (lines-1)*4) / 2; // vertical center, 4px between lines
  int tx1 = x + (w - text1_w) / 2;
  tft.setCursor(tx1, ty);
  tft.print(line1);

  if (lines == 2) {
    int tx2 = x + (w - text2_w) / 2;
    tft.setCursor(tx2, ty + text_h + 4);
    tft.print(line2);
  }
}

void drawPage5() {
  tft.fillScreen(ILI9341_WHITE);

  // Title
  tft.setTextSize(2);
  tft.setTextColor(ILI9341_BLACK);
  tft.setCursor(TITLE_X, TITLE_Y);
  tft.print("What should be displayed?");

  // 3x3 Button Grid
  for (int row = 0; row < BTN_ROWS; row++) {
    for (int col = 0; col < BTN_COLS; col++) {
      int idx = row * BTN_COLS + col;
      int x = GRID_LEFT + BTN_SPACING + col * (BTN_W + BTN_SPACING);
      int y = GRID_TOP + BTN_SPACING + row * (BTN_H + BTN_SPACING);

      // Button fill and outline
      tft.fillRoundRect(x, y, BTN_W, BTN_H, 10, ILI9341_LIGHTGREY);
      tft.drawRoundRect(x, y, BTN_W, BTN_H, 10, ILI9341_BLACK);

      // Multiline or single-line button text
      drawCenteredMultilineButtonText(
        btnLabels[idx][0], btnLabels[idx][1], x, y, BTN_W, BTN_H, ILI9341_BLACK, 1
      );
    }
  }
}

void setup() {
  tft.begin();
  tft.setRotation(1);
  ts.begin();
  ts.setRotation(3);

  drawPage5();
}

void loop() {
  // No touch logic yet!
}
