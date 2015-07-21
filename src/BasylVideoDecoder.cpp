#include "BasylVideoDecoder.h"
#include <fftw3.h>
#include <thread>
#include <iostream>
using namespace std;

BasylVideoDecoder::BasylVideoDecoder(int width, int height, int blockSize) : width(width), height(height), blockSize(blockSize), in1(nullptr), in2(nullptr), in3(nullptr), scalerA(4.88f), scalerB(24.88f)
{
	threadData = new float[width*height];
	threadData2 = new float[width*height];
	threadData3 = new float[width*height];
}


BasylVideoDecoder::BasylVideoDecoder(int width, int height, int blockSize, float scalerA, float scalerB): in1(nullptr), in2(nullptr), in3(nullptr), width(width), height(height), blockSize(blockSize), scalerA(scalerA), scalerB(scalerB)
{
	threadData = new float[width*height];
	threadData2 = new float[width*height];
	threadData3 = new float[width*height];
}

BasylVideoDecoder::~BasylVideoDecoder(void)
{
	delete[] threadData;
	delete[] threadData2;
	delete[] threadData3;


	
	fftwf_destroy_plan(g1);
	fftwf_destroy_plan(g2);
	fftwf_destroy_plan(g3);

	delete[] in1;
	delete[] in2;
	delete[] in3;

	
	delete[] out1;
	delete[] out2;
	delete[] out3;

}


void BasylVideoDecoder::decodeChannel(float* channel, fftwf_complex *in, fftwf_complex *out, fftwf_plan g)
{
	//fftwf_plan g;
	//fftwf_complex *in = new fftwf_complex[blockSize*blockSize], 
	//*out = new fftwf_complex[blockSize*blockSize];
	int offsetX = 0, offsetY = 0;
	int prs = 0, pos2 =0;
	
	int pos = 0;
	while(true) 
	{
		if(offsetY +blockSize >= height) break;

		bool skip = false;
		if(gChange[prs] & (1 << pos2) == 0)
		{
			skip = true;
		}

		pos2++;
		if(pos2 == 8)
		{
			prs++;
			pos2=0;

		}

		if(!skip) {
		float scaler = scalerA;
		pos = 0;
		for(int x = offsetX; x < offsetX+blockSize; x++)
		{
			for(int y = offsetY; y < offsetY + blockSize; y++)
			{
				if(pos == blockSize*1.8f -1)
				{
					scaler = scalerB;
				}
				in[pos][0] = (float)channel[x + y*width] * scaler;
				in[pos][1] = 0;
				pos++;
			}
		}

				
		fftwf_execute(g);
		pos = 0;
		for(int x = offsetX; x < offsetX+blockSize; x++)
		{
			for(int y = offsetY; y < offsetY + blockSize; y++)
			{
				
				//if(pos >= blockSize*1.8f -1) scaler *= 1.0685;
				channel[x + y*width] = (out[pos][0]/blockSize) + 127;
				pos++;
			}
		}
		
		}
		offsetX += blockSize;
		if(offsetX + blockSize >= width)
		{
			offsetX = 0;
			offsetY += blockSize;
		}

	}
	

}

void BasylVideoDecoder::decodeY()
{
	if(!in1)
	{
		BasylVideoEncoder::mut.lock();
		
		in1 = new fftwf_complex[blockSize*blockSize];
		out1 = new fftwf_complex[blockSize*blockSize];
		g1 = fftwf_plan_dft_2d(blockSize, blockSize, in1, out1, FFTW_BACKWARD, FFTW_MEASURE);
		
		BasylVideoEncoder::mut.unlock();
	}

	decodeChannel(threadData, in1, out1, g1);
}

void BasylVideoDecoder::decodeCb()
{

	if(!in2)
	{
		BasylVideoEncoder::mut.lock();
		
		in2 = new fftwf_complex[blockSize*blockSize];
		out2 = new fftwf_complex[blockSize*blockSize];
		g2 = fftwf_plan_dft_2d(blockSize, blockSize, in2, out2, FFTW_BACKWARD, FFTW_MEASURE);
		
		BasylVideoEncoder::mut.unlock();
	}

	decodeChannel(threadData2, in2, out2, g2);
}

void BasylVideoDecoder::decodeCr()
{

	if(!in3)
	{
		BasylVideoEncoder::mut.lock();
		
		in3 = new fftwf_complex[blockSize*blockSize];
		out3 = new fftwf_complex[blockSize*blockSize];
		g3 = fftwf_plan_dft_2d(blockSize, blockSize, in3, out3, FFTW_BACKWARD, FFTW_MEASURE);
		
		BasylVideoEncoder::mut.unlock();
	}

	decodeChannel(threadData3, in3, out3, g3);
}


/*
void BasylVideoDecoder::DecodeFrame(byte *y, byte *cb, byte *cr, byte* ret)
{


}
*/


void BasylVideoDecoder::DecodeFrame(byte *allBytes, byte* ret, byte* change)
{
	frameLock.lock();
	
	int inputPos = 0;
	int pos = 0, offsetX = 0,
	prs = 0, offsetY = 0;

	while(offsetY + blockSize < height)
	{
		for(int x = offsetX; x < offsetX+ blockSize; x++)
		{
			for(int y = offsetY; y < offsetY+ blockSize; y++)
			{
				if((change[prs] & (1 << pos)) != 0)
				{
					
					threadData[x + y*width] = allBytes[inputPos++] - 127.0f;
					threadData2[x + y*width] = allBytes[inputPos++] - 127.0f;
					threadData3[x + y*width] = allBytes[inputPos++] - 127.0f;	
				}
				else
				{
					//leaves the culled sections alone.
					//if debugging, I could set it to zero.
				}
			}
		}
	
		
		pos++;
		if(pos == 8)
		{
			prs++;
			pos = 0;
		}

		offsetX += blockSize;
		if(offsetX + blockSize >= width)
		{
			offsetX = 0;
			offsetY += blockSize;
		}
	}



	gChange = change;

	thread t1(& BasylVideoDecoder::decodeY, this);
	thread t2(& BasylVideoDecoder::decodeCb, this);
	thread t3(& BasylVideoDecoder::decodeCr, this);
	//waits for them to finish.
	t1.join();
	t2.join();
	t3.join();


	

	//now export time :D

	pos = prs = offsetX = offsetY = 0;
	//copies the stuff
	while(offsetY + blockSize < height)
	{
		for(int x = offsetX; x < offsetX+ blockSize; x++)
		{
			for(int y = offsetY; y < offsetY+ blockSize; y++)
			{
				if((change[prs] & (1 << pos)) != 0)
				{
					//converts to RGB
					int l = 298.082f * threadData[x + y*width] /256;
					int r = floor( l + 408.583f*threadData3[x + y*width]/256 - 222.921);
					int g = floor( l - 100.291f * threadData2[x + y*width]/256 - 208.120f*threadData3[x + y*width]/256 + 135.576);
					int b1 = floor( l + 516.412f * threadData2[x + y*width]/256 - 276.836f);

					BasylVideoEncoder::fix(r);
					BasylVideoEncoder::fix(g);
					BasylVideoEncoder::fix(b1);

					//booom.
					ret[x + y*width] = r;
					ret[x + y*width + width*height] = g;
					ret[x + y*width + width*height*2] = b1;

					if(y == offsetY + 1 && x == offsetX)
					{
						ret[offsetX + offsetY*width] = r;
						ret[offsetX + offsetY*width + width*height] = g;
						ret[offsetX + offsetY*width + width*height*2] = b1;
					}

				}
				else
				{
					//leaves the culled sections alone.
					//if debugging, I could set it to zero.
				}
			}
		}
	
		
		pos++;
		if(pos == 8)
		{
			prs++;
			pos = 0;
		}

		offsetX += blockSize;
		if(offsetX + blockSize >= width)
		{
			offsetX = 0;
			offsetY += blockSize;
		}
	}




	frameLock.unlock();





}