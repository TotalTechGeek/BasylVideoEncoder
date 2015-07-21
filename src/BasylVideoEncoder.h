
typedef unsigned __int8 byte;
#include <mutex>
#include <fftw3.h>
#pragma once
class BasylVideoEncoder
{
friend class BasylVideoDecoder;
private:
	int width, height;
	int blockSize;

	float scalerA, scalerB;

	byte* lastR;
	byte* lastG;
	byte* lastB;


	//once :D
	fftwf_plan g1, g2, g3;
	fftwf_complex *in1, *in2, *in3;
	fftwf_complex *out1, *out2, *out3;


	//global but meh
	byte* gChange;

	static std::mutex mut;
	std::mutex frameLock;

	float* threadData;
	float* threadData2;
	float* threadData3;

	static void fix(int& b);
	static int fix2(int b);
	static bool inRange(int a, int b);

	void encodeChannel(float* channel, fftwf_complex *in, fftwf_complex *out, fftwf_plan g);
	void encodeY();
	void encodeCb();
	void encodeCr();
public:
	BasylVideoEncoder(int width, int height, int blockSize);
	
	BasylVideoEncoder(int width, int height, int blockSize,float scalerA, float scalerB);
	~BasylVideoEncoder(void);

	int getOutputSize();
	int getFrameChangeSize();

	int EncodeFrame(byte *r, byte *g, byte *b, byte* ret, byte* change);
};
