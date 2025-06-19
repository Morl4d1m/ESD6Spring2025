#include <SPI.h>
#include <ILI9341_t3.h>
#include <XPT2046_Touchscreen.h>
#include <SD.h>
#include <math.h>

// ---- Hardware pinout ----
#define TFT_CS 10
#define TFT_DC 9
#define TFT_RST 255
#define TOUCH_CS 8
#define TOUCH_IRQ 255
#define SD_CS BUILTIN_SDCARD

ILI9341_t3 tft = ILI9341_t3(TFT_CS, TFT_DC, TFT_RST);
XPT2046_Touchscreen ts(TOUCH_CS, TOUCH_IRQ);

// ---- Page Management ----
enum PageNum { PAGE_1 = 1,
               PAGE_2,
               PAGE_3,
               PAGE_4,
               PAGE_5,
               PAGE_PLOT };
PageNum currentPage = PAGE_1;

String lastSelectedFile = "";  // Global variable for selected file


// ---------- Page 1 ----------
#define P1_TITLE_Y 10
#define P1_BUTTON_Y 34
#define P1_BUTTON_W 120
#define P1_BUTTON_H 54
#define P1_BUTTON_SPAC 24
#define P1_BUTTON_R 10
#define P1_BUTTON_LEFT_X 32
#define P1_BUTTON_RIGHT_X (P1_BUTTON_LEFT_X + P1_BUTTON_W + P1_BUTTON_SPAC)
#define P1_TEXTBOX_Y 100
#define P1_TEXTBOX_H 138
#define P1_TEXTBOX_X 2
#define P1_TEXTBOX_W 316
#define P1_TEXTBOX_R 8

void drawPage1() {
  tft.fillScreen(ILI9341_WHITE);

  // --- Title ---
  tft.setTextColor(ILI9341_BLACK);
  tft.setTextSize(2);
  tft.setCursor(5, P1_TITLE_Y);
  tft.print("What would you like to do?");

  // --- Left Button ---
  tft.fillRoundRect(P1_BUTTON_LEFT_X, P1_BUTTON_Y, P1_BUTTON_W, P1_BUTTON_H, P1_BUTTON_R, ILI9341_BLUE);
  tft.drawRoundRect(P1_BUTTON_LEFT_X, P1_BUTTON_Y, P1_BUTTON_W, P1_BUTTON_H, P1_BUTTON_R, ILI9341_BLACK);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(1.5);
  tft.setCursor(P1_BUTTON_LEFT_X + 14, P1_BUTTON_Y + 22);
  tft.print("Perform a new");
  tft.setCursor(P1_BUTTON_LEFT_X + 28, P1_BUTTON_Y + 36);
  tft.print("test");

  // --- Right Button ---
  tft.fillRoundRect(P1_BUTTON_RIGHT_X, P1_BUTTON_Y, P1_BUTTON_W, P1_BUTTON_H, P1_BUTTON_R, ILI9341_DARKGREY);
  tft.drawRoundRect(P1_BUTTON_RIGHT_X, P1_BUTTON_Y, P1_BUTTON_W, P1_BUTTON_H, P1_BUTTON_R, ILI9341_BLACK);
  tft.setTextColor(ILI9341_BLACK);
  tft.setTextSize(1.5);
  tft.setCursor(P1_BUTTON_RIGHT_X + 8, P1_BUTTON_Y + 18);
  tft.print("Read data from a");
  tft.setCursor(P1_BUTTON_RIGHT_X + 6, P1_BUTTON_Y + 32);
  tft.print("previous test");

  // --- Bottom Textbox ---
  tft.drawRoundRect(P1_TEXTBOX_X, P1_TEXTBOX_Y, P1_TEXTBOX_W, P1_TEXTBOX_H, P1_TEXTBOX_R, ILI9341_BLACK);
  tft.setTextColor(ILI9341_BLACK);
  tft.setTextSize(1);
  int text_y = P1_TEXTBOX_Y + 6;
  tft.setCursor(P1_TEXTBOX_X + 6, text_y);
  tft.print("The impedance tube itself has been developed in");
  text_y += 13;
  tft.setCursor(P1_TEXTBOX_X + 6, text_y);
  tft.print("accordance with the DS/ISO 10534-2:2023 standard.");
  text_y += 13;
  tft.setCursor(P1_TEXTBOX_X + 6, text_y);
  tft.print("Algorithms up until the transfer matrix method has");
  text_y += 13;
  tft.setCursor(P1_TEXTBOX_X + 6, text_y);
  tft.print("been succesfully implemented directly on the Teensy");
  text_y += 13;
  tft.setCursor(P1_TEXTBOX_X + 6, text_y);
  tft.print("4.1 with audio shield attached. The report found");
  text_y += 13;
  tft.setCursor(P1_TEXTBOX_X + 6, text_y);
  tft.print("adjacent to the tube describes the development of");
  text_y += 13;
  tft.setCursor(P1_TEXTBOX_X + 6, text_y);
  tft.print("the impedancetube and provides links for programs,");
  text_y += 13;
  tft.setCursor(P1_TEXTBOX_X + 6, text_y);
  tft.print("algorithm etc. The transfer matrix method has been");
  text_y += 13;
  tft.setCursor(P1_TEXTBOX_X + 6, text_y);
  tft.print("implemented in MATLAB only, due to memory");
  text_y += 13;
  tft.setCursor(P1_TEXTBOX_X + 6, text_y);
  tft.print("constraints of the Teensy.");
}

// ---------- Page 2 ----------
String p2_sampleName = "";
const char* p2_row1[] = { "Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P" };
const int p2_row1X = 30, p2_row1Y = 72, p2_keyW = 26, p2_keyH = 28, p2_keyGap = 2;
const char* p2_row2[] = { "A", "S", "D", "F", "G", "H", "J", "K", "L" };
const int p2_row2X = 43, p2_row2Y = 102;
const char* p2_row3[] = { "Z", "X", "C", "V", "B", "N", "M" };
const int p2_row3X = 69, p2_row3Y = 132;
struct P2SpecialKey {
  const char* label;
  int x, w;
};
P2SpecialKey p2_row4[] = {
  { "CLEAR", 30, 54 },
  { "SPACE", 88, 118 },
  { "<", 210, 54 }
};
const int p2_row4Y = 162;

#define P2_BOX_X 30
#define P2_BOX_Y 30
#define P2_BOX_W 260
#define P2_BOX_H 32
#define P2_TITLE_X 60
#define P2_TITLE_Y 10
#define P2_BOTTOM_BTN_W 158
#define P2_BOTTOM_BTN_H 32
#define P2_BOTTOM_BTN_R 8
#define P2_BOTTOM_BTN_SP 2
#define P2_BOTTOM_BTN_Y 203
#define P2_BOTTOM_BTN1_X 1
#define P2_BOTTOM_BTN2_X (P2_BOTTOM_BTN1_X + P2_BOTTOM_BTN_W + P2_BOTTOM_BTN_SP + 5)

void p2_drawCenteredText(const char* str, int x, int y, int w, int h, uint16_t color, uint8_t textsize = 2) {
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

void p2_drawSampleBox() {
  tft.fillRect(P2_BOX_X + 1, P2_BOX_Y + 1, P2_BOX_W - 2, P2_BOX_H - 2, ILI9341_WHITE);
  tft.drawRect(P2_BOX_X, P2_BOX_Y, P2_BOX_W, P2_BOX_H, ILI9341_BLACK);
  tft.setTextColor(ILI9341_BLACK);
  tft.setTextSize(2);
  tft.setCursor(P2_BOX_X + 6, P2_BOX_Y + 8);
  tft.print(p2_sampleName);
}

void drawPage2() {
  tft.fillScreen(ILI9341_WHITE);
  tft.setTextSize(2);
  tft.setTextColor(ILI9341_BLACK);
  tft.setCursor(P2_TITLE_X, P2_TITLE_Y);
  tft.print("Enter a sample name");
  p2_drawSampleBox();

  tft.setTextSize(2);
  for (int i = 0; i < 10; i++) {
    int x = p2_row1X + i * (p2_keyW + p2_keyGap);
    tft.fillRect(x, p2_row1Y, p2_keyW, p2_keyH, ILI9341_WHITE);
    tft.drawRect(x, p2_row1Y, p2_keyW, p2_keyH, ILI9341_BLACK);
    p2_drawCenteredText(p2_row1[i], x, p2_row1Y, p2_keyW, p2_keyH, ILI9341_BLACK, 2);
  }
  for (int i = 0; i < 9; i++) {
    int x = p2_row2X + i * (p2_keyW + p2_keyGap);
    tft.fillRect(x, p2_row2Y, p2_keyW, p2_keyH, ILI9341_WHITE);
    tft.drawRect(x, p2_row2Y, p2_keyW, p2_keyH, ILI9341_BLACK);
    p2_drawCenteredText(p2_row2[i], x, p2_row2Y, p2_keyW, p2_keyH, ILI9341_BLACK, 2);
  }
  for (int i = 0; i < 7; i++) {
    int x = p2_row3X + i * (p2_keyW + p2_keyGap);
    tft.fillRect(x, p2_row3Y, p2_keyW, p2_keyH, ILI9341_WHITE);
    tft.drawRect(x, p2_row3Y, p2_keyW, p2_keyH, ILI9341_BLACK);
    p2_drawCenteredText(p2_row3[i], x, p2_row3Y, p2_keyW, p2_keyH, ILI9341_BLACK, 2);
  }
  for (int i = 0; i < 3; i++) {
    int x = p2_row4[i].x;
    int w = p2_row4[i].w;
    tft.fillRect(x, p2_row4Y, w, p2_keyH, ILI9341_WHITE);
    tft.drawRect(x, p2_row4Y, w, p2_keyH, ILI9341_BLACK);
    uint8_t keyTextSize = (strcmp(p2_row4[i].label, "SPACE") == 0 || strcmp(p2_row4[i].label, "CLEAR") == 0) ? 1 : 2;
    p2_drawCenteredText(p2_row4[i].label, x, p2_row4Y, w, p2_keyH, ILI9341_BLACK, keyTextSize);
  }
  // Bottom Buttons
  tft.fillRoundRect(P2_BOTTOM_BTN1_X, P2_BOTTOM_BTN_Y, P2_BOTTOM_BTN_W + 5, P2_BOTTOM_BTN_H, P2_BOTTOM_BTN_R, ILI9341_DARKGREY);
  tft.drawRoundRect(P2_BOTTOM_BTN1_X, P2_BOTTOM_BTN_Y, P2_BOTTOM_BTN_W + 5, P2_BOTTOM_BTN_H, P2_BOTTOM_BTN_R, ILI9341_BLACK);
  p2_drawCenteredText("Previous page", P2_BOTTOM_BTN1_X, P2_BOTTOM_BTN_Y, P2_BOTTOM_BTN_W + 5, P2_BOTTOM_BTN_H, ILI9341_BLACK, 2);
  tft.fillRoundRect(P2_BOTTOM_BTN2_X, P2_BOTTOM_BTN_Y, P2_BOTTOM_BTN_W - 5, P2_BOTTOM_BTN_H, P2_BOTTOM_BTN_R, ILI9341_BLUE);
  tft.drawRoundRect(P2_BOTTOM_BTN2_X, P2_BOTTOM_BTN_Y, P2_BOTTOM_BTN_W - 5, P2_BOTTOM_BTN_H, P2_BOTTOM_BTN_R, ILI9341_BLACK);
  p2_drawCenteredText("Perform test", P2_BOTTOM_BTN2_X, P2_BOTTOM_BTN_Y, P2_BOTTOM_BTN_W - 5, P2_BOTTOM_BTN_H, ILI9341_WHITE, 2);
}

bool p2_inRect(int x, int y, int rx, int ry, int rw, int rh) {
  return (x >= rx) && (x < rx + rw) && (y >= ry) && (y < ry + rh);
}

void p2_handleTouch(int tx, int ty) {
  // Keyboard rows
  for (int i = 0; i < 10; i++) {
    int x = p2_row1X + i * (p2_keyW + p2_keyGap);
    if (p2_inRect(tx, ty, x, p2_row1Y, p2_keyW, p2_keyH)) {
      if (p2_sampleName.length() < 20) {
        p2_sampleName += p2_row1[i];
        p2_drawSampleBox();
      }
      return;
    }
  }
  for (int i = 0; i < 9; i++) {
    int x = p2_row2X + i * (p2_keyW + p2_keyGap);
    if (p2_inRect(tx, ty, x, p2_row2Y, p2_keyW, p2_keyH)) {
      if (p2_sampleName.length() < 20) {
        p2_sampleName += p2_row2[i];
        p2_drawSampleBox();
      }
      return;
    }
  }
  for (int i = 0; i < 7; i++) {
    int x = p2_row3X + i * (p2_keyW + p2_keyGap);
    if (p2_inRect(tx, ty, x, p2_row3Y, p2_keyW, p2_keyH)) {
      if (p2_sampleName.length() < 20) {
        p2_sampleName += p2_row3[i];
        p2_drawSampleBox();
      }
      return;
    }
  }
  // Row 4: Specials
  // CLEAR
  if (p2_inRect(tx, ty, p2_row4[0].x, p2_row4Y, p2_row4[0].w, p2_keyH)) {
    p2_sampleName = "";
    p2_drawSampleBox();
    return;
  }
  // SPACE
  if (p2_inRect(tx, ty, p2_row4[1].x, p2_row4Y, p2_row4[1].w, p2_keyH)) {
    if (p2_sampleName.length() < 20) {
      p2_sampleName += " ";
      p2_drawSampleBox();
    }
    return;
  }
  // BACKSPACE
  if (p2_inRect(tx, ty, p2_row4[2].x, p2_row4Y, p2_row4[2].w, p2_keyH)) {
    if (p2_sampleName.length() > 0) {
      p2_sampleName.remove(p2_sampleName.length() - 1, 1);
      p2_drawSampleBox();
    }
    return;
  }
  // Bottom buttons
  if (p2_inRect(tx, ty, P2_BOTTOM_BTN1_X, P2_BOTTOM_BTN_Y, P2_BOTTOM_BTN_W + 5, P2_BOTTOM_BTN_H)) {
    currentPage = PAGE_1;
    drawPage1();
    return;
  }
  if (p2_inRect(tx, ty, P2_BOTTOM_BTN2_X, P2_BOTTOM_BTN_Y, P2_BOTTOM_BTN_W - 5, P2_BOTTOM_BTN_H)) {
    currentPage = PAGE_3;
    drawPage3();
    return;
  }
}

// ---------- Page 3 ----------
#define P3_TITLE "Currently testing..."
#define P3_TITLE_X 50
#define P3_TITLE_Y 10
#define P3_BOX_X 16
#define P3_BOX_Y 40
#define P3_BOX_W 288
#define P3_BOX_H 187
#define P3_BOX_R 8
#define P3_FONT_SIZE 1
#define P3_LOG_LINES 15

String p3_logBuffer[P3_LOG_LINES];
int p3_logIndex = 0;

void drawPage3Box() {
  tft.fillRect(P3_BOX_X + 1, P3_BOX_Y + 1, P3_BOX_W - 2, P3_BOX_H - 2, ILI9341_WHITE);
  tft.drawRoundRect(P3_BOX_X, P3_BOX_Y, P3_BOX_W, P3_BOX_H, P3_BOX_R, ILI9341_BLACK);
  tft.setTextColor(ILI9341_BLACK);
  tft.setTextSize(P3_FONT_SIZE);
  int lineHeight = 8 * P3_FONT_SIZE + 4;
  int y = P3_BOX_Y + 5;
  for (int i = 0; i < P3_LOG_LINES; i++) {
    int idx = (p3_logIndex + i) % P3_LOG_LINES;
    tft.setCursor(P3_BOX_X + 8, y + i * lineHeight);
    tft.print(p3_logBuffer[idx]);
  }
}

void drawPage3() {
  tft.fillScreen(ILI9341_WHITE);
  tft.setTextSize(2);
  tft.setTextColor(ILI9341_BLACK);
  tft.setCursor(P3_TITLE_X, P3_TITLE_Y);
  tft.print(P3_TITLE);
  drawPage3Box();
}

void p3_addToLog(String line) {
  p3_logBuffer[p3_logIndex] = line;
  p3_logIndex = (p3_logIndex + 1) % P3_LOG_LINES;
  drawPage3Box();
  Serial.println(line);
}

// ---------- Page 4 ----------
#define P4_TITLE_X 60
#define P4_TITLE_Y 10
#define P4_LIST_X 20
#define P4_LIST_Y 40
#define P4_LIST_W 280
#define P4_LIST_H 120
#define P4_LIST_R 10
#define P4_LIST_LINES 6
#define P4_LINE_H 20
#define P4_NAV_BTN_W 44
#define P4_NAV_BTN_H 28
#define P4_NAV_BTN_Y (P4_LIST_Y + P4_LIST_H + 4)
#define P4_UP_BTN_X (P4_LIST_X + P4_LIST_W / 2 - P4_NAV_BTN_W - 6)
#define P4_DN_BTN_X (P4_LIST_X + P4_LIST_W / 2 + 6)
#define P4_BOTTOM_BTN_W 158
#define P4_BOTTOM_BTN_H 32
#define P4_BOTTOM_BTN_R 8
#define P4_BOTTOM_BTN_SP 2
#define P4_BOTTOM_BTN_Y 200
#define P4_BOTTOM_BTN1_X 1
#define P4_BOTTOM_BTN2_X (P4_BOTTOM_BTN1_X + P4_BOTTOM_BTN_W + P4_BOTTOM_BTN_SP + 5)
String p4_filePage[P4_LIST_LINES];
unsigned long p4_totalFiles = 0;
unsigned long p4_pageOffset = 0;
int p4_selectedLine = -1;

void p4_drawCenteredText(const char* str, int x, int y, int w, int h, uint16_t color, uint8_t textsize = 2) {
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

unsigned long p4_countSDfiles() {
  unsigned long count = 0;
  File root = SD.open("/");
  File entry = root.openNextFile();
  while (entry) {
    if (!entry.isDirectory()) {
      String name = entry.name();
      if (name.endsWith(".TXT") || name.endsWith(".txt") || name.endsWith(".CSV") || name.endsWith(".csv")) {
        count++;
      }
    }
    entry.close();
    entry = root.openNextFile();
  }
  root.close();
  return count;
}

void p4_loadFilePage(unsigned long startIdx) {
  File root = SD.open("/");
  File entry = root.openNextFile();
  unsigned long idx = 0;
  int pagePos = 0;
  for (int i = 0; i < P4_LIST_LINES; i++) p4_filePage[i] = "";
  while (entry && pagePos < P4_LIST_LINES) {
    if (!entry.isDirectory()) {
      String name = entry.name();
      if (name.endsWith("IRAndFFT.TXT") || name.endsWith("IRAndFFT.txt") || name.endsWith("IRAndFFT.CSV") || name.endsWith("IRAndFFT.csv")) {
        if (idx >= startIdx) {
          p4_filePage[pagePos++] = name;
        }
        idx++;
      }
    }
    entry.close();
    entry = root.openNextFile();
  }
  root.close();
}

void p4_drawFileList() {
  tft.fillRect(P4_LIST_X + 1, P4_LIST_Y + 1, P4_LIST_W - 2, P4_LIST_H - 2, ILI9341_WHITE);
  tft.drawRoundRect(P4_LIST_X, P4_LIST_Y, P4_LIST_W, P4_LIST_H, P4_LIST_R, ILI9341_BLACK);
  tft.setTextSize(1);
  for (int i = 0; i < P4_LIST_LINES; i++) {
    if (p4_filePage[i].length() == 0) continue;
    int y = P4_LIST_Y + 4 + i * P4_LINE_H;
    if (i == p4_selectedLine) {
      tft.fillRect(P4_LIST_X + 2, y - 2, P4_LIST_W - 4, P4_LINE_H, ILI9341_BLUE);
      tft.setTextColor(ILI9341_WHITE);
    } else {
      tft.setTextColor(ILI9341_BLACK);
    }
    tft.setCursor(P4_LIST_X + 10, y);
    tft.print(p4_filePage[i]);
  }
  tft.fillRect(P4_UP_BTN_X, P4_NAV_BTN_Y, P4_NAV_BTN_W, P4_NAV_BTN_H, (p4_pageOffset > 0) ? ILI9341_DARKGREY : ILI9341_LIGHTGREY);
  tft.drawRect(P4_UP_BTN_X, P4_NAV_BTN_Y, P4_NAV_BTN_W, P4_NAV_BTN_H, ILI9341_BLACK);
  p4_drawCenteredText("^", P4_UP_BTN_X, P4_NAV_BTN_Y, P4_NAV_BTN_W, P4_NAV_BTN_H, ILI9341_BLACK, 2);
  tft.fillRect(P4_DN_BTN_X, P4_NAV_BTN_Y, P4_NAV_BTN_W, P4_NAV_BTN_H,
               (p4_pageOffset + P4_LIST_LINES < p4_totalFiles) ? ILI9341_DARKGREY : ILI9341_LIGHTGREY);
  tft.drawRect(P4_DN_BTN_X, P4_NAV_BTN_Y, P4_NAV_BTN_W, P4_NAV_BTN_H, ILI9341_BLACK);
  p4_drawCenteredText("v", P4_DN_BTN_X, P4_NAV_BTN_Y, P4_NAV_BTN_W, P4_NAV_BTN_H, ILI9341_BLACK, 2);
}

void drawPage4() {
  tft.fillScreen(ILI9341_WHITE);
  tft.setTextSize(2);
  tft.setTextColor(ILI9341_BLACK);
  tft.setCursor(P4_TITLE_X, P4_TITLE_Y);
  tft.print("Select file");
  p4_drawFileList();
  tft.fillRoundRect(P4_BOTTOM_BTN1_X, P4_BOTTOM_BTN_Y, P4_BOTTOM_BTN_W + 5, P4_BOTTOM_BTN_H, P4_BOTTOM_BTN_R, ILI9341_DARKGREY);
  tft.drawRoundRect(P4_BOTTOM_BTN1_X, P4_BOTTOM_BTN_Y, P4_BOTTOM_BTN_W + 5, P4_BOTTOM_BTN_H, P4_BOTTOM_BTN_R, ILI9341_BLACK);
  p4_drawCenteredText("Previous page", P4_BOTTOM_BTN1_X, P4_BOTTOM_BTN_Y, P4_BOTTOM_BTN_W + 5, P4_BOTTOM_BTN_H, ILI9341_BLACK, 2);
  tft.fillRoundRect(P4_BOTTOM_BTN2_X, P4_BOTTOM_BTN_Y, P4_BOTTOM_BTN_W - 5, P4_BOTTOM_BTN_H, P4_BOTTOM_BTN_R, ILI9341_BLUE);
  tft.drawRoundRect(P4_BOTTOM_BTN2_X, P4_BOTTOM_BTN_Y, P4_BOTTOM_BTN_W - 5, P4_BOTTOM_BTN_H, P4_BOTTOM_BTN_R, ILI9341_BLACK);
  p4_drawCenteredText("Read data", P4_BOTTOM_BTN2_X, P4_BOTTOM_BTN_Y, P4_BOTTOM_BTN_W - 5, P4_BOTTOM_BTN_H, ILI9341_WHITE, 2);
}

bool p4_inRect(int x, int y, int rx, int ry, int rw, int rh) {
  return (x >= rx) && (x < rx + rw) && (y >= ry) && (y < ry + rh);
}

void p4_handleTouch(int tx, int ty) {
  if (p4_pageOffset > 0 && p4_inRect(tx, ty, P4_UP_BTN_X, P4_NAV_BTN_Y, P4_NAV_BTN_W, P4_NAV_BTN_H)) {
    p4_pageOffset -= P4_LIST_LINES;
    if (p4_pageOffset < 0) p4_pageOffset = 0;
    p4_selectedLine = -1;
    p4_loadFilePage(p4_pageOffset);
    p4_drawFileList();
    return;
  }
  if (p4_pageOffset + P4_LIST_LINES < p4_totalFiles && p4_inRect(tx, ty, P4_DN_BTN_X, P4_NAV_BTN_Y, P4_NAV_BTN_W, P4_NAV_BTN_H)) {
    p4_pageOffset += P4_LIST_LINES;
    p4_selectedLine = -1;
    p4_loadFilePage(p4_pageOffset);
    p4_drawFileList();
    return;
  }
  for (int i = 0; i < P4_LIST_LINES; i++) {
    int y = P4_LIST_Y + 4 + i * P4_LINE_H;
    if (p4_filePage[i].length() > 0 && p4_inRect(tx, ty, P4_LIST_X, y - 2, P4_LIST_W, P4_LINE_H)) {
      p4_selectedLine = i;
      p4_drawFileList();
      return;
    }
  }
  if (p4_inRect(tx, ty, P4_BOTTOM_BTN1_X, P4_BOTTOM_BTN_Y, P4_BOTTOM_BTN_W, P4_BOTTOM_BTN_H)) {
    currentPage = PAGE_1;
    drawPage1();
    return;
  }
  if (p4_inRect(tx, ty, P4_BOTTOM_BTN2_X, P4_BOTTOM_BTN_Y, P4_BOTTOM_BTN_W, P4_BOTTOM_BTN_H)) {
    if (p4_selectedLine >= 0 && p4_selectedLine < P4_LIST_LINES && p4_filePage[p4_selectedLine].length() > 0) {
      lastSelectedFile = p4_filePage[p4_selectedLine];
      currentPage = PAGE_5;
      drawPage5();
    }
    return;
  }
}

// ---------- Page 5 ----------
#define P5_TITLE_X 10
#define P5_TITLE_Y 10
#define P5_GRID_TOP 44
#define P5_GRID_LEFT 10
#define P5_GRID_W 300
#define P5_GRID_H 180
#define P5_BTN_COLS 3
#define P5_BTN_ROWS 3
#define P5_BTN_H 52
#define P5_BTN_W 94
#define P5_BTN_SPACING 6

enum PlotMode {
  NONE,
  FFT_MIC1,
  FFT_MIC2,
  FFT_BOTH,
  IMPULSE_MIC1,
  IMPULSE_MIC2,
  IMPULSE_BOTH
};

PlotMode currentPlot = NONE;

void plotDataOnTFT(PlotMode mode, String filename, int x0, int y0, int w, int h);

const char* p5_btnLabels[P5_BTN_ROWS * P5_BTN_COLS][2] = {
  { "FFT for", "mic 1" },
  { "FFT for", "mic 2" },
  { "FFT for both", "mic 1 and 2" },
  { "Impulseresponse", "mic 1" },
  { "Impulseresponse", "mic 2" },
  { "Impulseresponse", "mic 1 and 2" },
  { "Previous page", "" },
  { "Live FFT", "" },
  { "Front page", "" }
};
void p5_drawCenteredMultilineButtonText(const char* line1, const char* line2, int x, int y, int w, int h, uint16_t color, uint8_t size = 1) {
  tft.setTextColor(color);
  tft.setTextSize(size);
  int text1_w = strlen(line1) * 6 * size;
  int text2_w = strlen(line2) * 6 * size;
  int text_h = 8 * size;
  int lines = (line2[0] != '\0') ? 2 : 1;
  int ty = y + (h - lines * text_h - (lines - 1) * 4) / 2;
  int tx1 = x + (w - text1_w) / 2;
  tft.setCursor(tx1, ty);
  tft.print(line1);
  if (lines == 2) {
    int tx2 = x + (w - text2_w) / 2;
    tft.setCursor(tx2, ty + text_h + 4);
    tft.print(line2);
  }
}

void plotDataOnTFT(PlotMode mode, String filename, int x0, int y0, int w, int h) {
  File f = SD.open(filename.c_str());
  if (!f) {
    tft.setCursor(x0 + 10, y0 + 30);
    tft.setTextColor(ILI9341_RED);
    tft.setTextSize(2);
    tft.print("File error!");
    return;
  }

  const int maxPoints = 260;  // pixels in x axis
  float xs[maxPoints], ys1[maxPoints], ys2[maxPoints];
  int n = 0;

  // Set your sample rate here for impulse axis (change to your actual rate if not 44100)
  const float fs = 44100.0f;  
  const float inv_fs = 1.0f / fs;

  if (mode == FFT_MIC1 || mode == FFT_MIC2 || mode == FFT_BOTH) {
    // --- FFT mode: 2-pass even-sampling for frequency domain ---
    int totalPoints = 0;
    while (f.available()) {
      String line = f.readStringUntil('\n');
      line.trim();
      if (line.length() == 0 || line.startsWith("FreqHz")) continue;
      float values[20] = {0};
      int col = 0, from = 0, to = 0;
      while (from < line.length() && col < 20) {
        to = line.indexOf(',', from);
        if (to < 0) to = line.length();
        values[col] = line.substring(from, to).toFloat();
        from = to + 1;
        col++;
      }
      if (values[0] > 1250.0) break;
      totalPoints++;
    }
    f.close();

    // Pass 2: read and store evenly
    f = SD.open(filename.c_str());
    int currentIndex = 0, used = 0;
    int every = max(1, totalPoints / maxPoints);
    while (f.available()) {
      String line = f.readStringUntil('\n');
      line.trim();
      if (line.length() == 0 || line.startsWith("FreqHz")) continue;
      float values[20] = {0};
      int col = 0, from = 0, to = 0;
      while (from < line.length() && col < 20) {
        to = line.indexOf(',', from);
        if (to < 0) to = line.length();
        values[col] = line.substring(from, to).toFloat();
        from = to + 1;
        col++;
      }
      if (values[0] > 1250.0) break;

      if ((used < maxPoints && ((currentIndex % every == 0) || used == maxPoints - 1))) {
        xs[used] = values[0];
        if (mode == FFT_MIC1) {
          ys1[used] = values[7];
        } else if (mode == FFT_MIC2) {
          ys1[used] = values[13];
        } else if (mode == FFT_BOTH) {
          ys1[used] = values[7];
          ys2[used] = values[13];
        }
        used++;
      }
      currentIndex++;
      if (used >= maxPoints) break;
    }
    f.close();
    n = used;
  } else {
    // --- Impulse mode: Fast, just first maxPoints samples ---
    int used = 0;
    while (f.available() && used < maxPoints) {
      String line = f.readStringUntil('\n');
      line.trim();
      if (line.length() == 0 || line.startsWith("FreqHz")) continue;

      float values[20] = {0};
      int col = 0, from = 0, to = 0;
      while (from < line.length() && col < 20) {
        to = line.indexOf(',', from);
        if (to < 0) to = line.length();
        values[col] = line.substring(from, to).toFloat();
        from = to + 1;
        col++;
      }
      xs[used] = used * inv_fs * 1000.0f;
      if (mode == IMPULSE_MIC1 && col > 10) {
        ys1[used] = values[10];
        used++;
      } else if (mode == IMPULSE_MIC2 && col > 16) {
        ys1[used] = values[16];
        used++;
      } else if (mode == IMPULSE_BOTH && col > 16) {
        ys1[used] = values[10];
        ys2[used] = values[16];
        used++;
      }
    }
    f.close();
    n = used;
  }

  // Error if not enough data
  if (n < 2) {
    tft.setCursor(x0 + 10, y0 + 50);
    tft.setTextColor(ILI9341_RED);
    tft.setTextSize(2);
    tft.print("Not enough data!");
    return;
  }

  // --- Find y min/max for autoscale ---
  float y_min = 1e10, y_max = -1e10;
  for (int i = 0; i < n; i++) {
    if ((mode == FFT_BOTH || mode == IMPULSE_BOTH) && i < maxPoints) {
      if (ys1[i] < y_min) y_min = ys1[i];
      if (ys2[i] < y_min) y_min = ys2[i];
      if (ys1[i] > y_max) y_max = ys1[i];
      if (ys2[i] > y_max) y_max = ys2[i];
    } else {
      if (ys1[i] < y_min) y_min = ys1[i];
      if (ys1[i] > y_max) y_max = ys1[i];
    }
  }
  float y_span = y_max - y_min;
  y_min -= 0.05 * y_span;
  y_max += 0.05 * y_span;
  y_span = y_max - y_min;
  if (fabs(y_span) < 1e-6) y_span = 1.0;

  // --- X axis setup for drawing axis labels/ticks ---
  float x_start = xs[0];
  float x_end   = xs[n - 1];
  float x_span  = x_end - x_start;
  if (fabs(x_span) < 1e-6) x_span = 1.0;

  // --- Draw axes labels and ticks ---
  tft.setTextSize(1);
  tft.setTextColor(ILI9341_BLACK);

  // --- X axis ticks and label ---
  if (mode == FFT_MIC1 || mode == FFT_MIC2 || mode == FFT_BOTH) {
    for (int fx = 0; fx <= 5; fx++) {
      int freq = fx * 250;
      int xp = x0 + (int)(freq * w / 1250.0f);
      tft.setCursor(xp - 6, y0 + h + 8);
      tft.print(freq);
      if (fx == 5) tft.print(" Hz");
      // Draw vertical tick
      tft.drawFastVLine(xp, y0 + h - 3, 6, ILI9341_BLACK);
    }
    // Y axis label
    tft.setCursor(x0 - 28, y0 - 10);
    tft.print("dB");
  } else {
    int max_ms = (int)(xs[n-1]) + 1;
    for (int tm = 0; tm <= 5; tm++) {
      int t_ms = tm * (max_ms/5);
      int xp = x0 + (int)((t_ms - xs[0]) * w / (xs[n-1] - xs[0]));
      tft.setCursor(xp - 6, y0 + h + 8);
      tft.print(t_ms);
      if (tm == 5) tft.print(" ms");
      tft.drawFastVLine(xp, y0 + h - 3, 6, ILI9341_BLACK);
    }
    tft.setCursor(x0 - 28, y0 - 10);
    tft.print("Amp");
  }

  // --- Y axis ticks and labels ---
  int nYTicks = 5;
  for (int t = 0; t <= nYTicks; t++) {
    float yval = y_min + (y_span * t) / nYTicks;
    int yp = y0 + h - (int)((yval - y_min) * h / y_span);
    tft.setCursor(x0 - 30, yp - 4);
    // You can adjust decimals here:
    tft.print(String(yval, 1));
    tft.drawFastHLine(x0 - 3, yp, 6, ILI9341_BLACK);
  }

  // --- Plot the data ---
  for (int i = 1; i < n; i++) {
    int xp0 = x0 + (int)((xs[i-1] - xs[0]) * w / x_span);
    int xp1 = x0 + (int)((xs[i]   - xs[0]) * w / x_span);
    int yp0 = y0 + h - (int)((ys1[i-1] - y_min) * h / y_span);
    int yp1 = y0 + h - (int)((ys1[i]   - y_min) * h / y_span);
    tft.drawLine(xp0, yp0, xp1, yp1, ILI9341_BLUE);

    if ((mode == FFT_BOTH || mode == IMPULSE_BOTH)) {
      int yp0b = y0 + h - (int)((ys2[i-1] - y_min) * h / y_span);
      int yp1b = y0 + h - (int)((ys2[i]   - y_min) * h / y_span);
      tft.drawLine(xp0, yp0b, xp1, yp1b, ILI9341_RED);
    }
  }
}

void drawPage5() {
  tft.fillScreen(ILI9341_WHITE);
  tft.setTextSize(2);
  tft.setTextColor(ILI9341_BLACK);
  tft.setCursor(P5_TITLE_X, P5_TITLE_Y);
  tft.print("What should be displayed?");
  for (int row = 0; row < P5_BTN_ROWS; row++) {
    for (int col = 0; col < P5_BTN_COLS; col++) {
      int idx = row * P5_BTN_COLS + col;
      int x = P5_GRID_LEFT + P5_BTN_SPACING + col * (P5_BTN_W + P5_BTN_SPACING);
      int y = P5_GRID_TOP + P5_BTN_SPACING + row * (P5_BTN_H + P5_BTN_SPACING);
      tft.fillRoundRect(x, y, P5_BTN_W, P5_BTN_H, 10, ILI9341_LIGHTGREY);
      tft.drawRoundRect(x, y, P5_BTN_W, P5_BTN_H, 10, ILI9341_BLACK);
      p5_drawCenteredMultilineButtonText(
        p5_btnLabels[idx][0], p5_btnLabels[idx][1], x, y, P5_BTN_W, P5_BTN_H, ILI9341_BLACK, 1);
    }
  }
}

bool p5_inRect(int x, int y, int rx, int ry, int rw, int rh) {
  return (x >= rx) && (x < rx + rw) && (y >= ry) && (y < ry + rh);
}

void p5_handleTouch(int tx, int ty) {
  for (int row = 0; row < P5_BTN_ROWS; row++) {
    for (int col = 0; col < P5_BTN_COLS; col++) {
      int idx = row * P5_BTN_COLS + col;
      int x = P5_GRID_LEFT + P5_BTN_SPACING + col * (P5_BTN_W + P5_BTN_SPACING);
      int y = P5_GRID_TOP + P5_BTN_SPACING + row * (P5_BTN_H + P5_BTN_SPACING);
      if (p5_inRect(tx, ty, x, y, P5_BTN_W, P5_BTN_H)) {
        // For all plot actions, set PAGE_PLOT!
        if (idx == 0) {
          currentPlot = FFT_MIC1;
          currentPage = PAGE_PLOT;
          drawPlotPage();
          return;
        }
        if (idx == 1) {
          currentPlot = FFT_MIC2;
          currentPage = PAGE_PLOT;
          drawPlotPage();
          return;
        }
        if (idx == 2) {
          currentPlot = FFT_BOTH;
          currentPage = PAGE_PLOT;
          drawPlotPage();
          return;
        }
        if (idx == 3) {
          currentPlot = IMPULSE_MIC1;
          currentPage = PAGE_PLOT;
          drawPlotPage();
          return;
        }
        if (idx == 4) {
          currentPlot = IMPULSE_MIC2;
          currentPage = PAGE_PLOT;
          drawPlotPage();
          return;
        }
        if (idx == 5) {
          currentPlot = IMPULSE_BOTH;
          currentPage = PAGE_PLOT;
          drawPlotPage();
          return;
        }
        if (idx == 6) {
          currentPage = PAGE_4;
          drawPage4();
          return;
        }
        if (idx == 8) {
          currentPage = PAGE_1;
          drawPage1();
          return;
        }
        return;
      }
    }
  }
}

void drawPlotPage() {
  tft.fillScreen(ILI9341_WHITE);
  tft.setTextColor(ILI9341_BLACK);
  tft.setTextSize(2);
  tft.setCursor(10, 6);
  // Title depends on plot type
  if (currentPlot == FFT_MIC1) tft.print("FFT for mic 1");
  else if (currentPlot == FFT_MIC2) tft.print("FFT for mic 2");
  else if (currentPlot == FFT_BOTH) tft.print("FFT both mics");
  else if (currentPlot == IMPULSE_MIC1) tft.print("Impulse mic 1");
  else if (currentPlot == IMPULSE_MIC2) tft.print("Impulse mic 2");
  else if (currentPlot == IMPULSE_BOTH) tft.print("Impulse both");

  // "Back" button
  tft.fillRoundRect(220, 212, 90, 21, 8, ILI9341_DARKGREY);
  tft.drawRoundRect(220, 212, 90, 21, 8, ILI9341_BLACK);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);
  tft.setCursor(240, 215);
  tft.print("Back");

  // Draw axes
  int x0 = 32, y0 = 40, w = 260, h = 150;
  tft.drawRect(x0, y0, w, h, ILI9341_BLACK);
  tft.setTextSize(1);
  tft.setTextColor(ILI9341_BLACK);
  tft.setCursor(x0, y0 + h + 8);
  tft.print("X");
  tft.setCursor(x0 - 20, y0);
  tft.print("Y");

  // Plot signal (see below)
  plotDataOnTFT(currentPlot, lastSelectedFile, x0, y0, w, h);
}

void plotPageHandleTouch(int tx, int ty) {
  // "Back" button at (220, 200, 90, 32)
  if (tx >= 220 && tx < 310 && ty >= 200 && ty < 232) {
    currentPage = PAGE_5;
    drawPage5();
    currentPlot = NONE;
  }
}

// --------- Touch dispatcher ---------
void handleTouchAnyPage(int tx, int ty) {
  switch (currentPage) {
    case PAGE_1:
      // Left button
      if (tx >= P1_BUTTON_LEFT_X && tx < P1_BUTTON_LEFT_X + P1_BUTTON_W && ty >= P1_BUTTON_Y && ty < P1_BUTTON_Y + P1_BUTTON_H) {
        currentPage = PAGE_2;
        drawPage2();
        return;
      }
      // Right button
      if (tx >= P1_BUTTON_RIGHT_X && tx < P1_BUTTON_RIGHT_X + P1_BUTTON_W && ty >= P1_BUTTON_Y && ty < P1_BUTTON_Y + P1_BUTTON_H) {
        currentPage = PAGE_4;
        drawPage4();
        return;
      }
      break;
    case PAGE_2:
      p2_handleTouch(tx, ty);
      break;
    case PAGE_3:
      // Could add a back button to PAGE_1 or PAGE_2 here if needed
      break;
    case PAGE_4:
      p4_handleTouch(tx, ty);
      break;
    case PAGE_5:
      p5_handleTouch(tx, ty);
      break;
    case PAGE_PLOT: 
      plotPageHandleTouch(tx, ty);
      break;
  }
}

// ----------- SETUP & MAIN LOOP -----------
void setup() {
  Serial.begin(115200);
  tft.begin();
  tft.setRotation(1);
  ts.begin();
  ts.setRotation(3);

  // Page 4 (SD) only: init card and files
  if (!SD.begin(SD_CS)) {
    tft.fillScreen(ILI9341_WHITE);
    tft.setTextColor(ILI9341_RED);
    tft.setTextSize(2);
    tft.setCursor(30, 120);
    tft.print("SD card error!");
    while (1)
      ;
  }
  p4_totalFiles = p4_countSDfiles();
  p4_pageOffset = 0;
  p4_selectedLine = -1;
  p4_loadFilePage(p4_pageOffset);

  // Page 3: Clear log
  for (int i = 0; i < P3_LOG_LINES; i++) p3_logBuffer[i] = "";

  // Initial page:
  currentPage = PAGE_1;
  drawPage1();
}

void loop() {
  // Touch polling for all pages
  if (ts.touched()) {
    TS_Point p = ts.getPoint();
    int tx = map(p.x, 0, 4095, 0, tft.width());
    int ty = map(p.y, 0, 4095, 0, tft.height());
    handleTouchAnyPage(tx, ty);
    delay(180);
  }
}
