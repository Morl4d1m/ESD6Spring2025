// Page 2 on the tft display
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

String sampleName = "";

const char* row1[] = { "Q","W","E","R","T","Y","U","I","O","P" };
const int row1X = 30, row1Y = 72, keyW = 26, keyH = 28, keyGap = 2;
const char* row2[] = { "A","S","D","F","G","H","J","K","L" };
const int row2X = 43, row2Y = 102;
const char* row3[] = { "Z","X","C","V","B","N","M" };
const int row3X = 69, row3Y = 132;
struct SpecialKey {
  const char* label;
  int x, w;
};
// "OK" key REMOVED here
SpecialKey row4[] = {
  { "CLEAR", 30, 54 },
  { "SPACE", 88, 118 },
  { "<",    210, 54 }
};
const int row4Y = 162;

#define BOX_X 30
#define BOX_Y 30
#define BOX_W 260
#define BOX_H 32

#define TITLE_X 60
#define TITLE_Y 10

#define BOTTOM_BTN_W   158
#define BOTTOM_BTN_H    32
#define BOTTOM_BTN_R     8
#define BOTTOM_BTN_SP   2
#define BOTTOM_BTN_Y   203
#define BOTTOM_BTN1_X   1
#define BOTTOM_BTN2_X  (BOTTOM_BTN1_X + BOTTOM_BTN_W + BOTTOM_BTN_SP+5)

void drawCenteredText(const char* str, int x, int y, int w, int h, uint16_t color, uint8_t textsize=2) {
  int textlen = strlen(str);
  int text_w = textlen * 6 * textsize;
  int text_h = 8 * textsize;
  int cx = x + (w - text_w) / 2;
  int cy = y + (h - text_h) / 2;
  tft.setCursor(cx, cy);
  tft.setTextColor(color);
  tft.setTextSize(textsize);
  tft.print(str);
}

void drawSampleBox() {
  tft.fillRect(BOX_X+1, BOX_Y+1, BOX_W-2, BOX_H-2, ILI9341_WHITE); // Clear inside
  tft.drawRect(BOX_X, BOX_Y, BOX_W, BOX_H, ILI9341_BLACK);
  tft.setTextColor(ILI9341_BLACK);
  tft.setTextSize(2);
  tft.setCursor(BOX_X + 6, BOX_Y + 8);
  tft.print(sampleName);
}

void drawPage2() {
  tft.fillScreen(ILI9341_WHITE);

  tft.setTextSize(2);
  tft.setTextColor(ILI9341_BLACK);
  tft.setCursor(TITLE_X, TITLE_Y);
  tft.print("Enter a sample name");

  drawSampleBox();

  // Row 1: QWERTYUIOP
  tft.setTextSize(2);
  for (int i = 0; i < 10; i++) {
    int x = row1X + i * (keyW + keyGap);
    tft.fillRect(x, row1Y, keyW, keyH, ILI9341_WHITE);
    tft.drawRect(x, row1Y, keyW, keyH, ILI9341_BLACK);
    drawCenteredText(row1[i], x, row1Y, keyW, keyH, ILI9341_BLACK, 2);
  }

  // Row 2: ASDFGHJKL
  for (int i = 0; i < 9; i++) {
    int x = row2X + i * (keyW + keyGap);
    tft.fillRect(x, row2Y, keyW, keyH, ILI9341_WHITE);
    tft.drawRect(x, row2Y, keyW, keyH, ILI9341_BLACK);
    drawCenteredText(row2[i], x, row2Y, keyW, keyH, ILI9341_BLACK, 2);
  }

  // Row 3: ZXCVBNM
  for (int i = 0; i < 7; i++) {
    int x = row3X + i * (keyW + keyGap);
    tft.fillRect(x, row3Y, keyW, keyH, ILI9341_WHITE);
    tft.drawRect(x, row3Y, keyW, keyH, ILI9341_BLACK);
    drawCenteredText(row3[i], x, row3Y, keyW, keyH, ILI9341_BLACK, 2);
  }

  // Row 4: Specials (no OK button)
  for (int i = 0; i < 3; i++) {
    int x = row4[i].x;
    int w = row4[i].w;
    tft.fillRect(x, row4Y, w, keyH, ILI9341_WHITE);
    tft.drawRect(x, row4Y, w, keyH, ILI9341_BLACK);
    uint8_t keyTextSize = (strcmp(row4[i].label, "SPACE")==0 || strcmp(row4[i].label, "CLEAR")==0) ? 1 : 2;
    drawCenteredText(row4[i].label, x, row4Y, w, keyH, ILI9341_BLACK, keyTextSize);
  }

  // Bottom Buttons
  tft.fillRoundRect(BOTTOM_BTN1_X, BOTTOM_BTN_Y, BOTTOM_BTN_W+5, BOTTOM_BTN_H, BOTTOM_BTN_R, ILI9341_DARKGREY);
  tft.drawRoundRect(BOTTOM_BTN1_X, BOTTOM_BTN_Y, BOTTOM_BTN_W+5, BOTTOM_BTN_H, BOTTOM_BTN_R, ILI9341_BLACK);
  drawCenteredText("Previous page", BOTTOM_BTN1_X, BOTTOM_BTN_Y, BOTTOM_BTN_W+5, BOTTOM_BTN_H, ILI9341_BLACK, 2);

  tft.fillRoundRect(BOTTOM_BTN2_X, BOTTOM_BTN_Y, BOTTOM_BTN_W-5, BOTTOM_BTN_H, BOTTOM_BTN_R, ILI9341_BLUE);
  tft.drawRoundRect(BOTTOM_BTN2_X, BOTTOM_BTN_Y, BOTTOM_BTN_W-5, BOTTOM_BTN_H, BOTTOM_BTN_R, ILI9341_BLACK);
  drawCenteredText("Perform test", BOTTOM_BTN2_X, BOTTOM_BTN_Y, BOTTOM_BTN_W-5, BOTTOM_BTN_H, ILI9341_WHITE, 2);
}

bool inRect(int x, int y, int rx, int ry, int rw, int rh) {
  return (x >= rx) && (x < rx + rw) && (y >= ry) && (y < ry + rh);
}

void handleTouch(int tx, int ty) {
  // Keyboard rows
  for (int i = 0; i < 10; i++) {
    int x = row1X + i * (keyW + keyGap);
    if (inRect(tx, ty, x, row1Y, keyW, keyH)) {
      if (sampleName.length() < 20) {
        sampleName += row1[i];
        drawSampleBox();
      }
      return;
    }
  }
  for (int i = 0; i < 9; i++) {
    int x = row2X + i * (keyW + keyGap);
    if (inRect(tx, ty, x, row2Y, keyW, keyH)) {
      if (sampleName.length() < 20) {
        sampleName += row2[i];
        drawSampleBox();
      }
      return;
    }
  }
  for (int i = 0; i < 7; i++) {
    int x = row3X + i * (keyW + keyGap);
    if (inRect(tx, ty, x, row3Y, keyW, keyH)) {
      if (sampleName.length() < 20) {
        sampleName += row3[i];
        drawSampleBox();
      }
      return;
    }
  }
  // Row 4: Specials
  // CLEAR
  if (inRect(tx, ty, row4[0].x, row4Y, row4[0].w, keyH)) {
    sampleName = "";
    drawSampleBox();
    return;
  }
  // SPACE
  if (inRect(tx, ty, row4[1].x, row4Y, row4[1].w, keyH)) {
    if (sampleName.length() < 20) {
      sampleName += " ";
      drawSampleBox();
    }
    return;
  }
  // BACKSPACE
  if (inRect(tx, ty, row4[2].x, row4Y, row4[2].w, keyH)) {
    if (sampleName.length() > 0) {
      sampleName.remove(sampleName.length() - 1, 1);
      drawSampleBox();
    }
    return;
  }
  // Bottom buttons
  if (inRect(tx, ty, BOTTOM_BTN1_X, BOTTOM_BTN_Y, BOTTOM_BTN_W+5, BOTTOM_BTN_H)) {
    // Previous page
    // (Insert action here)
    return;
  }
  if (inRect(tx, ty, BOTTOM_BTN2_X, BOTTOM_BTN_Y, BOTTOM_BTN_W-5, BOTTOM_BTN_H)) {
    // Perform test
    // (Insert action here)
    return;
  }
}

void setup() {
  tft.begin();
  tft.setRotation(1);
  ts.begin();
  ts.setRotation(3);
  drawPage2();
}

void loop() {
  if (ts.touched()) {
    TS_Point p = ts.getPoint();
    int tx = map(p.x, 0, 4095, 0, tft.width());
    int ty = map(p.y, 0, 4095, 0, tft.height());
    handleTouch(tx, ty);
    delay(150); // debounce
  }
}
