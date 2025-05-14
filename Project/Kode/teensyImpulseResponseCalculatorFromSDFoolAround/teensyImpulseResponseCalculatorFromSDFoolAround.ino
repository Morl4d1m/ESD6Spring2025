
#include <Audio.h>
#include <Wire.h>
#include <SD.h>
#include <SPI.h>
#include <SerialFlash.h>
#include <arm_math.h>
#include <arm_const_structs.h>

static const float EPS = 1e-10f;
const size_t BLOCK = 1024;
const size_t NFFT = 2 * BLOCK;
const size_t TOTAL_SAMPLES = 512 * BLOCK;  // 524,288 samples
// Output buffer to accumulate the impulse response (overlap-add)
static float ir_full[TOTAL_SAMPLES] = { 0 };

// —————————————————————————————————————————————
// Buffers (statically allocated so as not to fragment the heap):
// —————————————————————————————————————————————
// Static buffers
static float y_block[2 * BLOCK];  // overlap buffer
static float fft_buf[2 * NFFT];   // interleaved real/imag
static float Hinv[2 * NFFT];      // accumulated inverse filter
static float conv[2 * NFFT];      // for frequency‑domain multiply

// SD card stuff
const int chipSelect = BUILTIN_SDCARD;
File mixerFile;
File micFile;
File combinedFile;
const char* filename = "sweepCombined.csv";
const int fs = 44100;
const float epsilon = 1e-10f;

// Utility: Compute next power of 2
size_t next_power_of_2(size_t n) {
  size_t p = 1;
  while (p < n) p <<= 1;
  return p;
}


void removeIfExists(const char* filename) {
  if (SD.exists(filename)) {
    SD.remove(filename);
    Serial.print("Deleted previous version of: ");
    Serial.println(filename);
  }
}


void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  // SD setup
  if (!SD.begin(chipSelect)) {
    Serial.println("SD card initialization failed!");
    while (1)
      ;
  }
  Serial.println("SD card initialized");
  delay(1000);  // Ensure system stabilizes
  Serial.println("Setup done");
  removeIfExists("fft_and_impulse.csv");
  computeImpulseResponseStreaming();
}

void loop() {
  // put your main code here, to run repeatedly:
}
/*
void computeImpulseResponse() {
  if (!SD.begin(BUILTIN_SDCARD)) {
    Serial.println("SD initialization failed!");
    return;
  }
  precomputeInverseFilter();
  overlapSaveDeconv();

  File file = SD.open(filename);
  if (!file) {
    Serial.println("Failed to open file!");
    return;
  }

  const size_t max_samples = 2048;  // Safe for Teensy 4.1
  float* x = (float*)malloc(max_samples * sizeof(float));
  float* y = (float*)malloc(max_samples * sizeof(float));
  size_t N = 0;

  if (!x || !y) {
    Serial.println("Memory allocation failed (x/y)");
    return;
  }
  Serial.println("Memory allocation successful");

  while (file.available() && N < max_samples) {
    String line = file.readStringUntil('\n');
    int commaIndex = line.indexOf(',');
    if (commaIndex > 0) {
      float val1 = line.substring(0, commaIndex).toFloat();
      float val2 = line.substring(commaIndex + 1).toFloat();
      x[N] = val1;
      y[N] = val2;
      N++;
    }
  }
  file.close();
  Serial.printf("Loaded %zu samples\n", N);

  size_t Nfft = next_power_of_2(2 * N);
  Serial.printf("FFT length: %zu\n", Nfft);

  float* X = (float*)calloc(2 * Nfft, sizeof(float));
  float* Y = (float*)calloc(2 * Nfft, sizeof(float));
  float* H = (float*)calloc(2 * Nfft, sizeof(float));
  float* h = (float*)malloc(Nfft * sizeof(float));

  if (!X || !Y || !H || !h) {
    Serial.println("Memory allocation failed (FFT buffers)");
    return;
  }
  Serial.println("Memory allocation successful (FFT buffers)");

  //Interleave x,y into X[],Y[]
  for (size_t i = 0; i < Nfft; i++) {
    X[2 * i] = (i < N) ? x[i] : 0.0f;
    X[2 * i + 1] = 0.0f;
    Y[2 * i] = (i < N) ? y[i] : 0.0f;
    Y[2 * i + 1] = 0.0f;
  }
  Serial.println("Interleaving complete");

  // === FFT (Radix-4) ===
  arm_cfft_radix4_instance_f32 S;
  arm_cfft_radix4_init_f32(&S, Nfft, 0, 1);
  Serial.println("FFT init done");

  arm_cfft_radix4_f32(&S, X);
  Serial.println("FFT X done");
  arm_cfft_radix4_f32(&S, Y);
  Serial.println("FFT Y done");

  for (size_t i = 0; i < Nfft; i++) {
    float xr = X[2 * i];
    float xi = X[2 * i + 1];
    float yr = Y[2 * i];
    float yi = Y[2 * i + 1];

    float denom = xr * xr + xi * xi + epsilon;
    H[2 * i] = (yr * xr + yi * xi) / denom;
    H[2 * i + 1] = (yi * xr - yr * xi) / denom;
  }
  Serial.println("Deconvolution done");

  // 7) **Copy** frequency‑domain H → Hfreq **before** IFFT
  float* Hfreq = (float*)malloc(2 * Nfft * sizeof(float));
  if (!Hfreq) {
    Serial.println("Hfreq alloc failed");
    return;
  }
  memcpy(Hfreq, H, 2 * Nfft * sizeof(float));
  arm_cfft_radix4_init_f32(&S, Nfft, 1, 1);
  arm_cfft_radix4_f32(&S, H);
  Serial.println("IFFT done");
  // 9) Scale by 1/Nfft
  for (size_t k = 0; k < Nfft; k++) {
    H[2 * k] /= (float)Nfft;
    H[2 * k + 1] /= (float)Nfft;
  }

  for (size_t i = 0; i < N; i++) {
    h[i] = H[2 * i];
  }
  Serial.println("Extract real part done");

  // === Save full FFT data + impulse response ===
  File combinedOut = SD.open("fft_and_impulse.csv", FILE_WRITE);
  if (!combinedOut) {
    Serial.println("Failed to open fft_and_impulse.csv!");
    return;
  }

  // Header
  combinedOut.println("Freq_Hz,X_real,X_imag,Y_real,Y_imag,H_real,H_imag,H_mag_dB,H_phase_rad,Impulse_response");

  float freqRes = (float)fs / Nfft;
  size_t N_unique = Nfft / 2 + 1;

  for (size_t i = 0; i < N_unique; i++) {
    float freq = i * freqRes;

    float xr = X[2 * i];
    float xi = X[2 * i + 1];
    float yr = Y[2 * i];
    float yi = Y[2 * i + 1];
    float hr = Hfreq[2 * i];      // Find hr from the frequency domain bins
    float hi = Hfreq[2 * i + 1];  // Find hi from the frequency domain bins


    float x_mag = sqrtf(xr * xr + xi * xi);
    float y_mag = sqrtf(yr * yr + yi * yi);
    float h_mag = sqrtf(hr * hr + hi * hi);

    float mag_dB = abs(20.0f * log10f((y_mag + epsilon) / (x_mag + epsilon)));
    float phase = atan2f(hi, hr);
    float h_val = (i < N) ? h[i] : 0.0f;

    combinedOut.printf("%f,%f,%f,%f,%f,%f,%f,%f,%f,%f\n", freq, xr, xi, yr, yi, hr, hi, mag_dB, phase, h_val);
  }

  combinedOut.close();
  Serial.println("All FFT data + impulse response written to fft_and_impulse.csv");

  free(x);
  free(y);
  free(X);
  free(Y);
  free(H);
  free(h);
  Serial.println("Cleanup done");
}
*/
void computeImpulseResponseStreaming() {
  Serial.println("=== Starting Streaming Impulse Response ===");

  if (!SD.begin(BUILTIN_SDCARD)) {
    Serial.println("SD card init failed");
    return;
  }

  if (SD.exists("impulse_segmented.csv")) SD.remove("impulse_segmented.csv");
  if (SD.exists("fft_and_impulse.csv")) SD.remove("fft_and_impulse.csv");

  File in = SD.open(filename);
  File impulseOut = SD.open("impulse_segmented.csv", FILE_WRITE);
  File freqOut = SD.open("fft_and_impulse.csv", FILE_WRITE);

  if (!in || !impulseOut || !freqOut) {
    Serial.println("File open failed");
    return;
  }

  freqOut.println("Freq_Hz,X_real,X_imag,Y_real,Y_imag,H_real,H_imag,H_mag_dB,H_phase_rad,Impulse_response");

  // Buffers inside stack or `.bss` (must stay below ~500 KB total)
  static float xbuf[BLOCK], ybuf[BLOCK];
  static float overlap[BLOCK] = { 0 };  // Save last BLOCK samples of each h[n]
  static float in_fft[NFFT * 2], out_fft[NFFT * 2], hbuf[NFFT];

  arm_cfft_radix4_instance_f32 fft;
  size_t blockIndex = 0;

  while (blockIndex * BLOCK < TOTAL_SAMPLES) {
    Serial.printf("Block %u\n", blockIndex);

    // Read BLOCK samples
    for (size_t i = 0; i < BLOCK; i++) {
      Serial.println(i);
      if (in.available()) {
        String line = in.readStringUntil('\n');
        int comma = line.indexOf(',');
        xbuf[i] = line.substring(0, comma).toFloat();   // X[n]
        ybuf[i] = line.substring(comma + 1).toFloat();  // Y[n]
      } else {
        xbuf[i] = ybuf[i] = 0.0f;
      }
    }
    Serial.println("Read block");

    // Fill complex arrays for FFT
    for (size_t k = 0; k < NFFT; k++) {
      in_fft[2 * k] = (k < BLOCK) ? xbuf[k] : 0.0f;
      in_fft[2 * k + 1] = 0.0f;
      out_fft[2 * k] = (k < BLOCK) ? ybuf[k] : 0.0f;
      out_fft[2 * k + 1] = 0.0f;
    }
    Serial.println("Filled complex arrays");
    

    // Forward FFT
    arm_cfft_radix4_init_f32(&fft, NFFT, 0, 1);
    Serial.println("Init fft");
    arm_cfft_radix4_f32(&fft, in_fft);
    Serial.println("fft in");
    arm_cfft_radix4_f32(&fft, out_fft);
    Serial.println("Prepped FFT");

    // Frequency domain division H = Y / X
    float H[NFFT * 2];
    for (size_t k = 0; k < NFFT; k++) {
      float xr = in_fft[2 * k], xi = in_fft[2 * k + 1];
      float yr = out_fft[2 * k], yi = out_fft[2 * k + 1];
      float denom = xr * xr + xi * xi + EPS;
      H[2 * k] = (yr * xr + yi * xi) / denom;
      H[2 * k + 1] = (yi * xr - yr * xi) / denom;
    }
    Serial.println("freq domain divided");

    // IFFT to get h[n]
    arm_cfft_radix4_init_f32(&fft, NFFT, 1, 1);
    arm_cfft_radix4_f32(&fft, H);
    Serial.println("Prepped IFFT");

    // Convert to real time-domain impulse response
    for (size_t i = 0; i < NFFT; i++) {
      hbuf[i] = H[2 * i] / NFFT;
    }

    // Overlap-add: combine with previous segment’s tail
    for (size_t i = 0; i < BLOCK; i++) {
      hbuf[i] += overlap[i];
    }

    // Save last BLOCK samples for next overlap
    memcpy(overlap, hbuf + BLOCK, sizeof(float) * BLOCK);

    // Save this segment (BLOCK samples)
    for (size_t i = 0; i < BLOCK; i++) {
      impulseOut.printf("%lu,%f\n", (unsigned long)(blockIndex * BLOCK + i), hbuf[i]);
    }

    // Save frequency-domain info (only once — first block)
    if (blockIndex == 0) {
      float freqRes = fs / NFFT;
      for (size_t i = 0; i <= NFFT / 2; i++) {
        float freq = i * freqRes;
        float xr = in_fft[2 * i], xi = in_fft[2 * i + 1];
        float yr = out_fft[2 * i], yi = out_fft[2 * i + 1];
        float hr = H[2 * i], hi = H[2 * i + 1];
        float mag_dB = 20.0f * log10f(sqrtf(hr * hr + hi * hi) + EPS);
        float phase = atan2f(hi, hr);
        float imp = hbuf[i];
        freqOut.printf("%f,%f,%f,%f,%f,%f,%f,%f,%f,%f\n",
                       freq, xr, xi, yr, yi, hr, hi, mag_dB, phase, imp);
      }
    }

    blockIndex++;
  }

  in.close();
  impulseOut.close();
  freqOut.close();

  Serial.println("=== Finished Streaming Impulse Response ===");
}



//------------------------------------------------------------------
// Precompute Hinv[k] = average of 1/(Xblock[k]+eps) over the entire mixer sweep
//------------------------------------------------------------------
void precomputeInverseFilter() {
  File f = SD.open(filename);
  if (!f) {
    Serial.println("Cannot open input");
    return;
  }

  memset(y_block, 0, sizeof(y_block));
  for (size_t i = 0; i < 2 * NFFT; i++) Hinv[i] = 0.0f;
  size_t nBlocks = 0;

  arm_cfft_radix4_instance_f32 fft;
  arm_cfft_radix4_init_f32(&fft, NFFT, 0, 1);

  while (true) {
    // overlap shift
    memmove(y_block, y_block + BLOCK, BLOCK * sizeof(float));
    // read mixer samples
    for (size_t i = 0; i < BLOCK; i++) {
      if (!f.available()) goto donePre;
      String L = f.readStringUntil('\n');
      int c = L.indexOf(',');
      y_block[BLOCK + i] = L.substring(0, c).toFloat();
    }
    // build FFT input
    for (size_t k = 0; k < NFFT; k++) {
      if (k < 2 * BLOCK) {
        fft_buf[2 * k] = y_block[k];
        fft_buf[2 * k + 1] = 0;
      } else {
        fft_buf[2 * k] = 0;
        fft_buf[2 * k + 1] = 0;
      }
    }
    // FFT
    arm_cfft_radix4_f32(&fft, fft_buf);
    // accumulate 1/(X+eps)
    for (size_t k = 0; k < NFFT; k++) {
      float xr = fft_buf[2 * k], xi = fft_buf[2 * k + 1];
      float d = xr * xr + xi * xi;
      if (d < EPS) d = EPS;
      Hinv[2 * k] += xr / d;
      Hinv[2 * k + 1] += -xi / d;
    }
    nBlocks++;
  }
donePre:
  f.close();
  // average
  for (size_t i = 0; i < 2 * NFFT; i++) {
    Hinv[i] /= float(nBlocks);
  }
  Serial.printf("Precomputed Hinv over %u blocks\n", (unsigned)nBlocks);
}

//------------------------------------------------------------------
// Overlap–save deconvolution of mic channel.
// Streams each impulse sample as soon as it's computed.
//------------------------------------------------------------------
void overlapSaveStreamImpulse() {
  File in = SD.open(filename);
  File out = SD.open("fft_and_impulse.csv", FILE_WRITE);
  if (!in || !out) {
    Serial.println("File open error");
    if (in) in.close();
    return;
  }
  out.println("SampleIndex,ImpulseValue");

  memset(y_block, 0, sizeof(y_block));
  size_t idx = 0;

  arm_cfft_radix4_instance_f32 fft;
  // forward FFT plan
  arm_cfft_radix4_init_f32(&fft, NFFT, 0, 1);

  while (idx < TOTAL_SAMPLES) {
    // overlap shift
    memmove(y_block, y_block + BLOCK, BLOCK * sizeof(float));
    // read mic samples
    for (size_t i = 0; i < BLOCK; i++) {
      if (!in.available()) {
        y_block[BLOCK + i] = 0;
      } else {
        String L = in.readStringUntil('\n');
        int c = L.indexOf(',');
        y_block[BLOCK + i] = L.substring(c + 1).toFloat();
      }
    }
    // build FFT input
    for (size_t k = 0; k < NFFT; k++) {
      if (k < 2 * BLOCK) {
        fft_buf[2 * k] = y_block[k];
        fft_buf[2 * k + 1] = 0;
      } else {
        fft_buf[2 * k] = 0;
        fft_buf[2 * k + 1] = 0;
      }
    }
    // FFT
    arm_cfft_radix4_f32(&fft, fft_buf);

    // multiply by Hinv
    for (size_t k = 0; k < NFFT; k++) {
      float yr = fft_buf[2 * k], yi = fft_buf[2 * k + 1];
      float hr = Hinv[2 * k], hi = Hinv[2 * k + 1];
      conv[2 * k] = yr * hr - yi * hi;
      conv[2 * k + 1] = yr * hi + yi * hr;
    }

    // IFFT plan
    arm_cfft_radix4_init_f32(&fft, NFFT, 1, 1);
    arm_cfft_radix4_f32(&fft, conv);

    // scale & stream last BLOCK samples
    for (size_t i = 0; i < BLOCK && idx < TOTAL_SAMPLES; i++, idx++) {
      float imp = conv[2 * (BLOCK + i)] / float(NFFT);
      out.printf("%u,%f\n", (unsigned)idx, imp);
    }
  }

  in.close();
  out.close();
  Serial.println("Streaming complete");
}