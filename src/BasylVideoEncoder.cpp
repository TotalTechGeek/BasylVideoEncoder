#include "BasylVideoEncoder.h"
#include <thread>
#include <iostream>
using namespace std;

mutex BasylVideoEncoder::mut;

BasylVideoEncoder::BasylVideoEncoder(int width, int height, int blockSize) : width(width), height(height), blockSize(blockSize), threadData(nullptr), threadData2(nullptr), threadData3(nullptr), lastR(nullptr), lastG(nullptr), lastB(nullptr), in1(nullptr), in2(nullptr), in3(nullptr), scalerA(4.88f), scalerB(24.88f)
{
	
	threadData = new float[width * height];
	threadData2 = new float[width * height];
	threadData3 = new float[width * height];

	lastR = new byte[width*height];
	lastG = new byte[width*height];
	lastB = new byte[width*height];

}


BasylVideoEncoder::BasylVideoEncoder(int width, int height, int blockSize, float scalerA, float scalerB):  width(width), height(height), blockSize(blockSize), threadData(nullptr), threadData2(nullptr), threadData3(nullptr), lastR(nullptr), lastG(nullptr), lastB(nullptr), in1(nullptr), in2(nullptr), in3(nullptr), scalerA(scalerA), scalerB(scalerB)
{
	threadData = new float[width*height];
	threadData2 = new float[width*height];
	threadData3 = new float[width*height];
	
	lastR = new byte[width*height];
	lastG = new byte[width*height];
	lastB = new byte[width*height];
}



BasylVideoEncoder::~BasylVideoEncoder(void)
{
	delete[] threadData;
	delete[] threadData2;
	delete[] threadData3;

	delete[] lastB;
	delete[] lastR;
	delete[] lastG;


	fftwf_destroy_plan(g1);
	fftwf_destroy_plan(g2);
	fftwf_destroy_plan(g3);

	delete[] in1;
	delete[] in2;
	delete[] in3;

	
	delete[] out1;
	delete[] out2;
	delete[] out3;

	/*
	fftwf_free(in1);
	fftwf_free(in2);
	fftwf_free(in3);
	
	fftwf_free(out1);
	fftwf_free(out2);
	fftwf_free(out3);
	*/

	
}

void BasylVideoEncoder::fix(int& b)
{
	if(b < 0) b = 0;
	if(b > 255) b = 255;
	
}
int BasylVideoEncoder::fix2(int b)
{
	if(b < -128)
	{
		return -128;
	}

	if(b > 127)
		return 127;
	return b;
}


int BasylVideoEncoder::getOutputSize()
{
	return width*height*3;
}

int BasylVideoEncoder::getFrameChangeSize()
{
	return width*height/8/(blockSize*blockSize);
}


bool BasylVideoEncoder::inRange(int a, int b)
{


	int c = abs(a - b);
	return c < 6;
}

void BasylVideoEncoder::encodeChannel(float* channel, fftwf_complex *in, fftwf_complex *out, fftwf_plan g)
{

	//fftwf_plan g, d;
	//fftwf_complex *in = new fftwf_complex[blockSize*blockSize], *out = new fftwf_complex[blockSize*blockSize];
	//fftwf_complex *inv_in=  new fftwf_complex[blockSize*blockSize], *inv_out = new fftwf_complex[blockSize*blockSize];


	int prs = 0, pos2= 0;
	int offsetX = 0, offsetY = 0;
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

		pos = 0;
		for(int x = offsetX; x < offsetX+blockSize; x++)
		{
			for(int y = offsetY; y < offsetY + blockSize; y++)
			{
				in[pos][0] = (float)channel[x + y*width];
				in[pos++][1] = 0;
			
			}
		}
		fftwf_execute(g);
		
		pos = 0;
		float scaler = scalerA;
		for(int x = offsetX; x < offsetX+blockSize; x++)
		{
			for(int y = offsetY; y < offsetY + blockSize; y++)
			{
				if(pos == blockSize*1.8f -1)
				{
					scaler = scalerB;
				}
				
				channel[x + y*width] = /*inv_in[pos][0] = */ (float)fix2(out[pos++][0]/blockSize/scaler);
			
			}
		}

		/*
		fftwf_execute(d);
		pos = 0;
		for(int x = offsetX; x < offsetX+blockSize; x++)
		{
			for(int y = offsetY; y < offsetY + blockSize; y++)
			{
				channel[x+y*width] = (inv_out[pos][0]/blockSize * scaler) + 127;
					
				pos++;
			}
		}
		*/
		}
		offsetX += blockSize;
		if(offsetX + blockSize >= width)
		{
			offsetX = 0;
			offsetY += blockSize;
		}

	}
	
}

void BasylVideoEncoder::encodeY()
{
	if(!in1)
	{
	//	cout << "Hello?";
		mut.lock();
		
		in1 = new fftwf_complex[blockSize*blockSize];
		out1 = new fftwf_complex[blockSize*blockSize];
		g1 = fftwf_plan_dft_2d(blockSize, blockSize, in1, out1, FFTW_FORWARD, FFTW_MEASURE);
		
		mut.unlock();
	}


	encodeChannel(threadData, in1,out1, g1);
}
void BasylVideoEncoder::encodeCb()
{
	if(!in2)
	{

		mut.lock();
		
		in2 = new fftwf_complex[blockSize*blockSize];
		out2 = new fftwf_complex[blockSize*blockSize];
		g2 = fftwf_plan_dft_2d(blockSize, blockSize, in2, out2, FFTW_FORWARD, FFTW_MEASURE);
		
		mut.unlock();
	}

	encodeChannel(threadData2, in2, out2, g2);
}
void BasylVideoEncoder::encodeCr()
{
	if(!in3)
	{
		mut.lock();
		
		in3 = new fftwf_complex[blockSize*blockSize];
		out3 = new fftwf_complex[blockSize*blockSize];
		g3 = fftwf_plan_dft_2d(blockSize, blockSize, in3, out3, FFTW_FORWARD, FFTW_MEASURE);
		
		mut.unlock();
	}

	encodeChannel(threadData3, in3, out3, g3);
}



int BasylVideoEncoder::EncodeFrame(byte *r, byte *g, byte *b, byte* ret, byte* change)
{
	frameLock.lock();
	int result = 0;
	//No culling yet. Encodes every sub frame as of right now. 
	for(int x = 0; x < width; x++)
	{
			for(int y =0; y < height; y++)
			{

				int rn = r[x + y*width];
				int gn = g[x  + y*width];
				int bn = b[x  + y*width];

				
				threadData[x + y*width] = rn * 65.738f / 256 + 16 + gn * 129.057f / 256 + bn * 25.064f / 256;
				threadData2[x + y*width] = 128 - rn * 37.945f / 256 - gn * 74.494f / 256 + bn * 112.439f / 256;
				threadData3[x + y*width] = 128 + rn * 112.439f / 256 -gn * 94.154f / 256 - bn * 18.285f / 256;
			
				threadData[x + y*width] -= 127;
				threadData2[x + y*width] -= 127;
				threadData3[x + y*width] -= 127;
			}

	}


	int offsetX= 0, offsetY=0, pos =0, prs =0, bc = 0;
	change[prs] = 0;

	//registers change
	while(offsetY + blockSize < height)
	{
		int count = 0;
		for(int x = 0; x < blockSize; x++)
		{
			for(int y = 0; y < blockSize; y++)
			{

				if(!inRange(r[x+offsetX + (y+offsetY)*width], lastR[x+offsetX + (y+offsetY)*width])) count++;
				if(!inRange(g[x+offsetX + (y+offsetY)*width], lastG[x+offsetX + (y+offsetY)*width])) count++;
				if(!inRange(b[x+offsetX + (y+offsetY)*width], lastB[x+offsetX + (y+offsetY)*width])) count++;
				
			
			}
		}
	
		
		offsetX += blockSize;
		if(offsetX + blockSize >= width)
		{
			offsetX = 0;
			offsetY += blockSize;
		}

		if(count > blockSize*1.5f + 1) 
		{
			//change[prs] |= (1 << pos);
			change[prs] = 255;
		}
		else
		{
		}

		pos++;
		if(pos == 8)
		{
			bc++;
			prs++;
			change[prs] = 0;
			pos = 0;
		}
	}


	gChange = change;
			

	
	thread t1(& BasylVideoEncoder::encodeY, this);
	thread t2(& BasylVideoEncoder::encodeCr, this);
	thread t3(& BasylVideoEncoder::encodeCb, this);

	t1.join();
	t2.join();
	t3.join();

	pos = 0; offsetX = 0;
	prs = 0; offsetY = 0;
	int posInOutput = 0;
	//copies the stuff
	while(offsetY + blockSize < height)
	{
		for(int x = offsetX; x < offsetX+ blockSize; x++)
		{
			for(int y = offsetY; y < offsetY+ blockSize; y++)
			{
				lastR[x + y*width] = r[x  + y*width];
				lastG[x + y*width] = g[x  + y*width];
				lastB[x + y*width] = b[x  + y*width];


				if((change[prs] & (1 << pos)) != 0)
				{
					//cout << (change[prs] & (1<<pos)) << endl;
					ret[posInOutput++] = threadData[x + y*width] + 127;
					ret[posInOutput++] = threadData2[x + y*width] + 127;
					ret[posInOutput++] = threadData3[x + y*width] + 127;
				}
				else
				{
					//ret[x + y*width] = 0;
					//ret[x + y*width + width*height] = 0;
					//ret[x + y*width + 2*width*height] = 0;
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

	result = posInOutput;

	/*
	for(int x = 0; x < width; x++)
	{
		for(int y =0; y < height; y++)
		{

			lastR[x + y*width] = r[x  + y*width];
			lastG[x + y*width] = g[x  + y*width];
			lastB[x + y*width] = b[x  + y*width];


			ret[x + y*width] = threadData[x + y*width] + 127;
			ret[x + y*width + width*height] = threadData2[x + y*width] + 127;
			ret[x + y*width + 2*width*height] = threadData3[x + y*width] + 127;


			/*
			int l = 298.082f * threadData[x + y*width] /256;
			int r = floor( l + 408.583f*threadData3[x + y*width]/256 - 222.921);
			int g = floor( l - 100.291f * threadData2[x + y*width]/256 - 208.120f*threadData3[x + y*width]/256 + 135.576);
			int b1 = floor( l + 516.412f * threadData2[x + y*width]/256 - 276.836f);

			fix(r);
			fix(g);
			fix(b1);

			
			ret[x + y*width] = r;
			ret[x + y*width + width*height] = g;
			ret[x + y*width + width*height*2] = b1;
			//

			pos++;
		}
	}*/
	


	frameLock.unlock();
	return result;
}

