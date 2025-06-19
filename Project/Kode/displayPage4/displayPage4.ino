// Page 4 on the tft display
#include <SPI.h>
#include <ILI9341_t3.h>
#include <XPT2046_Touchscreen.h>
#include <SD.h>

#define TFT_CS   10
#define TFT_DC   9
#define TFT_RST  255
#define TOUCH_CS 8
#define TOUCH_IRQ 255
#define SD_CS    BUILTIN_SDCARD  // Change to match your SD card wiring

ILI9341_t3 tft = ILI9341_t3(TFT_CS, TFT_DC, TFT_RST);
XPT2046_Touchscreen ts(TOUCH_CS, TOUCH_IRQ);

#define TITLE_X 60
#define TITLE_Y 10

#define LIST_X 20
#define LIST_Y 40
#define LIST_W 280
#define LIST_H 120
#define LIST_R 10

#define LIST_LINES 6
#define LINE_H 20

// ---- Side-by-side navigation buttons ----
#define NAV_BTN_W  44
#define NAV_BTN_H  28
#define NAV_BTN_Y  (LIST_Y+LIST_H+4)
#define UP_BTN_X   (LIST_X + LIST_W/2 - NAV_BTN_W - 6)
#define DN_BTN_X   (LIST_X + LIST_W/2 + 6)

#define BOTTOM_BTN_W   158
#define BOTTOM_BTN_H    32
#define BOTTOM_BTN_R     8
#define BOTTOM_BTN_SP   2
#define BOTTOM_BTN_Y   200
#define BOTTOM_BTN1_X   1
#define BOTTOM_BTN2_X  (BOTTOM_BTN1_X + BOTTOM_BTN_W + BOTTOM_BTN_SP+5)

String filePage[LIST_LINES];         // Only the current visible file names
unsigned long totalFiles = 0;        // How many files exist (for navigation)
unsigned long pageOffset = 0;        // Index in file list of top of current page (0,6,12,...)
int selectedLine = -1;               // Which file is selected in visible page (0-5), -1 if none

// Utility: Draw centered text in a rect
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

// Count total files (with extension filter)
unsigned long countSDfiles() {
  unsigned long count = 0;
  File root = SD.open("/");
  File entry = root.openNextFile();
  while (entry) {
    if (!entry.isDirectory()) {
      String name = entry.name();
      if (name.endsWith(".TXT") || name.endsWith(".txt") ||
          name.endsWith(".CSV") || name.endsWith(".csv")) {
        count++;
      }
    }
    entry.close();
    entry = root.openNextFile();
  }
  root.close();
  return count;
}

// Fill filePage[] with files for the current page (starting at pageOffset)
void loadFilePage(unsigned long startIdx) {
  File root = SD.open("/");
  File entry = root.openNextFile();
  unsigned long idx = 0;
  int pagePos = 0;
  // Clear previous
  for (int i = 0; i < LIST_LINES; i++) filePage[i] = "";
  while (entry && pagePos < LIST_LINES) {
    if (!entry.isDirectory()) {
      String name = entry.name();
      if (name.endsWith(".TXT") || name.endsWith(".txt") ||
          name.endsWith(".CSV") || name.endsWith(".csv")) {
        if (idx >= startIdx) {
          filePage[pagePos++] = name;
        }
        idx++;
      }
    }
    entry.close();
    entry = root.openNextFile();
  }
  root.close();
}

// Draw file list for current page
void drawFileList() {
  // List box
  tft.fillRect(LIST_X+1, LIST_Y+1, LIST_W-2, LIST_H-2, ILI9341_WHITE);
  tft.drawRoundRect(LIST_X, LIST_Y, LIST_W, LIST_H, LIST_R, ILI9341_BLACK);

  tft.setTextSize(1);
  int visible = 0;
  for (int i = 0; i < LIST_LINES; i++) {
    if (filePage[i].length() == 0) continue;
    int y = LIST_Y + 4 + i * LINE_H;
    if (i == selectedLine) {
      tft.fillRect(LIST_X+2, y-2, LIST_W-4, LINE_H, ILI9341_BLUE);
      tft.setTextColor(ILI9341_WHITE);
    } else {
      tft.setTextColor(ILI9341_BLACK);
    }
    tft.setCursor(LIST_X + 10, y);
    tft.print(filePage[i]);
    visible++;
  }

  // --- Draw side-by-side up/down buttons ---
  // Up button
  tft.fillRect(UP_BTN_X, NAV_BTN_Y, NAV_BTN_W, NAV_BTN_H, (pageOffset > 0) ? ILI9341_DARKGREY : ILI9341_LIGHTGREY);
  tft.drawRect(UP_BTN_X, NAV_BTN_Y, NAV_BTN_W, NAV_BTN_H, ILI9341_BLACK);
  drawCenteredText("^", UP_BTN_X, NAV_BTN_Y, NAV_BTN_W, NAV_BTN_H, ILI9341_BLACK, 2);

  // Down button
  tft.fillRect(DN_BTN_X, NAV_BTN_Y, NAV_BTN_W, NAV_BTN_H,
    (pageOffset + LIST_LINES < totalFiles) ? ILI9341_DARKGREY : ILI9341_LIGHTGREY);
  tft.drawRect(DN_BTN_X, NAV_BTN_Y, NAV_BTN_W, NAV_BTN_H, ILI9341_BLACK);
  drawCenteredText("v", DN_BTN_X, NAV_BTN_Y, NAV_BTN_W, NAV_BTN_H, ILI9341_BLACK, 2);
}

void drawPage4() {
  tft.fillScreen(ILI9341_WHITE);

  // Title
  tft.setTextSize(2);
  tft.setTextColor(ILI9341_BLACK);
  tft.setCursor(TITLE_X, TITLE_Y);
  tft.print("Select file");

  drawFileList();

  // Bottom Buttons
  tft.fillRoundRect(BOTTOM_BTN1_X, BOTTOM_BTN_Y, BOTTOM_BTN_W+5, BOTTOM_BTN_H, BOTTOM_BTN_R, ILI9341_DARKGREY);
  tft.drawRoundRect(BOTTOM_BTN1_X, BOTTOM_BTN_Y, BOTTOM_BTN_W+5, BOTTOM_BTN_H, BOTTOM_BTN_R, ILI9341_BLACK);
  drawCenteredText("Previous page", BOTTOM_BTN1_X, BOTTOM_BTN_Y, BOTTOM_BTN_W+5, BOTTOM_BTN_H, ILI9341_BLACK, 2);

  tft.fillRoundRect(BOTTOM_BTN2_X, BOTTOM_BTN_Y, BOTTOM_BTN_W-5, BOTTOM_BTN_H, BOTTOM_BTN_R, ILI9341_BLUE);
  tft.drawRoundRect(BOTTOM_BTN2_X, BOTTOM_BTN_Y, BOTTOM_BTN_W-5, BOTTOM_BTN_H, BOTTOM_BTN_R, ILI9341_BLACK);
  drawCenteredText("Read data", BOTTOM_BTN2_X, BOTTOM_BTN_Y, BOTTOM_BTN_W-5, BOTTOM_BTN_H, ILI9341_WHITE, 2);
}

// Touch helpers
bool inRect(int x, int y, int rx, int ry, int rw, int rh) {
  return (x >= rx) && (x < rx + rw) && (y >= ry) && (y < ry + rh);
}

void handleTouch(int tx, int ty) {
  // Up button (left)
  if (pageOffset > 0 && inRect(tx, ty, UP_BTN_X, NAV_BTN_Y, NAV_BTN_W, NAV_BTN_H)) {
    pageOffset -= LIST_LINES;
    if (pageOffset < 0) pageOffset = 0;
    selectedLine = -1;
    loadFilePage(pageOffset);
    drawFileList();
    return;
  }
  // Down button (right)
  if (pageOffset + LIST_LINES < totalFiles && inRect(tx, ty, DN_BTN_X, NAV_BTN_Y, NAV_BTN_W, NAV_BTN_H)) {
    pageOffset += LIST_LINES;
    selectedLine = -1;
    loadFilePage(pageOffset);
    drawFileList();
    return;
  }
  // Tap on file line
  for (int i=0; i<LIST_LINES; i++) {
    int y = LIST_Y + 4 + i * LINE_H;
    if (filePage[i].length() > 0 && inRect(tx, ty, LIST_X, y-2, LIST_W, LINE_H)) {
      selectedLine = i;
      drawFileList();
      return;
    }
  }
  // Previous page button
  if (inRect(tx, ty, BOTTOM_BTN1_X, BOTTOM_BTN_Y, BOTTOM_BTN_W, BOTTOM_BTN_H)) {
    // Previous page action
    return;
  }
  // Read data button
  if (inRect(tx, ty, BOTTOM_BTN2_X, BOTTOM_BTN_Y, BOTTOM_BTN_W, BOTTOM_BTN_H)) {
    if (selectedLine >= 0 && selectedLine < LIST_LINES && filePage[selectedLine].length() > 0) {
      String chosenFile = filePage[selectedLine];
      Serial.print("Selected file: ");
      Serial.println(chosenFile);
      // You can implement loading the file here
    }
    return;
  }
}

void setup() {
  tft.begin();
  tft.setRotation(1);
  ts.begin();
  ts.setRotation(3);

  if (!SD.begin(SD_CS)) {
    tft.fillScreen(ILI9341_WHITE);
    tft.setTextColor(ILI9341_RED);
    tft.setTextSize(2);
    tft.setCursor(30, 120);
    tft.print("SD card error!");
    while (1);
  }
  totalFiles = countSDfiles();
  pageOffset = 0;
  selectedLine = -1;
  loadFilePage(pageOffset);
  drawPage4();
}

void loop() {
  if (ts.touched()) {
    TS_Point p = ts.getPoint();
    int tx = map(p.x, 0, 4095, 0, tft.width());
    int ty = map(p.y, 0, 4095, 0, tft.height());
    handleTouch(tx, ty);
    delay(180); // debounce
  }
}
