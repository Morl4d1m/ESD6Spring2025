#include <Audio.h>
#include <SD.h>
#include <SPI.h>

AudioInputI2S     i2s1;
//AudioInputI2S2    i2s2;
AudioRecordQueue  queue1;
AudioRecordQueue  queue2;

AudioConnection   patchCord1(i2s1, 0, queue1, 0);
AudioConnection   patchCord2(i2s1, 1, queue2, 0);

void setup() {
  AudioMemory(60);
  queue1.begin();
  queue2.begin();
  Serial.begin(115200);
}

void loop() {
  if (queue1.available() && queue2.available()) {
    int16_t *data1 = queue1.readBuffer();
    int16_t *data2 = queue2.readBuffer();
    Serial.print("CH1[0]: "); Serial.print(data1[0]);
    Serial.print("\tCH2[0]: "); Serial.println(data2[0]);
    queue1.freeBuffer();
    queue2.freeBuffer();
  }
}