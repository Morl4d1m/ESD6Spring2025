#include <Audio.h>
#include <Wire.h>
#include <SD.h>
#include <SPI.h>
#include <SerialFlash.h>
#include "kiss_fft.h"
#include <ILI9341_t3.h>
#include <XPT2046_Touchscreen.h>
#include <math.h>

#define CH12Pin 30
#define CH34Pin 31
#define CH56Pin 32
#define CH12Pin2 35  // Two pins are used to absolutely ensure relays are switched
#define CH34Pin2 34
#define CH56Pin2 33

// ---- Hardware pinout ----
#define TFT_CS 10
#define TFT_DC 9
#define TFT_RST 255
#define TOUCH_CS 8
#define TOUCH_IRQ 255
#define SD_CS BUILTIN_SDCARD

ILI9341_t3 tft = ILI9341_t3(TFT_CS, TFT_DC, TFT_RST);
XPT2046_Touchscreen ts(TOUCH_CS, TOUCH_IRQ);

String p2_sampleName = "";

// Helper function to replace "SAMPLENAME" in a template filename
String makeFilenameWithSample(const String& tmpl) {
  String name = tmpl;
  name.replace("SAMPLENAME", p2_sampleName);
  return name;
}

// PSRAM allocator for KissFFT
kiss_fft_cfg kiss_fft_alloc_psram(int nfft, int inverse_fft) {
  size_t len_needed = 0;
  // Query memory requirement
  kiss_fft_alloc(nfft, inverse_fft, nullptr, &len_needed);
  //p3_addToLog("Allocating %zu bytes for KissFFT config\n");
  //p3_addToLog(len_needed);

  void* buffer = extmem_malloc(len_needed);
  if (!buffer) {
    p3_addToLogLN("[ERROR] PSRAM allocation for KissFFT failed!");
    return nullptr;
  }
  kiss_fft_cfg cfg = kiss_fft_alloc(nfft, inverse_fft, buffer, &len_needed);
  if (!cfg) {
    p3_addToLogLN("[ERROR] kiss_fft_alloc returned null even after buffer allocation");
    extmem_free(buffer);
  }
  return cfg;
}

// Wrapper to force PSRAM usage
__attribute__((section(".psram"))) void kiss_fft_psram(kiss_fft_cfg cfg, const kiss_fft_cpx* in, kiss_fft_cpx* out) {
  kiss_fft(cfg, in, out);
}

size_t next_power_of_2(size_t n) {
  size_t p = 1;
  while (p < n) p <<= 1;
  return p;
}


// General audio preparation
AudioOutputI2S i2s1;
AudioInputI2S2 i2sMic1;
AudioControlSGTL5000 sgtl5000_1;

// Mixer to enable all signals being "active" at once.
AudioMixer4 mixer;                              // Has 4 channels to mix on
AudioConnection patchCord5(mixer, 0, i2s1, 0);  // Mixer to left output
AudioConnection patchCord6(mixer, 0, i2s1, 1);  // Mixer to right output

// When MLS is generated
AudioPlayQueue MLSSignal;
AudioConnection patchCord1(MLSSignal, 0, mixer, 0);  // Sends the MLS to channel 0 in the mixer

// When pure sine is generated
AudioSynthWaveform sineWave;                        // Utilizes pre-made sine wave generator
AudioConnection patchCord2(sineWave, 0, mixer, 1);  // Sends the pure sine to channel 1 in the mixer

// When white noise is generated
AudioSynthNoiseWhite whiteNoise;                      // Utilizes pre-made white noise generator
AudioConnection patchCord3(whiteNoise, 0, mixer, 2);  // Sends the white noise to channel 2 in the mixer

// When sine sweep is generated
AudioSynthToneSweep sineSweep;                       // Utilizes pre-made sine sweep generator
AudioConnection patchCord4(sineSweep, 0, mixer, 3);  // Sends the sine sweep to channel 3 in the mixer

// Signal recorder for testing:
AudioRecordQueue recordQueue;  // To capture audio data directly from mixer
AudioConnection patchCord7(mixer, 0, recordQueue, 0);

// Microphone recording for impulse responses:
AudioRecordQueue mic1Queue;  // Recordqueue for the 1st microphone
AudioConnection patchCord8(i2sMic1, 0, mic1Queue, 0);
AudioRecordQueue mic2Queue;  // Recordqueue for the 2nd microphone
AudioConnection patchCord9(i2sMic1, 1, mic2Queue, 0);

// SD card stuff
const int chipSelect = BUILTIN_SDCARD;
File mixerFile;
File micFile;
File combinedFile;

// Global variables
//const uint8_t ledPin = 13;        // Pin 13 is the builtin LED
uint32_t LFSRBits = 15;           // Change this between 2 and 32
const unsigned long delayUS = 1;  // Delay in microseconds between bits
uint32_t LFSR;
uint32_t mask;
float sineSweepTime = 0.743;                      // Practically decides how fast the frequency is changed in the sine sweep, by saying deltaF/time
const uint16_t sampleRate = 44100;                // Default sample rate
const uint8_t blockSize = 128;                    // Default block size for the audio.h library
const int blocksNeeded = sampleRate / blockSize;  // 1 second of samples
uint16_t totalSamples = 441000;                   // Desired length of signal sequence
uint8_t iteration = 0;                            // Counter for alternating between microphone pairs
const float epsilon = 1e-10f;

// ---- Page Management ----
enum PageNum { PAGE_1 = 1,
               PAGE_2,
               PAGE_3,
               PAGE_4,
               PAGE_5,
               PAGE_PLOT,
               PAGE_HELP };
PageNum currentPage = PAGE_1;

String lastSelectedFile = "";  // Global variable for selected file


// ---------- Page 1 ----------
#define P1_TITLE_Y 10
#define P1_BUTTON_Y 34
#define P1_BUTTON_W 100
#define P1_BUTTON_H 54
#define P1_BUTTON_MID_W 100
#define P1_BUTTON_SPAC 8
#define P1_BUTTON_R 8
#define P1_BUTTON_LEFT_X 2
#define P1_BUTTON_MID_X (P1_BUTTON_LEFT_X + P1_BUTTON_W + P1_BUTTON_SPAC)
#define P1_BUTTON_RIGHT_X (P1_BUTTON_MID_X + P1_BUTTON_MID_W + P1_BUTTON_SPAC)
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
  tft.setCursor(P1_BUTTON_LEFT_X + 6, P1_BUTTON_Y + 18);
  tft.print("Perform a new");
  tft.setCursor(P1_BUTTON_LEFT_X + 6, P1_BUTTON_Y + 32);
  tft.print("test");

  // --- Middle Button ---
  tft.fillRoundRect(P1_BUTTON_MID_X, P1_BUTTON_Y, P1_BUTTON_MID_W, P1_BUTTON_H, P1_BUTTON_R, ILI9341_GREEN);
  tft.drawRoundRect(P1_BUTTON_MID_X, P1_BUTTON_Y, P1_BUTTON_MID_W, P1_BUTTON_H, P1_BUTTON_R, ILI9341_BLACK);
  tft.setTextColor(ILI9341_BLACK);
  tft.setTextSize(1);
  tft.setCursor(P1_BUTTON_MID_X + 6, P1_BUTTON_Y + 6);
  tft.print("How to use the");
  tft.setCursor(P1_BUTTON_MID_X + 6, P1_BUTTON_Y + 18);
  tft.print("impedancetube");
  tft.setCursor(P1_BUTTON_MID_X + 6, P1_BUTTON_Y + 30);
  tft.print("in its current");
  tft.setCursor(P1_BUTTON_MID_X + 6, P1_BUTTON_Y + 42);
  tft.print("state.");

  // --- Right Button ---
  tft.fillRoundRect(P1_BUTTON_RIGHT_X, P1_BUTTON_Y, P1_BUTTON_W, P1_BUTTON_H, P1_BUTTON_R, ILI9341_DARKGREY);
  tft.drawRoundRect(P1_BUTTON_RIGHT_X, P1_BUTTON_Y, P1_BUTTON_W, P1_BUTTON_H, P1_BUTTON_R, ILI9341_BLACK);
  tft.setTextColor(ILI9341_BLACK);
  tft.setTextSize(1.5);
  tft.setCursor(P1_BUTTON_RIGHT_X + 6, P1_BUTTON_Y + 18);
  tft.print("Read data from");
  tft.setCursor(P1_BUTTON_RIGHT_X + 6, P1_BUTTON_Y + 32);
  tft.print("a previous test");

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

// --- Help Page Defines (reuse page 1’s constants where relevant) ---
#define HELP_TITLE_Y        P1_TITLE_Y
#define HELP_TITLE_X        5
#define HELP_TITLE_SIZE     2

#define HELP_TEXTBOX_X      P1_TEXTBOX_X
#define HELP_TEXTBOX_Y      P1_TEXTBOX_Y
#define HELP_TEXTBOX_W      P1_TEXTBOX_W
#define HELP_TEXTBOX_H      P1_TEXTBOX_H
#define HELP_TEXTBOX_R      P1_TEXTBOX_R

#define HELP_BTN_W          168
#define HELP_BTN_H          28
#define HELP_BTN_R          8
#define HELP_BTN_X          (320 - HELP_BTN_W - 8)  // Right aligned, 8px margin
#define HELP_BTN_Y          (246 - HELP_BTN_H - 8)  // Bottom aligned, 8px margin

void drawHelpPage() {
  tft.fillScreen(ILI9341_WHITE);

  // --- Title ---
  tft.setTextColor(ILI9341_BLACK);
  tft.setTextSize(HELP_TITLE_SIZE);
  tft.setCursor(HELP_TITLE_X, HELP_TITLE_Y);
  tft.print("How to use impedance tube:");

  // --- Textbox ---
  tft.drawRoundRect(HELP_TEXTBOX_X, HELP_TEXTBOX_Y-69, HELP_TEXTBOX_W, HELP_TEXTBOX_H+38, HELP_TEXTBOX_R, ILI9341_BLACK);
  tft.setTextColor(ILI9341_BLACK);
  tft.setTextSize(1);
  int text_y = HELP_TEXTBOX_Y + 6-69;
  tft.setCursor(HELP_TEXTBOX_X + 6, text_y);
  tft.print("1. Screw the impedance tube to the speaker using");
  text_y += 12;
  tft.setCursor(HELP_TEXTBOX_X + 6, text_y);
  tft.print("the two bottom screws. Connect the SWT as usual.");
  text_y += 12;
  tft.setCursor(HELP_TEXTBOX_X + 6, text_y);
  tft.print("Insert jack to Type 2706 amp.");
  text_y += 12;
  tft.setCursor(HELP_TEXTBOX_X + 6, text_y);
  tft.print("2. Place the sample sample holder, and slide it");
  text_y += 12;
  tft.setCursor(HELP_TEXTBOX_X + 6, text_y);
  tft.print("forward so that the front is aligned.\n");
  text_y += 12;
  tft.setCursor(HELP_TEXTBOX_X + 6, text_y);
  tft.print("3. Seal all openings - If connections seem loose,");
  text_y += 12;
  tft.setCursor(HELP_TEXTBOX_X + 6, text_y);
  tft.print("use petroleum jelly on joining surfaces.\n");
  text_y += 12;
  tft.setCursor(HELP_TEXTBOX_X + 6, text_y);
  tft.print("4. To test, go back and press \"Perform a new test\".\n");
  text_y += 12;
  tft.setCursor(HELP_TEXTBOX_X + 6, text_y);
  tft.print("5. Enter a unique sample name and perform test.\n");
  text_y += 12;
  tft.setCursor(HELP_TEXTBOX_X + 6, text_y);
  tft.print("6. Results are saved on the SD card.\n");
  text_y += 12;
  tft.setCursor(HELP_TEXTBOX_X + 6, text_y);
  tft.print("7. Impulseresponses and FFTs can be seen in the");
  text_y += 12;
  tft.setCursor(HELP_TEXTBOX_X + 6, text_y);
  tft.print(" \"read data\" tab.\n");
  text_y += 12;
  tft.setCursor(HELP_TEXTBOX_X + 6, text_y);
  tft.print("8. Further analysis is made using MATLAB scripts.\n");
  text_y += 12;
  tft.setCursor(HELP_TEXTBOX_X + 6, text_y);
  tft.print("9. For details, see the documentation/report.");

  // --- Previous Page Button ---
  tft.fillRoundRect(HELP_BTN_X+6, HELP_BTN_Y, HELP_BTN_W, HELP_BTN_H, HELP_BTN_R, ILI9341_DARKGREY);
  tft.drawRoundRect(HELP_BTN_X+6, HELP_BTN_Y, HELP_BTN_W, HELP_BTN_H, HELP_BTN_R, ILI9341_BLACK);
  tft.setTextColor(ILI9341_BLACK);
  tft.setTextSize(2);
  tft.setCursor(HELP_BTN_X+6+6, HELP_BTN_Y+4);
  tft.print("Previous page");
}



// ---------- Page 2 ----------

bool p2_shiftActive = true;
// Lowercase and uppercase key layouts
const char* p2_row1_lower[] = { "q", "w", "e", "r", "t", "y", "u", "i", "o", "p" };
const char* p2_row1_upper[] = { "Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P" };
const char* p2_row2_lower[] = { "a", "s", "d", "f", "g", "h", "j", "k", "l" };
const char* p2_row2_upper[] = { "A", "S", "D", "F", "G", "H", "J", "K", "L" };
const char* p2_row3_lower[] = { "z", "x", "c", "v", "b", "n", "m" };
const char* p2_row3_upper[] = { "Z", "X", "C", "V", "B", "N", "M" };


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
  { "SHIFT", 30, 54 },
  { "CLEAR", 88, 54 },
  { "SPACE", 146, 86 },
  { "<", 236, 54 }
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

  const char** row1 = p2_shiftActive ? p2_row1_upper : p2_row1_lower;
  const char** row2 = p2_shiftActive ? p2_row2_upper : p2_row2_lower;
  const char** row3 = p2_shiftActive ? p2_row3_upper : p2_row3_lower;

  tft.setTextSize(2);
  for (int i = 0; i < 10; i++) {
    int x = p2_row1X + i * (p2_keyW + p2_keyGap);
    tft.fillRect(x, p2_row1Y, p2_keyW, p2_keyH, ILI9341_WHITE);
    tft.drawRect(x, p2_row1Y, p2_keyW, p2_keyH, ILI9341_BLACK);
    p2_drawCenteredText(row1[i], x, p2_row1Y, p2_keyW, p2_keyH, ILI9341_BLACK, 2);
  }
  for (int i = 0; i < 9; i++) {
    int x = p2_row2X + i * (p2_keyW + p2_keyGap);
    tft.fillRect(x, p2_row2Y, p2_keyW, p2_keyH, ILI9341_WHITE);
    tft.drawRect(x, p2_row2Y, p2_keyW, p2_keyH, ILI9341_BLACK);
    p2_drawCenteredText(row2[i], x, p2_row2Y, p2_keyW, p2_keyH, ILI9341_BLACK, 2);
  }
  for (int i = 0; i < 7; i++) {
    int x = p2_row3X + i * (p2_keyW + p2_keyGap);
    tft.fillRect(x, p2_row3Y, p2_keyW, p2_keyH, ILI9341_WHITE);
    tft.drawRect(x, p2_row3Y, p2_keyW, p2_keyH, ILI9341_BLACK);
    p2_drawCenteredText(row3[i], x, p2_row3Y, p2_keyW, p2_keyH, ILI9341_BLACK, 2);
  }
  // Special keys row
  for (int i = 0; i < 4; i++) {
    int x = p2_row4[i].x;
    int w = p2_row4[i].w;
    uint16_t fillColor = ILI9341_WHITE;
    // Highlight SHIFT if active
    if (i == 0 && p2_shiftActive) fillColor = ILI9341_BLUE;
    tft.fillRect(x, p2_row4Y, w, p2_keyH, fillColor);
    tft.drawRect(x, p2_row4Y, w, p2_keyH, ILI9341_BLACK);
    uint8_t keyTextSize = (strcmp(p2_row4[i].label, "SPACE") == 0 || strcmp(p2_row4[i].label, "CLEAR") == 0 || strcmp(p2_row4[i].label, "SHIFT") == 0) ? 1 : 2;
    p2_drawCenteredText(p2_row4[i].label, x, p2_row4Y, w, p2_keyH, ILI9341_BLACK, keyTextSize);
  }
  // Bottom buttons
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
  const char** row1 = p2_shiftActive ? p2_row1_upper : p2_row1_lower;
  const char** row2 = p2_shiftActive ? p2_row2_upper : p2_row2_lower;
  const char** row3 = p2_shiftActive ? p2_row3_upper : p2_row3_lower;

  // Row 1
  for (int i = 0; i < 10; i++) {
    int x = p2_row1X + i * (p2_keyW + p2_keyGap);
    if (p2_inRect(tx, ty, x, p2_row1Y, p2_keyW, p2_keyH)) {
      if (p2_sampleName.length() < 20) {
        p2_sampleName += row1[i];
        p2_drawSampleBox();
        // Auto-disable shift after single char
        if (p2_shiftActive) {
          p2_shiftActive = false;
          drawPage2();
        }
      }
      return;
    }
  }
  // Row 2
  for (int i = 0; i < 9; i++) {
    int x = p2_row2X + i * (p2_keyW + p2_keyGap);
    if (p2_inRect(tx, ty, x, p2_row2Y, p2_keyW, p2_keyH)) {
      if (p2_sampleName.length() < 20) {
        p2_sampleName += row2[i];
        p2_drawSampleBox();
        if (p2_shiftActive) {
          p2_shiftActive = false;
          drawPage2();
        }
      }
      return;
    }
  }
  // Row 3
  for (int i = 0; i < 7; i++) {
    int x = p2_row3X + i * (p2_keyW + p2_keyGap);
    if (p2_inRect(tx, ty, x, p2_row3Y, p2_keyW, p2_keyH)) {
      if (p2_sampleName.length() < 20) {
        p2_sampleName += row3[i];
        p2_drawSampleBox();
        if (p2_shiftActive) {
          p2_shiftActive = false;
          drawPage2();
        }
      }
      return;
    }
  }

  // Special keys
  // SHIFT
  if (p2_inRect(tx, ty, p2_row4[0].x, p2_row4Y, p2_row4[0].w, p2_keyH)) {
    p2_shiftActive = !p2_shiftActive;
    drawPage2();
    return;
  }
  // CLEAR
  if (p2_inRect(tx, ty, p2_row4[1].x, p2_row4Y, p2_row4[1].w, p2_keyH)) {
    p2_sampleName = "";
    p2_drawSampleBox();
    return;
  }
  // SPACE
  if (p2_inRect(tx, ty, p2_row4[2].x, p2_row4Y, p2_row4[2].w, p2_keyH)) {
    if (p2_sampleName.length() < 20) {
      p2_sampleName += "_";
      p2_drawSampleBox();
    }
    return;
  }
  // BACKSPACE
  if (p2_inRect(tx, ty, p2_row4[3].x, p2_row4Y, p2_row4[3].w, p2_keyH)) {
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

  for (uint8_t q = 0; q <= 3; q++) {
    switch (iteration) {
      case 0:
        digitalWrite(CH12Pin, HIGH);  // Switch the relay for channel 1 and 2
        digitalWrite(CH12Pin2, HIGH);
        delay(50);
        recordSine();
        delay(1000);
        recordPhaseShift();
        delay(1000);
        recordWhiteNoise();
        delay(1000);
        recordSineSweep();
        delay(1000);
        //recordMLS();
        delay(1000);
        digitalWrite(CH12Pin, LOW);  // Switch the relay for channel 1 and 2
        digitalWrite(CH12Pin2, LOW);
        p3_addToLogLN(" ");
        p3_addToLogLN("Recordings on channel 1/2 done.");
        p3_addToLogLN(" ");
        iteration++;
        delay(1000);
      case 1:
        digitalWrite(CH34Pin, HIGH);  // Switch the relay for channel 3 and 4
        digitalWrite(CH34Pin2, HIGH);
        delay(50);
        recordSine();
        delay(1000);
        recordPhaseShift();
        delay(1000);
        recordWhiteNoise();
        delay(1000);
        recordSineSweep();
        delay(1000);
        //recordMLS();
        delay(1000);
        digitalWrite(CH34Pin, LOW);  // Switch the relay for channel 3 and 4
        digitalWrite(CH34Pin2, LOW);
        p3_addToLogLN(" ");
        p3_addToLogLN("Recordings on channel 3/4 done.");
        p3_addToLogLN(" ");
        iteration++;
        delay(1000);
      case 2:
        digitalWrite(CH56Pin, HIGH);  // Switch the relay for channel 5 and 6
        digitalWrite(CH56Pin2, HIGH);
        delay(50);
        recordSine();
        delay(1000);
        recordPhaseShift();
        delay(1000);
        recordWhiteNoise();
        delay(1000);
        recordSineSweep();
        delay(1000);
        //recordMLS(); // Not functional :(
        delay(1000);
        digitalWrite(CH56Pin, LOW);  // Switch the relay for channel 5 and 6
        digitalWrite(CH56Pin2, LOW);
        p3_addToLogLN(" ");
        p3_addToLogLN("Recordings on channel 5/6 done.");
        p3_addToLogLN(" ");
        iteration++;
        delay(1000);
      case 3:
        p3_addToLogLN(" ");
        p3_addToLogLN("All recordings done!");
        p3_addToLogLN("Beginning FFT and IR computations.");
        p3_addToLogLN(" ");
        File root = SD.open("/");
        while (true) {  // Flips through all files on the SD card
          File entry = root.openNextFile();
          if (!entry) break;
          String name = entry.name();
          entry.close();
          // Process only CSVs matching the sample name
          if (name.endsWith(makeFilenameWithSample("CH12.csv")) || name.endsWith(makeFilenameWithSample("CH34.csv")) || name.endsWith(makeFilenameWithSample("CH56.csv"))) {  // Remember to set SAMPLENAME to match sample across all functions
            computeImpulseResponse(name.c_str());
          }
        }
        p3_addToLogLN("All files processed.");
        delay(5000);
    currentPage = PAGE_4;
        drawPage4();
        break; 
    }
        break; 
  }
}

void p3_addToLogLN(String line) {
  p3_logBuffer[p3_logIndex] = line;
  p3_logIndex = (p3_logIndex + 1) % P3_LOG_LINES;
  drawPage3Box();
  Serial.println(line);
}

void p3_addToLog(String line) {
  p3_logBuffer[p3_logIndex] = line;
  p3_logIndex = (p3_logIndex + 1) % P3_LOG_LINES;
  drawPage3Box();
  Serial.print(line);
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
      float values[20] = { 0 };
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
      float values[20] = { 0 };
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

      float values[20] = { 0 };
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
  float x_end = xs[n - 1];
  float x_span = x_end - x_start;
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
    int max_ms = (int)(xs[n - 1]) + 1;
    for (int tm = 0; tm <= 5; tm++) {
      int t_ms = tm * (max_ms / 5);
      int xp = x0 + (int)((t_ms - xs[0]) * w / (xs[n - 1] - xs[0]));
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
    int xp0 = x0 + (int)((xs[i - 1] - xs[0]) * w / x_span);
    int xp1 = x0 + (int)((xs[i] - xs[0]) * w / x_span);
    int yp0 = y0 + h - (int)((ys1[i - 1] - y_min) * h / y_span);
    int yp1 = y0 + h - (int)((ys1[i] - y_min) * h / y_span);
    tft.drawLine(xp0, yp0, xp1, yp1, ILI9341_BLUE);

    if ((mode == FFT_BOTH || mode == IMPULSE_BOTH)) {
      int yp0b = y0 + h - (int)((ys2[i - 1] - y_min) * h / y_span);
      int yp1b = y0 + h - (int)((ys2[i] - y_min) * h / y_span);
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
      // Left button: "Perform a new test"
      if (tx >= P1_BUTTON_LEFT_X && tx < P1_BUTTON_LEFT_X + P1_BUTTON_W && 
          ty >= P1_BUTTON_Y && ty < P1_BUTTON_Y + P1_BUTTON_H) {
        currentPage = PAGE_2;
        drawPage2();
        return;
      }
      // Middle button: "How to use the impedance tube in its current state"
      if (tx >= P1_BUTTON_MID_X && tx < P1_BUTTON_MID_X + P1_BUTTON_MID_W &&
          ty >= P1_BUTTON_Y && ty < P1_BUTTON_Y + P1_BUTTON_H) {
        currentPage = PAGE_HELP;
        drawHelpPage();
        return;
      }
      // Right button: "Read data from previous test"
      if (tx >= P1_BUTTON_RIGHT_X && tx < P1_BUTTON_RIGHT_X + P1_BUTTON_W &&
          ty >= P1_BUTTON_Y && ty < P1_BUTTON_Y + P1_BUTTON_H) {
        currentPage = PAGE_4;
        drawPage4();
        return;
      }
      break;
    case PAGE_HELP:
      // Previous Page button
      if (tx >= HELP_BTN_X && tx < HELP_BTN_X + HELP_BTN_W &&
          ty >= HELP_BTN_Y && ty < HELP_BTN_Y + HELP_BTN_H) {
        currentPage = PAGE_1;
        drawPage1();
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
  AudioMemory(1500);  // Way too much audiomemory set aside, but currently functional

  // SD setup
  if (!SD.begin(chipSelect)) {
    Serial.println("SD card initialization failed!");
    tft.fillScreen(ILI9341_WHITE);
    tft.setTextColor(ILI9341_RED);
    tft.setTextSize(2);
    tft.setCursor(30, 120);
    tft.print("SD card error!");

    while (1)
      ;
  }
  Serial.println("SD card initialized");

  // Pin setup
  pinMode(CH12Pin, OUTPUT);
  pinMode(CH34Pin, OUTPUT);
  pinMode(CH56Pin, OUTPUT);
  pinMode(CH12Pin2, OUTPUT);
  pinMode(CH34Pin2, OUTPUT);
  pinMode(CH56Pin2, OUTPUT);
  digitalWrite(CH12Pin, LOW);
  digitalWrite(CH34Pin, LOW);
  digitalWrite(CH56Pin, LOW);
  digitalWrite(CH12Pin2, LOW);
  digitalWrite(CH34Pin2, LOW);
  digitalWrite(CH56Pin2, LOW);

  // Audio setup
  sgtl5000_1.enable();
  sgtl5000_1.volume(1);
  mixer.gain(0, 0.0653);  // Gain for MLS
  mixer.gain(1, 0.0793);  // Gain for pure sine
  mixer.gain(2, 0.105);   // Gain for white noise
  mixer.gain(3, 0.078);   // Gain for sine sweep
  recordQueue.begin();
  mic1Queue.begin();
  mic2Queue.begin();

  delay(1000);  // Ensure system stabilizes

  tft.begin();
  tft.setRotation(1);
  ts.begin();
  ts.setRotation(3);

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

void recordSine() {
  p3_addToLogLN("Recording sine wave");
  sineWave.begin(WAVEFORM_SINE);  // Specifies a sine wave. Could also be sawtooth, square, triangle etc
  sineWave.amplitude(1);          // Sets the gain/amplitude of the sinusoid

  for (int f = 1; f <= 21; f++) {            // Increments frequency by 50 Hz
    sineWave.frequency(octaveFrequency(f));  // Sets the current frequency
    char filenameCombined[45];
    String tmpName;
    if (iteration == 0) {
      tmpName = makeFilenameWithSample(String("sine") + String(octaveFrequency(f)) + "SAMPLENAMECH12.csv");
      tmpName.toCharArray(filenameCombined, sizeof(filenameCombined));
    } else if (iteration == 1) {
      tmpName = makeFilenameWithSample(String("sine") + String(octaveFrequency(f)) + "SAMPLENAMECH34.csv");
      tmpName.toCharArray(filenameCombined, sizeof(filenameCombined));
    } else if (iteration == 2) {
      tmpName = makeFilenameWithSample(String("sine") + String(octaveFrequency(f)) + "SAMPLENAMECH34.csv");
      tmpName.toCharArray(filenameCombined, sizeof(filenameCombined));
    } else {
      p3_addToLogLN("Unknown iteration number - Shutting down!");
      while (true)
        ;
    }
    removeIfExists(filenameCombined);  // Deletes previous versions of the file, so that a new file is created, ensuring data integrity
    p3_addToLogLN("Currently recording: ");
    p3_addToLogLN(filenameCombined);
    p3_addToLogLN(" ");
    combinedFile = SD.open(filenameCombined, FILE_WRITE);  // Creates file on the SD card
    if (!combinedFile) {
      p3_addToLogLN("Failed to open file.");
      return;  // Exit the function or handle the error
    }
    recordThreeToFileSingleFile(combinedFile, sampleRate);  // Saves the current audio to SD
    combinedFile.close();                                   // Closes the file on SD
  }
  sineWave.amplitude(0);  // Turns off the sinusoid
  p3_addToLogLN("Sine done");
}

void recordPhaseShift() {
  String tmpName;
  p3_addToLogLN("Recording phase shift at 1kHz");
  sineWave.amplitude(1);                            // Sets the gain/amplitude of the sinusoid
  sineWave.frequency(1000);                         // Sets the frequency
  for (int phase = 0; phase <= 360; phase += 90) {  // Increments phase shift by 90 degrees
    sineWave.phase(0);                              // Initializes phase at 0 degrees
    char filenameCombined[45];
    if (iteration == 0) {
      tmpName = makeFilenameWithSample(String("sineShifted") + String(phase) + "SAMPLENAMECH12.csv");
      tmpName.toCharArray(filenameCombined, sizeof(filenameCombined));
    } else if (iteration == 1) {
      tmpName = makeFilenameWithSample(String("sineShifted") + String(phase) + "SAMPLENAMECH34.csv");
      tmpName.toCharArray(filenameCombined, sizeof(filenameCombined));
    } else if (iteration == 2) {
      tmpName = makeFilenameWithSample(String("sineShifted") + String(phase) + "SAMPLENAMECH56.csv");
      tmpName.toCharArray(filenameCombined, sizeof(filenameCombined));
    } else {
      p3_addToLogLN("Unknown iteration number - Shutting down!");
      while (true)
        ;
    }
    removeIfExists(filenameCombined);  // Deletes previous versions of the file, so that a new file is created, ensuring data integrity
    p3_addToLogLN("Currently recording: ");
    p3_addToLogLN(filenameCombined);
    p3_addToLogLN(" ");
    combinedFile = SD.open(filenameCombined, FILE_WRITE);  // Creates file on the SD card
    if (!combinedFile) {
      p3_addToLogLN("Failed to open file.");
      return;  // Exit the function or handle the error
    }
    recordThreeToFileSingleFile(combinedFile, sampleRate);  // Saves the current audio to SD
    combinedFile.close();                                   // Closes the file on SD
  }
  sineWave.amplitude(0);  // Turns off the sinusoid
  p3_addToLogLN("Phaseshift done");
}

void recordWhiteNoise() {
  String tmpName;
  p3_addToLogLN("Recording white noise");
  whiteNoise.amplitude(1);  // Sets the amplitude for white noise signal
  char filenameCombined[45];
  if (iteration == 0) {
    tmpName = makeFilenameWithSample(String("whiteNoise") + "SAMPLENAMECH12.csv");
    tmpName.toCharArray(filenameCombined, sizeof(filenameCombined));
    removeIfExists(filenameCombined);  // Deletes previous versions of the file, so that a new file is created, ensuring data integrity
    p3_addToLogLN("Currently recording: ");
    p3_addToLogLN(filenameCombined);
    p3_addToLogLN(" ");
    combinedFile = SD.open("whiteNoiseSAMPLENAMECH12.csv", FILE_WRITE);  // Creates file on the SD
  } else if (iteration == 1) {
    tmpName = makeFilenameWithSample(String("whiteNoise") + "SAMPLENAMECH34.csv");
    tmpName.toCharArray(filenameCombined, sizeof(filenameCombined));
    removeIfExists(filenameCombined);  // Deletes previous versions of the file, so that a new file is created, ensuring data integrity
    p3_addToLogLN("Currently recording: ");
    p3_addToLogLN(filenameCombined);
    p3_addToLogLN(" ");
    combinedFile = SD.open("whiteNoiseSAMPLENAMECH12.csv", FILE_WRITE);  // Creates file on the SD
  } else if (iteration == 2) {
    tmpName = makeFilenameWithSample(String("whiteNoise") + "SAMPLENAMECH56.csv");
    tmpName.toCharArray(filenameCombined, sizeof(filenameCombined));
    removeIfExists(filenameCombined);  // Deletes previous versions of the file, so that a new file is created, ensuring data integrity
    p3_addToLogLN("Currently recording: ");
    p3_addToLogLN(filenameCombined);
    p3_addToLogLN(" ");
    combinedFile = SD.open("whiteNoiseSAMPLENAMECH12.csv", FILE_WRITE);  // Creates file on the SD
  } else {
    p3_addToLogLN("Unknown iteration number - Shutting down!");
    while (true)
      ;
  }
  if (!combinedFile) {
    p3_addToLogLN("Failed to open file.");
    return;  // Exit the function or handle the error
  }
  recordThreeToFileSingleFile(combinedFile, sampleRate);  // Saves the current audio to SD
  combinedFile.close();                                   // Closes the file on SD
  whiteNoise.amplitude(0);                                // Turns off the white noise
  p3_addToLogLN("White noise done.");
}

void recordSineSweep() {
  String tmpName;
  p3_addToLogLN("Recording sine sweep");
  uint32_t sweepSamples = sineSweepTime * 44100;
  char filenameCombined[45];
  if (iteration == 0) {
    tmpName = makeFilenameWithSample(String("sweep") + "SAMPLENAMECH12.csv");
    tmpName.toCharArray(filenameCombined, sizeof(filenameCombined));
    removeIfExists(filenameCombined);  // Deletes previous versions of the file, so that a new file is created, ensuring data integrity
    p3_addToLogLN("Currently recording: ");
    p3_addToLogLN(filenameCombined);
    p3_addToLogLN(" ");
    combinedFile = SD.open("sweepSAMPLENAMECH12.csv", FILE_WRITE);  // Creates file on the SD
  } else if (iteration == 1) {
    tmpName = makeFilenameWithSample(String("sweep") + "SAMPLENAMECH34.csv");
    tmpName.toCharArray(filenameCombined, sizeof(filenameCombined));
    removeIfExists(filenameCombined);  // Deletes previous versions of the file, so that a new file is created, ensuring data integrity
    p3_addToLogLN("Currently recording: ");
    p3_addToLogLN(filenameCombined);
    p3_addToLogLN(" ");
    combinedFile = SD.open("sweepSAMPLENAMECH12.csv", FILE_WRITE);  // Creates file on the SD
  } else if (iteration == 2) {
    tmpName = makeFilenameWithSample(String("sweep") + "SAMPLENAMECH56.csv");
    tmpName.toCharArray(filenameCombined, sizeof(filenameCombined));
    removeIfExists(filenameCombined);  // Deletes previous versions of the file, so that a new file is created, ensuring data integrity
    p3_addToLogLN("Currently recording: ");
    p3_addToLogLN(filenameCombined);
    p3_addToLogLN(" ");
    combinedFile = SD.open("sweepSAMPLENAMECH12.csv", FILE_WRITE);  // Creates file on the SD
  } else {
    p3_addToLogLN("Unknown iteration number - Shutting down!");
    while (true)
      ;
  }
  if (!combinedFile) {
    p3_addToLogLN("Failed to open file.");
    return;  // Exit the function or handle the error
  }
  sineSweep.play(1, 20, 1220, sineSweepTime);               // Initializes sine sweep from 20Hz to 1220Hz over 1 second
  recordThreeToFileSingleFile(combinedFile, sweepSamples);  // Save both inputs in a 6Mics.csv file
  combinedFile.close();                                     // Closes the file on SD
  p3_addToLogLN("Sine sweep done.");
}

void recordThreeToFileSingleFile(File& file, uint32_t totalSamples) {  // Records three audioqueues to a .csv file
  uint32_t samplesRecorded = 0;                                        // Checker for when to stop
  while (samplesRecorded < totalSamples) {
    if (recordQueue.available() && mic1Queue.available() && mic2Queue.available()) {  // Checks for data on all signal lines, ensuring that data is present on all lines simultaneously
      int16_t* bufMixer = recordQueue.readBuffer();
      int16_t* bufMic1 = mic1Queue.readBuffer();
      int16_t* bufMic2 = mic2Queue.readBuffer();
      for (int i = 0; i < blockSize; i++) {  // Divides signals into blocks to ensure no overflow is happening
        float sMixer = (float)bufMixer[i] / 32768.0f;
        float sMic1 = (float)bufMic1[i] / 32768.0f;
        float sMic2 = (float)bufMic2[i] / 32768.0f;
        file.print(sMixer, 6);  // Saves mixer signal to SD card
        file.print(",");        // Saves to SD card
        file.print(sMic1, 6);   // Saves mic 1 signal to SD card
        file.print(",");        // Saves to SD card
        file.print(sMic2, 6);   // Saves mic2 signal to SD card
        file.print("\n");
      }
      recordQueue.freeBuffer();      // Frees buffer
      mic1Queue.freeBuffer();        // Frees buffer
      mic2Queue.freeBuffer();        // Frees buffer
      samplesRecorded += blockSize;  // Keeps track of how many samples has been saved
    }
  }
}


void removeIfExists(const char* filename) {  // Function to remove files, ensuring that new measurements are not just appended to previous
  if (SD.exists(filename)) {
    SD.remove(filename);
    p3_addToLog("Deleted previous version of: ");
    p3_addToLogLN(filename);
  }
}

uint16_t octaveFrequency(uint16_t octaveBand) {  // All 1/3 octave frequencies the impedance tube can measure
  switch (octaveBand) {
    case 1: return 12.5;
    case 2: return 16;
    case 3: return 20;
    case 4: return 25;
    case 5: return 31.5;
    case 6: return 40;
    case 7: return 50;
    case 8: return 63;
    case 9: return 80;
    case 10: return 100;
    case 11: return 125;
    case 12: return 160;
    case 13: return 200;
    case 14: return 250;
    case 15: return 315;
    case 16: return 400;
    case 17: return 500;
    case 18: return 630;
    case 19: return 800;
    case 20: return 1000;
    case 21: return 1250;  // Might have modal irregularities
    default:
      Serial.println("Unsupported bit length!");
      return 0;
  }
}

// Feedback tap map for various left-shifting LFSR lengths (primitive polynomials) with correct taps
uint32_t feedbackTaps(uint8_t bits) {
  switch (bits) {
    case 2: return (1 << 1) | (1 << 0);                             // x^2 + x + 1
    case 3: return (1 << 2) | (1 << 0);                             // x^3 + x + 1
    case 4: return (1 << 3) | (1 << 0);                             // x^4 + x + 1
    case 5: return (1 << 4) | (1 << 2);                             // x^5 + x^3 + 1
    case 6: return (1 << 5) | (1 << 4);                             // x^6 + x^5 + 1
    case 7: return (1 << 6) | (1 << 5);                             // x^7 + x^6 + 1
    case 8: return (1 << 7) | (1 << 5) | (1 << 4) | (1 << 3);       // x^8 + x^6 + x^5 + x^4 + 1
    case 9: return (1 << 8) | (1 << 4);                             // x^9 + x^5 + 1
    case 10: return (1 << 9) | (1 << 6);                            // x^10 + x^7 + 1
    case 11: return (1 << 10) | (1 << 8);                           // x^11 + x^9 + 1
    case 12: return (1 << 11) | (1 << 5) | (1 << 3) | (1 << 0);     // x^12 + x^6 + x^4 + x + 1
    case 13: return (1 << 12) | (1 << 3) | (1 << 2) | (1 << 0);     // x^13 + x^4 + x^3 + x + 1
    case 14: return (1 << 13) | (1 << 12) | (1 << 11) | (1 << 1);   // x^14 + x^13 + x^3 + x 1
    case 15: return (1 << 14) | (1 << 13);                          // x^15 + x^14 + 1
    case 16: return (1 << 15) | (1 << 13) | (1 << 12) | (1 << 10);  // x^16 + x^14 + x^13 + x^11 + 1
    case 17: return (1 << 16) | (1 << 13);                          // x^17 + x^14 + 1
    case 18: return (1 << 17) | (1 << 10);                          // x^18 + x^11 + 1
    case 19: return (1 << 18) | (1 << 17) | (1 << 16) | (1 << 13);  // x^19 + x^18 + x^16 + x^14 + 1
    case 20: return (1 << 19) | (1 << 16);                          // x^20 + x^17 + 1
    case 21: return (1 << 20) | (1 << 18);                          // x^21 + x^19 + 1
    case 22: return (1 << 21) | (1 << 20);                          // x^22 + x^21 + 1
    case 23: return (1 << 22) | (1 << 17);                          // x^23 + x^18 + 1
    case 24: return (1 << 23) | (1 << 22) | (1 << 21) | (1 << 16);  // x^24 + x^23 + x^22 + x^17 + 1
    case 25: return (1 << 24) | (1 << 21);                          // x^25 + x^22 + 1
    case 26: return (1 << 25) | (1 << 5) | (1 << 1) | (1 << 0);     // x^26 + x^6 + x^2 + x 1
    case 27: return (1 << 26) | (1 << 4) | (1 << 1) | (1 << 0);     // x^27 + x^5 + x^2 + x + 1
    case 28: return (1 << 27) | (1 << 24);                          // x^28 + x^25 + 1
    case 29: return (1 << 28) | (1 << 26);                          // x^29 + x^27 + 1
    case 30: return (1 << 29) | (1 << 5) | (1 << 3) | (1 << 0);     // x^30 + x^6 + x^4 + x + 1
    case 31: return (1 << 30) | (1 << 27);                          // x^31 + x^28 + 1
    case 32: return (1 << 31) | (1 << 21) | (1 << 1) | (1 << 0);    // x^32 + x^22 + x^2 + x + 1
    default:
      Serial.println("Unsupported bit length!");
      return 0;
  }
}

void recordMLS() {
  Serial.println("Recording MLS");
  char filename[45];
  if (iteration == 0) {
    removeIfExists("15BitMLSSAMPLENAMECH12.csv");                      // Deletes previous versions of the file, so that a new file is created, ensuring data integrity
    combinedFile = SD.open("15BitMLSSAMPLENAMECH12.csv", FILE_WRITE);  // Creates file on the SD
  } else if (iteration == 1) {
    removeIfExists("15BitMLSSAMPLENAMECH34.csv");                      // Deletes previous versions of the file, so that a new file is created, ensuring data integrity
    combinedFile = SD.open("15BitMLSSAMPLENAMECH34.csv", FILE_WRITE);  // Creates file on the SD
  } else if (iteration == 2) {
    removeIfExists("15BitMLSSAMPLENAMECH56.csv");                      // Deletes previous versions of the file, so that a new file is created, ensuring data integrity
    combinedFile = SD.open("15BitMLSSAMPLENAMECH56.csv", FILE_WRITE);  // Creates file on the SD
  } else {
    Serial.println("Unknown iteration number - Shutting down!");
    while (true)
      ;
  }
  if (!combinedFile) {
    Serial.println("Failed to open file.");
    return;  // Exit the function or handle the error
  }
  generateMLS();                                          // Sets the amplitude for white noise signal
  recordThreeToFileSingleFile(combinedFile, sampleRate);  // Saves the current audio to SD
  combinedFile.close();                                   // Closes the file on SD
  Serial.println("MLS done.");
}

void playMLSBit(bool bit, int samplesPerBit = 1, int amplitude = 28000) {  // Bool is the actual input parameter of the function (0 or 1), while samplesPerBit and amplitude are static. Amplitude is set at 28000 to not risk clipping the signal on a speaker
  static int16_t buffer[128];
  static int bufferIndex = 0;

  int16_t value = bit ? amplitude : -amplitude;  // Ternary operator deciding to set the amplitude at + or - the given amplitude based on boolean value

  for (int i = 0; i < samplesPerBit; i++) {
    buffer[bufferIndex++] = value;

    if (bufferIndex == 128) {                                 // Buffer set to 128 bits, conforming to I2S standards
      memcpy(MLSSignal.getBuffer(), buffer, sizeof(buffer));  // Copies buffer data to the AudioPlayQueue, enabling the Teensy to play the MLS
      MLSSignal.playBuffer();                                 // Proprietary play function for the AudioPlayQueue
      bufferIndex = 0;                                        // Resets the bufferindex, so a new sequence can be initialized
    }
  }
}

void generateMLS() {
  if (LFSRBits < 2 || LFSRBits > 32) {
    Serial.println("LFSRBits must be between 2 and 32.");
    while (1)
      ;
  }
  mask = (1UL << LFSRBits) - 1;
  LFSR = mask;
  uint32_t taps = feedbackTaps(LFSRBits);
  Serial.print("Generating MLS with ");
  Serial.print(LFSRBits);
  Serial.println(" bits:");
  Serial.print("The MLS should be ");
  Serial.print((1UL << LFSRBits) - 1);
  Serial.println(" bits long.");
  uint32_t startMLSTime = micros();  // Timer for MLS generation
  uint32_t MLSLength = (1UL << LFSRBits) - 1;
  uint32_t increment = 1048576;
  for (uint32_t start = 0; start < MLSLength; start += increment) {
    uint32_t end = start + increment;
    if (end > MLSLength) {
      end = MLSLength;
    }
    for (uint32_t i = start; i < end; i++) {
      bool feedback = __builtin_parity(LFSR & taps);
      //Serial.print(feedback ? 1 : 0);
      //logFile.print(feedback ? 1 : 0);
      playMLSBit(feedback);
      LFSR <<= 1;
      if (feedback) {
        LFSR |= 1;
      }
      LFSR &= mask;
    }
  }

  uint32_t endMLSTime = micros();  // Timer end to monitor execution time
  Serial.println("\nMLS generation complete.");
  Serial.print("It has taken ");
  Serial.print(endMLSTime - startMLSTime);  // Total execution time given in serial monitor
  Serial.println(" microseconds to calculate and print.");
}

// Compute and save impulse responses for two mics from a given CSV
void computeImpulseResponse(const char* inputFilename) {
  p3_addToLogLN(" ");
  p3_addToLogLN("Now computing impulse response and FFT of:");
  p3_addToLogLN(inputFilename);
  File infile = SD.open(inputFilename);
  if (!infile) {
    p3_addToLogLN("Failed to open input file!");
    return;
  }

  const size_t max_samples = 32768;
  float* x = (float*)extmem_malloc(max_samples * sizeof(float));   // mixer
  float* y1 = (float*)extmem_malloc(max_samples * sizeof(float));  // mic1
  float* y2 = (float*)extmem_malloc(max_samples * sizeof(float));  // mic2
  if (!x || !y1 || !y2) {
    p3_addToLogLN("Memory allocation failed for input arrays");
    return;
  }

  // Read CSV: mixer, mic1, mic2
  size_t N = 0;
  while (infile.available() && N < max_samples) {
    String line = infile.readStringUntil('\n');
    int idx1 = line.indexOf(',');
    int idx2 = line.indexOf(',', idx1 + 1);
    if (idx1 > 0 && idx2 > idx1) {
      x[N] = line.substring(0, idx1).toFloat();
      y1[N] = line.substring(idx1 + 1, idx2).toFloat();
      y2[N] = line.substring(idx2 + 1).toFloat();
      N++;
    }
  }
  infile.close();
  Serial.printf("Loaded %zu samples from %s\n", N, inputFilename);

  // FFT length = next power of two of 2*N
  size_t Nfft = next_power_of_2(2 * N);
  Serial.printf("FFT length: %zu\n", Nfft);

  // Allocate FFT buffers
  kiss_fft_cpx* X = (kiss_fft_cpx*)extmem_malloc(Nfft * sizeof(kiss_fft_cpx));
  kiss_fft_cpx* Y1 = (kiss_fft_cpx*)extmem_malloc(Nfft * sizeof(kiss_fft_cpx));
  kiss_fft_cpx* Y2 = (kiss_fft_cpx*)extmem_malloc(Nfft * sizeof(kiss_fft_cpx));
  kiss_fft_cpx* H1 = (kiss_fft_cpx*)extmem_malloc(Nfft * sizeof(kiss_fft_cpx));
  kiss_fft_cpx* H2 = (kiss_fft_cpx*)extmem_malloc(Nfft * sizeof(kiss_fft_cpx));
  kiss_fft_cpx* H1c = (kiss_fft_cpx*)extmem_malloc(Nfft * sizeof(kiss_fft_cpx));
  kiss_fft_cpx* H2c = (kiss_fft_cpx*)extmem_malloc(Nfft * sizeof(kiss_fft_cpx));
  kiss_fft_cpx* h1t = (kiss_fft_cpx*)extmem_malloc(Nfft * sizeof(kiss_fft_cpx));
  kiss_fft_cpx* h2t = (kiss_fft_cpx*)extmem_malloc(Nfft * sizeof(kiss_fft_cpx));
  if (!X || !Y1 || !Y2 || !H1 || !H2 || !H1c || !H2c || !h1t || !h2t) {
    p3_addToLogLN("Memory allocation failed for FFT buffers");
    return;
  }

  // Zero-pad inputs
  for (size_t i = 0; i < Nfft; i++) {
    X[i].r = (i < N ? x[i] : 0.0f);
    X[i].i = 0.0f;
    Y1[i].r = (i < N ? y1[i] : 0.0f);
    Y1[i].i = 0.0f;
    Y2[i].r = (i < N ? y2[i] : 0.0f);
    Y2[i].i = 0.0f;
  }

  // Create FFT configs
  kiss_fft_cfg fwd = kiss_fft_alloc_psram(Nfft, 0);
  kiss_fft_cfg inv = kiss_fft_alloc_psram(Nfft, 1);
  if (!fwd || !inv) {
    p3_addToLogLN("KissFFT config alloc failed");
    return;
  }

  // Forward FFTs
  kiss_fft_psram(fwd, X, X);
  kiss_fft_psram(fwd, Y1, Y1);
  kiss_fft_psram(fwd, Y2, Y2);
  p3_addToLogLN("FFTs Done");

  // Frequency response: H1 = Y1/X, H2 = Y2/X
  for (size_t i = 0; i < Nfft; i++) {
    float xr = X[i].r, xi = X[i].i;
    float denom = xr * xr + xi * xi + epsilon;
    // mic1
    {
      float yr = Y1[i].r, yi = Y1[i].i;
      H1[i].r = (yr * xr + yi * xi) / denom;
      H1[i].i = (yi * xr - yr * xi) / denom;
    }
    // mic2
    {
      float yr = Y2[i].r, yi = Y2[i].i;
      H2[i].r = (yr * xr + yi * xi) / denom;
      H2[i].i = (yi * xr - yr * xi) / denom;
    }
  }

  // Copy for output
  memcpy(H1c, H1, Nfft * sizeof(kiss_fft_cpx));
  memcpy(H2c, H2, Nfft * sizeof(kiss_fft_cpx));

  // Inverse FFT to get impulse responses
  kiss_fft(inv, H1, h1t);
  kiss_fft(inv, H2, h2t);
  // Normalize
  for (size_t i = 0; i < Nfft; i++) {
    h1t[i].r /= (float)Nfft;
    h2t[i].r /= (float)Nfft;
  }
  p3_addToLogLN("IFFTs Done");

  // Write CSV
  String outName = String(inputFilename);
  outName.replace(".csv", "IRAndFFT.csv");
  removeIfExists(outName.c_str());
  File out = SD.open(outName.c_str(), FILE_WRITE);
  out.println("FreqHz,Xreal,Ximag,Y1real,Y1imag,H1real,H1imag,H1magdB,H1phaserad,h1Impulse,Y2real,Y2imag,H2real,H2imag,H2magdB,H2phaserad,h2Impulse");

  float freqRes = (float)sampleRate / Nfft;
  size_t Nu = Nfft / 2 + 1;
  for (size_t i = 0; i < Nu; i++) {
    float freq = i * freqRes;
    float xr = X[i].r, xi = X[i].i;
    // mic1
    float y1r = Y1[i].r, y1i = Y1[i].i;
    float h1r = H1c[i].r, h1i = H1c[i].i;
    float mag1 = abs(sqrtf(h1r * h1r + h1i * h1i));
    float dB1 = 20.0f * log10f(mag1 + epsilon);
    float ph1 = atan2f(h1i, h1r);
    float imp1 = h1t[i].r;
    // mic2
    float y2r = Y2[i].r, y2i = Y2[i].i;
    float h2r = H2c[i].r, h2i = H2c[i].i;
    float mag2 = abs(sqrtf(h2r * h2r + h2i * h2i));
    float dB2 = 20.0f * log10f(mag2 + epsilon);
    float ph2 = atan2f(h2i, h2r);
    float imp2 = h2t[i].r;

    out.printf("%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f\n",
               freq, xr, xi,
               y1r, y1i, h1r, h1i, dB1, ph1, imp1,
               y2r, y2i, h2r, h2i, dB2, ph2, imp2);
  }
  out.close();
  p3_addToLog("Output written to");
  p3_addToLogLN(outName.c_str());

  // Cleanup
  extmem_free(x);
  extmem_free(y1);
  extmem_free(y2);
  extmem_free(X);
  extmem_free(Y1);
  extmem_free(Y2);
  extmem_free(H1);
  extmem_free(H2);
  extmem_free(H1c);
  extmem_free(H2c);
  extmem_free(h1t);
  extmem_free(h2t);
  extmem_free(fwd);
  extmem_free(inv);
  p3_addToLogLN("Cleanup Done!");
}
