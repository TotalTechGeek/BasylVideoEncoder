#pragma once
#include "BasylVideoEncoder.h";
class BasylVideoDecoder
{
private:
friend class BasylVideoEncoder; //shares the mutex.
int width, height, blockSize;

std::mutex frameLock;



//once :D
fftwf_plan g1, g2, g3;
fftwf_complex *in1, *in2, *in3;
fftwf_complex *out1, *out2, *out3;

//global but meh
byte* gChange;

float scalerA, scalerB;

float* threadData;
float* threadData2;
float* threadData3;

void decodeChannel(float* channel, fftwf_complex *in, fftwf_complex *out, fftwf_plan g);

void decodeY();
void decodeCb();
void decodeCr();

public:
	BasylVideoDecoder(int width, int height, int blockSize);
	BasylVideoDecoder(int width, int height, int blockSize, float scalerA, float scalerB);
	~BasylVideoDecoder(void);

//void DecodeFrame(byte *y, byte *cb, byte *cr, byte* ret);
void DecodeFrame(byte *allBytes, byte* ret, byte* change);
};

