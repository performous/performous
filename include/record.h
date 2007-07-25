#ifndef __RECORD_H_
#define __RECORD_H_

#include "../config.h"

class CFft {
	public:
	CFft(int size=10);
	~CFft();
	void compute(int nframes, signed short int *indata);
	float getFreq( void ) { return m_freq; }
	private:
	void measure (int nframes, int overlap, float *indata);
	float *fftSampleBuffer;
	float *fftSample;
	float *fftLastPhase;
	int fftSize;
	int fftFrameCount;
	float *fftIn;
	fftwf_complex *fftOut;
	fftwf_plan fftPlan;
	float m_freq;
	double *window;
};

class CRecord {
	public:
	CRecord(char * captureDevice = "default");
	~CRecord();
	void compute();
	float getFreq( void ) { return fft->getFreq(); }
	const char * getNoteStr( int id);
	int getNoteId(void);
	float getNoteFreq(int id);
	private:
	CFft * fft;
	snd_pcm_t *alsaHandle;
	signed short int buf[4096];
};

#endif
