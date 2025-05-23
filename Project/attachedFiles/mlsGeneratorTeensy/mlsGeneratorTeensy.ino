int LFSRBits = 16;                // Change this between 2 and 32
const unsigned long delayUS = 1;  // Delay in microseconds between bits

uint32_t LFSR;
uint32_t mask;

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


void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;
}

void loop() {
  generateMLS();
  /*
  if (LFSRBits < 32) {
    LFSRBits++;
  } else if (LFSRBits >= 32) {
    LFSRBits = 2;
    return;
  }
  */
  delay(1000);
}

void generateMLS() {
  if (LFSRBits < 2 || LFSRBits > 32) {
    Serial.println("LFSRBits must be between 2 and 32.");
    while (1);
  }

  mask = (1UL << LFSRBits) - 1;
  LFSR = mask;  // Start with all ones
  uint32_t taps = feedbackTaps(LFSRBits);  // Should be correct for left shift!

  Serial.print("Generating MLS with ");
  Serial.print(LFSRBits);
  Serial.println(" bits:");
  Serial.print("The MLS should be ");
  Serial.print((1UL << LFSRBits) - 1);
  Serial.println(" bits long.");

  uint32_t startMLSTime = micros();
  uint32_t MLSLength = (1UL << LFSRBits) - 1;

  for (uint32_t i = 0; i < MLSLength; i++) {
    uint8_t outputBit = (LFSR >> (LFSRBits - 1)) & 1;  // Output MSB
    Serial.print(outputBit);

    bool feedback = __builtin_parity(LFSR & taps);  // Parity of taps
    LFSR <<= 1;  // Shift left

    if (feedback) {
      LFSR |= 1;  // Insert feedback at LSB
    }
    LFSR &= mask;  // Ensure we stay inside LFSRBits
  }

  uint32_t endMLSTime = micros();
  Serial.println("\nMLS generation complete.");
  Serial.print("It has taken ");
  Serial.print(endMLSTime - startMLSTime);
  Serial.println(" microseconds to calculate and print.");
}













