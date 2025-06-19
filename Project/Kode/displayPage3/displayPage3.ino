// Page 3 on the tft display
#include <SPI.h>
#include <ILI9341_t3.h>
#include <XPT2046_Touchscreen.h>

#define TFT_CS   10
#define TFT_DC   9
#define TFT_RST  255
#define TOUCH_CS 8
#define TOUCH_IRQ 255

ILI9341_t3 tft = ILI9341_t3(TFT_CS, TFT_DC, TFT_RST);

// --- Page 3 layout ---
#define TITLE "Currently testing..."
#define TITLE_X 50
#define TITLE_Y 10

#define BOX_X 16
#define BOX_Y 40
#define BOX_W 288
#define BOX_H 187
#define BOX_R 8

#define FONT_SIZE 1
#define LOG_LINES 15   // Number of lines that fit in the box for size 2 font

String logBuffer[LOG_LINES];
int logIndex = 0;

// --- Draw Functions ---
void drawPage3Box() {
  tft.fillRect(BOX_X+1, BOX_Y+1, BOX_W-2, BOX_H-2, ILI9341_WHITE);
  tft.drawRoundRect(BOX_X, BOX_Y, BOX_W, BOX_H, BOX_R, ILI9341_BLACK);

  // Print buffered log
  tft.setTextColor(ILI9341_BLACK);
  tft.setTextSize(FONT_SIZE);
  int lineHeight = 8 * FONT_SIZE + 4; // text height plus spacing
  int y = BOX_Y + 5;
  for (int i = 0; i < LOG_LINES; i++) {
    int idx = (logIndex + i) % LOG_LINES;
    tft.setCursor(BOX_X + 8, y + i * lineHeight);
    tft.print(logBuffer[idx]);
  }
}

void drawPage3() {
  tft.fillScreen(ILI9341_WHITE);

  // Title
  tft.setTextSize(2);
  tft.setTextColor(ILI9341_BLACK);
  tft.setCursor(TITLE_X, TITLE_Y);
  tft.print(TITLE);

  drawPage3Box();
}

// --- Log update function: call this instead of Serial.println! ---
void addToLog(String line) {
  logBuffer[logIndex] = line;
  logIndex = (logIndex + 1) % LOG_LINES;
  drawPage3Box();
  Serial.println(line); // Also print to serial, if desired
}

void setup() {
  tft.begin();
  tft.setRotation(1); // 320x240 landscape

  // Init log buffer to empty lines
  for (int i = 0; i < LOG_LINES; i++) logBuffer[i] = "";

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
}
int n=1;
void loop() {
  // For demo: nothing here
  // In your real program, replace Serial.println("...") with addToLog("...")
  addToLog(n);
  delay(10);
  n++;
}
