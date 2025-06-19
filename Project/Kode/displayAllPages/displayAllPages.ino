// Page 1 on the tft display
#include <SPI.h>
#include <ILI9341_t3.h>
#include <XPT2046_Touchscreen.h>
#include <SD.h>

#define TFT_CS 10
#define TFT_DC 9
#define TFT_RST 255
#define TOUCH_CS 8
#define TOUCH_IRQ 255
#define SD_CS    BUILTIN_SDCARD  // Change to match your SD card wiring

ILI9341_t3 tft = ILI9341_t3(TFT_CS, TFT_DC, TFT_RST);
XPT2046_Touchscreen ts(TOUCH_CS, TOUCH_IRQ);

// ---- Adjusted Layout ----
#define TITLE_YPage1 10

#define BUTTON_YPage1 34  // Moved up from 70
#define BUTTON_WPage1 120
#define BUTTON_HPage1 54  // Slightly taller for balance
#define BUTTON_SPACPage1 24
#define BUTTON_RPage1 10
#define BUTTON_LEFT_XPage1 32
#define BUTTON_RIGHT_XPage1 (BUTTON_LEFT_XPage1 + BUTTON_WPage1 + BUTTON_SPACPage1)

#define TEXTBOX_YPage1 100  // Moved up
#define TEXTBOX_HPage1 138  // Taller box
#define TEXTBOX_XPage1 2
#define TEXTBOX_WPage1 316
#define TEXTBOX_RPage1 8



String sampleName = "";

const char* row1[] = { "Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P" };
const int row1X = 30, row1Y = 72, keyW = 26, keyH = 28, keyGap = 2;
const char* row2[] = { "A", "S", "D", "F", "G", "H", "J", "K", "L" };
const int row2X = 43, row2Y = 102;
const char* row3[] = { "Z", "X", "C", "V", "B", "N", "M" };
const int row3X = 69, row3Y = 132;
struct SpecialKey {
  const char* label;
  int x, w;
};
// "OK" key REMOVED here
SpecialKey row4[] = {
  { "CLEAR", 30, 54 },
  { "SPACE", 88, 118 },
  { "<", 210, 54 }
};
const int row4Y = 162;

#define BOX_XPage2 30
#define BOX_YPage2 30
#define BOX_WPage2 260
#define BOX_HPage2 32

#define TITLE_XPage2 60
#define TITLE_YPage2 10

#define BOTTOM_BTN_WPage2 158
#define BOTTOM_BTN_HPage2 32
#define BOTTOM_BTN_RPage2 8
#define BOTTOM_BTN_SPPage2 2
#define BOTTOM_BTN_YPage2 203
#define BOTTOM_BTN1_XPage2 1
#define BOTTOM_BTN2_XPage2 (BOTTOM_BTN1_XPage2 + BOTTOM_BTN_WPage2 + BOTTOM_BTN_SPPage2 + 5)

// --- Page 3 layout ---
#define TITLEPage3 "Currently testing..."
#define TITLE_XPage3 50
#define TITLE_YPage3 10

#define BOX_XPage3 16
#define BOX_YPage3 40
#define BOX_WPage3 288
#define BOX_HPage3 187
#define BOX_RPage3 8

#define FONT_SIZEPage3 1
#define LOG_LINESPage3 15  // Number of lines that fit in the box for size 2 font

String logBuffer[LOG_LINESPage3];
int logIndex = 0;


#define TITLE_XPage4 60
#define TITLE_YPage4 10

#define LIST_XPage4 20
#define LIST_YPage4 40
#define LIST_WPage4 280
#define LIST_HPage4 120
#define LIST_RPage4 10

#define LIST_LINESPage4 6
#define LINE_HPage4 20

// ---- Side-by-side navigation buttons ----
#define NAV_BTN_WPage4  44
#define NAV_BTN_HPage4  28
#define NAV_BTN_YPage4  (LIST_YPage4+LIST_HPage4+4)
#define UP_BTN_XPage4   (LIST_XPage4 + LIST_WPage4/2 - NAV_BTN_WPage4 - 6)
#define DN_BTN_XPage4   (LIST_XPage4 + LIST_WPage4/2 + 6)

#define BOTTOM_BTN_WPage4   158
#define BOTTOM_BTN_HPage4    32
#define BOTTOM_BTN_RPage4     8
#define BOTTOM_BTN_SPPage4   2
#define BOTTOM_BTN_YPage4   200
#define BOTTOM_BTN1_XPage4   1
#define BOTTOM_BTN2_XPage4  (BOTTOM_BTN1_XPage4 + BOTTOM_BTN_WPage4 + BOTTOM_BTN_SPPage4+5)

String filePage[LIST_LINESPage4];         // Only the current visible file names
unsigned long totalFiles = 0;        // How many files exist (for navigation)
unsigned long pageOffset = 0;        // Index in file list of top of current page (0,6,12,...)
int selectedLine = -1;               // Which file is selected in visible page (0-5), -1 if none


void setup() {
  tft.begin();
  tft.setRotation(1);  // 320x240 landscape
  ts.begin();
  ts.setRotation(3);
  drawPage1();
  drawPage2();
  // Init log buffer to empty lines
  for (int i = 0; i < LOG_LINESPage3; i++) logBuffer[i] = "";

  drawPage3();

  // --- Example for demo ---
  addToLog("System ready.");
  delay(600);
  addToLog("Pump started...");
  delay(600);
  addToLog("Valve open.");
  delay(600);
  addToLog("Acquiring data...");
  delay(600);
  addToLog("50 Hz detected");
  delay(600);
  addToLog("Saving result...");
  delay(600);
  addToLog("Done.");

  

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
  
  drawPage5();
}

void loop() {
  if (ts.touched()) {
    TS_Point p = ts.getPoint();
    int tx = map(p.x, 0, 4095, 0, tft.width());
    int ty = map(p.y, 0, 4095, 0, tft.height());
    handleTouchPage3(tx, ty);
    delay(150);  // debounce
  }
  
}


void drawPage1() {
  tft.fillScreen(ILI9341_WHITE);

  // --- TITLEPage3 (smaller and higher) ---
  tft.setTextColor(ILI9341_BLACK);
  tft.setTextSize(2);              // Smaller
  tft.setCursor(5, TITLE_YPage1);  // Centered for 320px width, adjust as needed
  tft.print("What would you like to do?");

  // --- Left Button: Perform a new test (smaller text, higher button) ---
  tft.fillRoundRect(BUTTON_LEFT_XPage1, BUTTON_YPage1, BUTTON_WPage1, BUTTON_HPage1, BUTTON_RPage1, ILI9341_BLUE);
  tft.drawRoundRect(BUTTON_LEFT_XPage1, BUTTON_YPage1, BUTTON_WPage1, BUTTON_HPage1, BUTTON_RPage1, ILI9341_BLACK);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(1.5);  // Smaller text
  tft.setCursor(BUTTON_LEFT_XPage1 + 14, BUTTON_YPage1 + 22);
  tft.print("Perform a new");
  tft.setCursor(BUTTON_LEFT_XPage1 + 28, BUTTON_YPage1 + 36);
  tft.print("test");

  // --- Right Button: Read data from a previous test (smaller text, higher button) ---
  tft.fillRoundRect(BUTTON_RIGHT_XPage1, BUTTON_YPage1, BUTTON_WPage1, BUTTON_HPage1, BUTTON_RPage1, ILI9341_DARKGREY);
  tft.drawRoundRect(BUTTON_RIGHT_XPage1, BUTTON_YPage1, BUTTON_WPage1, BUTTON_HPage1, BUTTON_RPage1, ILI9341_BLACK);
  tft.setTextColor(ILI9341_BLACK);
  tft.setTextSize(1.5);  // Smaller text
  tft.setCursor(BUTTON_RIGHT_XPage1 + 8, BUTTON_YPage1 + 18);
  tft.print("Read data from a");
  tft.setCursor(BUTTON_RIGHT_XPage1 + 6, BUTTON_YPage1 + 32);
  tft.print("previous test");

  // --- Bottom Textbox (taller) ---
  tft.drawRoundRect(TEXTBOX_XPage1, TEXTBOX_YPage1, TEXTBOX_WPage1, TEXTBOX_HPage1, TEXTBOX_RPage1, ILI9341_BLACK);
  tft.setTextColor(ILI9341_BLACK);
  tft.setTextSize(1);

  // Multiline info text:
  int text_y = TEXTBOX_YPage1 + 6;
  tft.setCursor(TEXTBOX_XPage1 + 6, text_y);
  tft.print("The impedance tube itself has been developed in");
  text_y += 13;
  tft.setCursor(TEXTBOX_XPage1 + 6, text_y);
  tft.print("accordance with the DS/ISO 10534-2:2023 standard.");
  text_y += 13;
  tft.setCursor(TEXTBOX_XPage1 + 6, text_y);
  tft.print("Algorithms up until the transfer matrix method has");
  text_y += 13;
  tft.setCursor(TEXTBOX_XPage1 + 6, text_y);
  tft.print("been succesfully implemented directly on the Teensy");
  text_y += 13;
  tft.setCursor(TEXTBOX_XPage1 + 6, text_y);
  tft.print("4.1 with audio shield attached. The report found");
  text_y += 13;
  tft.setCursor(TEXTBOX_XPage1 + 6, text_y);
  tft.print("adjacent to the tube describes the development of");
  text_y += 13;
  tft.setCursor(TEXTBOX_XPage1 + 6, text_y);
  tft.print("the impedancetube and provides links for programs,");
  text_y += 13;
  tft.setCursor(TEXTBOX_XPage1 + 6, text_y);
  tft.print("algorithm etc. The transfer matrix method has been");
  text_y += 13;
  tft.setCursor(TEXTBOX_XPage1 + 6, text_y);
  tft.print("implemented in MATLAB only, due to memory");
  text_y += 13;
  tft.setCursor(TEXTBOX_XPage1 + 6, text_y);
  tft.print("constraints of the Teensy.");
}


void drawCenteredTextPage3(const char* str, int x, int y, int w, int h, uint16_t color, uint8_t textsize = 2) {
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
  tft.fillRect(BOX_XPage2 + 1, BOX_YPage2 + 1, BOX_WPage2 - 2, BOX_HPage2 - 2, ILI9341_WHITE);  // Clear inside
  tft.drawRect(BOX_XPage2, BOX_YPage2, BOX_WPage2, BOX_HPage2, ILI9341_BLACK);
  tft.setTextColor(ILI9341_BLACK);
  tft.setTextSize(2);
  tft.setCursor(BOX_XPage2 + 6, BOX_YPage2 + 8);
  tft.print(sampleName);
}

void drawPage2() {
  tft.fillScreen(ILI9341_WHITE);

  tft.setTextSize(2);
  tft.setTextColor(ILI9341_BLACK);
  tft.setCursor(TITLE_XPage2, TITLE_YPage2);
  tft.print("Enter a sample name");

  drawSampleBox();

  // Row 1: QWERTYUIOP
  tft.setTextSize(2);
  for (int i = 0; i < 10; i++) {
    int x = row1X + i * (keyW + keyGap);
    tft.fillRect(x, row1Y, keyW, keyH, ILI9341_WHITE);
    tft.drawRect(x, row1Y, keyW, keyH, ILI9341_BLACK);
    drawCenteredTextPage3(row1[i], x, row1Y, keyW, keyH, ILI9341_BLACK, 2);
  }

  // Row 2: ASDFGHJKL
  for (int i = 0; i < 9; i++) {
    int x = row2X + i * (keyW + keyGap);
    tft.fillRect(x, row2Y, keyW, keyH, ILI9341_WHITE);
    tft.drawRect(x, row2Y, keyW, keyH, ILI9341_BLACK);
    drawCenteredTextPage3(row2[i], x, row2Y, keyW, keyH, ILI9341_BLACK, 2);
  }

  // Row 3: ZXCVBNM
  for (int i = 0; i < 7; i++) {
    int x = row3X + i * (keyW + keyGap);
    tft.fillRect(x, row3Y, keyW, keyH, ILI9341_WHITE);
    tft.drawRect(x, row3Y, keyW, keyH, ILI9341_BLACK);
    drawCenteredTextPage3(row3[i], x, row3Y, keyW, keyH, ILI9341_BLACK, 2);
  }

  // Row 4: Specials (no OK button)
  for (int i = 0; i < 3; i++) {
    int x = row4[i].x;
    int w = row4[i].w;
    tft.fillRect(x, row4Y, w, keyH, ILI9341_WHITE);
    tft.drawRect(x, row4Y, w, keyH, ILI9341_BLACK);
    uint8_t keyTextSize = (strcmp(row4[i].label, "SPACE") == 0 || strcmp(row4[i].label, "CLEAR") == 0) ? 1 : 2;
    drawCenteredTextPage3(row4[i].label, x, row4Y, w, keyH, ILI9341_BLACK, keyTextSize);
  }

  // Bottom Buttons
  tft.fillRoundRect(BOTTOM_BTN1_XPage2, BOTTOM_BTN_YPage2, BOTTOM_BTN_WPage2 + 5, BOTTOM_BTN_HPage2, BOTTOM_BTN_RPage2, ILI9341_DARKGREY);
  tft.drawRoundRect(BOTTOM_BTN1_XPage2, BOTTOM_BTN_YPage2, BOTTOM_BTN_WPage2 + 5, BOTTOM_BTN_HPage2, BOTTOM_BTN_RPage2, ILI9341_BLACK);
  drawCenteredTextPage3("Previous page", BOTTOM_BTN1_XPage2, BOTTOM_BTN_YPage2, BOTTOM_BTN_WPage2 + 5, BOTTOM_BTN_HPage2, ILI9341_BLACK, 2);

  tft.fillRoundRect(BOTTOM_BTN2_XPage2, BOTTOM_BTN_YPage2, BOTTOM_BTN_WPage2 - 5, BOTTOM_BTN_HPage2, BOTTOM_BTN_RPage2, ILI9341_BLUE);
  tft.drawRoundRect(BOTTOM_BTN2_XPage2, BOTTOM_BTN_YPage2, BOTTOM_BTN_WPage2 - 5, BOTTOM_BTN_HPage2, BOTTOM_BTN_RPage2, ILI9341_BLACK);
  drawCenteredTextPage3("Perform test", BOTTOM_BTN2_XPage2, BOTTOM_BTN_YPage2, BOTTOM_BTN_WPage2 - 5, BOTTOM_BTN_HPage2, ILI9341_WHITE, 2);
}

bool inRectPage3(int x, int y, int rx, int ry, int rw, int rh) {
  return (x >= rx) && (x < rx + rw) && (y >= ry) && (y < ry + rh);
}

void handleTouchPage3(int tx, int ty) {
  // Keyboard rows
  for (int i = 0; i < 10; i++) {
    int x = row1X + i * (keyW + keyGap);
    if (inRectPage3(tx, ty, x, row1Y, keyW, keyH)) {
      if (sampleName.length() < 20) {
        sampleName += row1[i];
        drawSampleBox();
      }
      return;
    }
  }
  for (int i = 0; i < 9; i++) {
    int x = row2X + i * (keyW + keyGap);
    if (inRectPage3(tx, ty, x, row2Y, keyW, keyH)) {
      if (sampleName.length() < 20) {
        sampleName += row2[i];
        drawSampleBox();
      }
      return;
    }
  }
  for (int i = 0; i < 7; i++) {
    int x = row3X + i * (keyW + keyGap);
    if (inRectPage3(tx, ty, x, row3Y, keyW, keyH)) {
      if (sampleName.length() < 20) {
        sampleName += row3[i];
        drawSampleBox();
      }
      return;
    }
  }
  // Row 4: Specials
  // CLEAR
  if (inRectPage3(tx, ty, row4[0].x, row4Y, row4[0].w, keyH)) {
    sampleName = "";
    drawSampleBox();
    return;
  }
  // SPACE
  if (inRectPage3(tx, ty, row4[1].x, row4Y, row4[1].w, keyH)) {
    if (sampleName.length() < 20) {
      sampleName += " ";
      drawSampleBox();
    }
    return;
  }
  // BACKSPACE
  if (inRectPage3(tx, ty, row4[2].x, row4Y, row4[2].w, keyH)) {
    if (sampleName.length() > 0) {
      sampleName.remove(sampleName.length() - 1, 1);
      drawSampleBox();
    }
    return;
  }
  // Bottom buttons
  if (inRectPage3(tx, ty, BOTTOM_BTN1_XPage2, BOTTOM_BTN_YPage2, BOTTOM_BTN_WPage2 + 5, BOTTOM_BTN_HPage2)) {
    // Previous page
    // (Insert action here)
    return;
  }
  if (inRectPage3(tx, ty, BOTTOM_BTN2_XPage2, BOTTOM_BTN_YPage2, BOTTOM_BTN_WPage2 - 5, BOTTOM_BTN_HPage2)) {
    // Perform test
    // (Insert action here)
    return;
  }
}





// --- Draw Functions ---
void drawPage3Box() {
  tft.fillRect(BOX_XPage3 + 1, BOX_YPage3 + 1, BOX_WPage3 - 2, BOX_HPage3 - 2, ILI9341_WHITE);
  tft.drawRoundRect(BOX_XPage3, BOX_YPage3, BOX_WPage3, BOX_HPage3, BOX_RPage3, ILI9341_BLACK);

  // Print buffered log
  tft.setTextColor(ILI9341_BLACK);
  tft.setTextSize(FONT_SIZEPage3);
  int lineHeight = 8 * FONT_SIZEPage3 + 4;  // text height plus spacing
  int y = BOX_YPage3 + 5;
  for (int i = 0; i < LOG_LINESPage3; i++) {
    int idx = (logIndex + i) % LOG_LINESPage3;
    tft.setCursor(BOX_XPage3 + 8, y + i * lineHeight);
    tft.print(logBuffer[idx]);
  }
}

void drawPage3() {
  tft.fillScreen(ILI9341_WHITE);

  // TITLEPage3
  tft.setTextSize(2);
  tft.setTextColor(ILI9341_BLACK);
  tft.setCursor(TITLE_XPage3, TITLE_YPage3);
  tft.print(TITLEPage3);

  drawPage3Box();
}

// --- Log update function: call this instead of Serial.println! ---
void addToLog(String line) {
  logBuffer[logIndex] = line;
  logIndex = (logIndex + 1) % LOG_LINESPage3;
  drawPage3Box();
  Serial.println(line);  // Also print to serial, if desired
}

// Utility: Draw centered text in a rect
void drawCenteredTextPage4(const char* str, int x, int y, int w, int h, uint16_t color, uint8_t textsize=2) {
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
  for (int i = 0; i < LIST_LINESPage4; i++) filePage[i] = "";
  while (entry && pagePos < LIST_LINESPage4) {
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
  tft.fillRect(LIST_XPage4+1, LIST_YPage4+1, LIST_WPage4-2, LIST_HPage4-2, ILI9341_WHITE);
  tft.drawRoundRect(LIST_XPage4, LIST_YPage4, LIST_WPage4, LIST_HPage4, LIST_RPage4, ILI9341_BLACK);

  tft.setTextSize(1);
  int visible = 0;
  for (int i = 0; i < LIST_LINESPage4; i++) {
    if (filePage[i].length() == 0) continue;
    int y = LIST_YPage4 + 4 + i * LINE_HPage4;
    if (i == selectedLine) {
      tft.fillRect(LIST_XPage4+2, y-2, LIST_WPage4-4, LINE_HPage4, ILI9341_BLUE);
      tft.setTextColor(ILI9341_WHITE);
    } else {
      tft.setTextColor(ILI9341_BLACK);
    }
    tft.setCursor(LIST_XPage4 + 10, y);
    tft.print(filePage[i]);
    visible++;
  }

  // --- Draw side-by-side up/down buttons ---
  // Up button
  tft.fillRect(UP_BTN_XPage4, NAV_BTN_YPage4, NAV_BTN_WPage4, NAV_BTN_HPage4, (pageOffset > 0) ? ILI9341_DARKGREY : ILI9341_LIGHTGREY);
  tft.drawRect(UP_BTN_XPage4, NAV_BTN_YPage4, NAV_BTN_WPage4, NAV_BTN_HPage4, ILI9341_BLACK);
  drawCenteredTextPage4("^", UP_BTN_XPage4, NAV_BTN_YPage4, NAV_BTN_WPage4, NAV_BTN_HPage4, ILI9341_BLACK, 2);

  // Down button
  tft.fillRect(DN_BTN_XPage4, NAV_BTN_YPage4, NAV_BTN_WPage4, NAV_BTN_HPage4,
    (pageOffset + LIST_LINESPage4 < totalFiles) ? ILI9341_DARKGREY : ILI9341_LIGHTGREY);
  tft.drawRect(DN_BTN_XPage4, NAV_BTN_YPage4, NAV_BTN_WPage4, NAV_BTN_HPage4, ILI9341_BLACK);
  drawCenteredTextPage4("v", DN_BTN_XPage4, NAV_BTN_YPage4, NAV_BTN_WPage4, NAV_BTN_HPage4, ILI9341_BLACK, 2);
}

void drawPage4() {
  tft.fillScreen(ILI9341_WHITE);

  // Title
  tft.setTextSize(2);
  tft.setTextColor(ILI9341_BLACK);
  tft.setCursor(TITLE_XPage4, TITLE_YPage4);
  tft.print("Select file");

  drawFileList();

  // Bottom Buttons
  tft.fillRoundRect(BOTTOM_BTN1_XPage4, BOTTOM_BTN_YPage4, BOTTOM_BTN_WPage4+5, BOTTOM_BTN_HPage4, BOTTOM_BTN_RPage4, ILI9341_DARKGREY);
  tft.drawRoundRect(BOTTOM_BTN1_XPage4, BOTTOM_BTN_YPage4, BOTTOM_BTN_WPage4+5, BOTTOM_BTN_HPage4, BOTTOM_BTN_RPage4, ILI9341_BLACK);
  drawCenteredTextPage4("Previous page", BOTTOM_BTN1_XPage4, BOTTOM_BTN_YPage4, BOTTOM_BTN_WPage4+5, BOTTOM_BTN_HPage4, ILI9341_BLACK, 2);

  tft.fillRoundRect(BOTTOM_BTN2_XPage4, BOTTOM_BTN_YPage4, BOTTOM_BTN_WPage4-5, BOTTOM_BTN_HPage4, BOTTOM_BTN_RPage4, ILI9341_BLUE);
  tft.drawRoundRect(BOTTOM_BTN2_XPage4, BOTTOM_BTN_YPage4, BOTTOM_BTN_WPage4-5, BOTTOM_BTN_HPage4, BOTTOM_BTN_RPage4, ILI9341_BLACK);
  drawCenteredTextPage4("Read data", BOTTOM_BTN2_XPage4, BOTTOM_BTN_YPage4, BOTTOM_BTN_WPage4-5, BOTTOM_BTN_HPage4, ILI9341_WHITE, 2);
}

// Touch helpers
bool inRectPage4(int x, int y, int rx, int ry, int rw, int rh) {
  return (x >= rx) && (x < rx + rw) && (y >= ry) && (y < ry + rh);
}

void handleTouch(int tx, int ty) {
  // Up button (left)
  if (pageOffset > 0 && inRectPage4(tx, ty, UP_BTN_XPage4, NAV_BTN_YPage4, NAV_BTN_WPage4, NAV_BTN_HPage4)) {
    pageOffset -= LIST_LINESPage4;
    if (pageOffset < 0) pageOffset = 0;
    selectedLine = -1;
    loadFilePage(pageOffset);
    drawFileList();
    return;
  }
  // Down button (right)
  if (pageOffset + LIST_LINESPage4 < totalFiles && inRectPage4(tx, ty, DN_BTN_XPage4, NAV_BTN_YPage4, NAV_BTN_WPage4, NAV_BTN_HPage4)) {
    pageOffset += LIST_LINESPage4;
    selectedLine = -1;
    loadFilePage(pageOffset);
    drawFileList();
    return;
  }
  // Tap on file line
  for (int i=0; i<LIST_LINESPage4; i++) {
    int y = LIST_YPage4 + 4 + i * LINE_HPage4;
    if (filePage[i].length() > 0 && inRectPage4(tx, ty, LIST_XPage4, y-2, LIST_WPage4, LINE_HPage4)) {
      selectedLine = i;
      drawFileList();
      return;
    }
  }
  // Previous page button
  if (inRectPage4(tx, ty, BOTTOM_BTN1_XPage4, BOTTOM_BTN_YPage4, BOTTOM_BTN_WPage4, BOTTOM_BTN_HPage4)) {
    // Previous page action
    return;
  }
  // Read data button
  if (inRectPage4(tx, ty, BOTTOM_BTN2_XPage4, BOTTOM_BTN_YPage4, BOTTOM_BTN_WPage4, BOTTOM_BTN_HPage4)) {
    if (selectedLine >= 0 && selectedLine < LIST_LINESPage4 && filePage[selectedLine].length() > 0) {
      String chosenFile = filePage[selectedLine];
      Serial.print("Selected file: ");
      Serial.println(chosenFile);
      // You can implement loading the file here
    }
    return;
  }
}
// Page 5 on the tft display


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
