
#ifndef MLS_TEENSY_H
#define MLS_TEENSY_H

#include <Arduino.h>

class MLS {
public:
    MLS(int nbits);
    void setBits(int n_bits);
    int size();
    void getSeq(bool* seq_out);

private:
    static const int MAX_NBITS = 32;
    int nbits;
    const int* taps;
    int numTaps;

    bool goodSeq(const bool* seq);
    void initTaps();
};

#endif // MLS_TEENSY_H
