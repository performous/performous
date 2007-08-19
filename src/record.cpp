#include <record.h>
#include <screen.h>

#define MAX_FFT_LENGTH 48000
#define MAX_PEAKS 4
unsigned int rate = MAX_FFT_LENGTH;
typedef struct {
	double freq;
	double db;
} Peak;


CFft::CFft(int size)
{
	fftSize = rate/size;
	fftIn = (float*)fftwf_malloc(sizeof(float) * 2 * (fftSize/2+1));
	fftOut = (fftwf_complex *)fftIn;
	fftPlan = fftwf_plan_dft_r2c_1d(fftSize, fftIn, fftOut, FFTW_MEASURE);
	fftSampleBuffer = new float[fftSize];
	fftSample = NULL;
	fftLastPhase = new float[fftSize/2+1];
	memset(fftSampleBuffer, 0, fftSize*sizeof(float));
	memset(fftLastPhase, 0, (fftSize/2+1)*sizeof(float));
	fftFrameCount = 0;
	window = new double[fftSize];
	for (int i=0; i<fftSize; i++)
	  window[i] = -.5*cos(2.*M_PI*(double)i/(double)fftSize)+.5;

}
CFft::~CFft()
{
	fftwf_destroy_plan(fftPlan);
	fftwf_free(fftIn);
	delete[] fftSampleBuffer;
	delete[] fftLastPhase;
	delete[] window;
}

void CFft::measure(int nframes, int overlap, float *indata)
{
	int stepSize = fftSize/overlap;
	double freqPerBin = rate/(double)fftSize, phaseDifference = 2.*M_PI*(double)stepSize/(double)fftSize;

	if (!fftSample) fftSample = fftSampleBuffer + (fftSize-stepSize);

	for (int i=0; i<nframes; i++) {
		*fftSample++ = indata[i];
		if (fftSample-fftSampleBuffer >= fftSize) {
			int k;
			Peak peaks[MAX_PEAKS];

			for (k=0; k<MAX_PEAKS; k++) {
				peaks[k].db = -200.;
				peaks[k].freq = 0.;
			}

			fftSample = fftSampleBuffer + (fftSize-stepSize);

			for (k=0; k<fftSize; k++) {
				fftIn[k] = fftSampleBuffer[k] * window[k];
			}
			fftwf_execute(fftPlan);

			for (k=0; k<=fftSize/2; k++) {
				long qpd;
				float real = fftOut[k][0];
				float imag = fftOut[k][1];
				float magnitude = 20.*log10(2.*sqrt(real*real + imag*imag)/fftSize);
				float phase = atan2(imag, real);
				float tmp, freq;

				/* compute phase difference */
				tmp = phase - fftLastPhase[k];
				fftLastPhase[k] = phase;

				/* subtract expected phase difference */
				tmp -= (double)k*phaseDifference;

				/* map delta phase into +/- Pi interval */
				qpd = (long) (tmp / M_PI);
				if (qpd >= 0) qpd += qpd&1;
				else qpd -= qpd&1;
				tmp -= M_PI*(double)qpd;

				/* get deviation from bin frequency from the +/- Pi interval */
				tmp = overlap*tmp/(2.*M_PI);

				/* compute the k-th partials' true frequency */
				freq = (double)k*freqPerBin + tmp*freqPerBin;

				if (freq > 0.0 && magnitude > peaks[0].db) {
					memmove(peaks+1, peaks, sizeof(Peak)*(MAX_PEAKS-1));
					peaks[0].freq = freq;
					peaks[0].db = magnitude;
				}
			}
			fftFrameCount++;
			if (fftFrameCount > 0 && fftFrameCount % overlap == 0) {
				int l, maxharm = 0;
				k = 0;
				for (l=1; l<MAX_PEAKS && peaks[l].freq > 0.0; l++) {
					int harmonic;

					for (harmonic=5; harmonic>1; harmonic--) {
						if (peaks[0].freq / peaks[l].freq < harmonic+.02 && peaks[0].freq / peaks[l].freq > harmonic-.02) {
							if (harmonic > maxharm && peaks[0].db < peaks[l].db/2) {
								maxharm = harmonic;
								k = l;
							}
						}
					}
				}
				if( peaks[k].freq > 100. && peaks[k].freq < 2000. && peaks[k].db > -45. )
					m_freq = peaks[k].freq;
				else
					m_freq = 0.0;
			}
			memmove(fftSampleBuffer, fftSampleBuffer+stepSize, (fftSize-stepSize)*sizeof(float));
		}
	}
}

void CFft::compute(int nframes,signed short int *indata)
{
	float buf[nframes];
	for (int i = 0 ; i<nframes ; i++) {
		buf[i] = indata[i]/32768.;
	}
	if( nframes > 0 )
		measure(nframes, 4, buf);
}

CRecord::CRecord( char * deviceName )
{
	fft = new CFft(10);
	captureDevice = deviceName;
}

CRecord::~CRecord()
{
	delete fft;
}

#ifdef USE_ALSA_RECORD
int thread_func(void * _alsaHandle)
{
	signed short int buf[4096];
	int frames  = 1;
	int nFrames;
	snd_pcm_t *alsaHandle = (snd_pcm_t *)_alsaHandle;

	while( !CScreenManager::getSingletonPtr()->isFinished() ) {
		nFrames = 0;
		nFrames = snd_pcm_readi(alsaHandle, buf, frames);
	
		if(nFrames == -EPIPE)
			snd_pcm_prepare(alsaHandle);
		else if(nFrames < 0)
			fprintf(stderr,"error from read: %s\n",snd_strerror(nFrames));
		else if(nFrames != frames)
			fprintf(stderr, "short read, read %d frames\n", nFrames);
		CScreenManager::getSingletonPtr()->getRecord()->getFft()->compute(nFrames, buf);
	}
	return 1;
}
#endif

#ifdef USE_PORTAUDIO19_RECORD
static int recordCallback( const void *inputBuffer, void *outputBuffer, unsigned long framesPerBuffer,
			const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags, void *userData )
{
	CScreenManager::getSingletonPtr()->getRecord()->getFft()->compute(framesPerBuffer,(signed short int *)inputBuffer);
	return paContinue;
}
#endif
#ifdef USE_PORTAUDIO18_RECORD
static int recordCallback( void *inputBuffer, void *outputBuffer, unsigned long framesPerBuffer,
			PaTimestamp outTime, void *userData )
{
	CScreenManager::getSingletonPtr()->getRecord()->getFft()->compute(framesPerBuffer,(signed short int *)inputBuffer);
	return 0;
}
#endif

void CRecord::startThread()
{
#ifdef USE_ALSA_RECORD
	int result;
	snd_pcm_hw_params_t *hw_params;

	if ((result = snd_pcm_open(&alsaHandle, captureDevice, SND_PCM_STREAM_CAPTURE, 0)) < 0) {
		fprintf(stderr, "Cannot open audio device %s: %s\n",captureDevice, snd_strerror(result));
		exit(EXIT_FAILURE);
	}

	snd_pcm_hw_params_malloc(&hw_params);
	snd_pcm_hw_params_any(alsaHandle, hw_params);
	snd_pcm_hw_params_set_access(alsaHandle, hw_params,SND_PCM_ACCESS_RW_INTERLEAVED);
	snd_pcm_hw_params_set_format(alsaHandle, hw_params,SND_PCM_FORMAT_S16_LE);
	snd_pcm_hw_params_set_rate_near(alsaHandle, hw_params,&rate, 0);
	if( rate != MAX_FFT_LENGTH )
		fprintf(stderr, "rate as changed, please report this as a bug\n");
	snd_pcm_hw_params_set_channels(alsaHandle, hw_params, 1);
	snd_pcm_hw_params(alsaHandle, hw_params);
	snd_pcm_hw_params_free(hw_params);
	snd_pcm_prepare(alsaHandle);

	thread = SDL_CreateThread(thread_func, (void *)alsaHandle);
#endif
#ifdef USE_PORTAUDIO_RECORD
	PaError err = paNoError;

	err = Pa_Initialize();
	if( err != paNoError ) {
		fprintf(stderr, "Cannot open audio device %s\n",captureDevice);
		exit(EXIT_FAILURE);
	}

	#ifdef USE_PORTAUDIO19_RECORD
	PaStreamParameters inputParameters;

	inputParameters.device = Pa_GetDefaultInputDevice();
	inputParameters.channelCount = 1;
	inputParameters.sampleFormat = paInt16;
	inputParameters.suggestedLatency = Pa_GetDeviceInfo( inputParameters.device )->defaultLowInputLatency;
	inputParameters.hostApiSpecificStreamInfo = NULL;

	err = Pa_OpenStream( &stream, &inputParameters, NULL, MAX_FFT_LENGTH, 50, paClipOff, recordCallback, NULL );
	#endif
	#ifdef USE_PORTAUDIO18_RECORD
	err = Pa_OpenStream( &stream, Pa_GetDefaultInputDeviceID(), 1, paInt16, NULL, paNoDevice, 0, paInt16, NULL, MAX_FFT_LENGTH, 50, 0, 0, recordCallback, NULL);
	#endif
	if( err != paNoError ) {
		fprintf(stderr, "Cannot open audio device %s\n",captureDevice);
		exit(EXIT_FAILURE);
	}

	err = Pa_StartStream( stream );
	if( err != paNoError ) {
		fprintf(stderr, "Cannot open audio device %s\n",captureDevice);
		exit(EXIT_FAILURE);
	}
#endif
}

void CRecord::stopThread()
{
#ifdef USE_ALSA_RECORD
	SDL_WaitThread(thread, NULL);
	snd_pcm_drain(alsaHandle);
	snd_pcm_close(alsaHandle);
#endif
#ifdef USE_PORTAUDIO_RECORD
	Pa_CloseStream( stream );
	Pa_Terminate();
#endif
}

#define QUARTER_TONE 1.029302236643
#define HALF_TONE    1.059463094359

float game[12] = {
	65.406,
	69.295,
	73.416,
	77.781,
	82.406,
	87.307,
	92.498,
	97.998,
	103.826,
	110.000,
	116.540,123.470
};
float gameSep[12] = {
	65.406*QUARTER_TONE,
	69.295*QUARTER_TONE,
	73.416*QUARTER_TONE,
	77.781*QUARTER_TONE,
	82.406*QUARTER_TONE,
	87.307*QUARTER_TONE,
	92.498*QUARTER_TONE,
	97.998*QUARTER_TONE,
	103.826*QUARTER_TONE,
	110.000*QUARTER_TONE,
	116.540*QUARTER_TONE,
	123.470*QUARTER_TONE
};

const char * note[12] = {"C ","C#","D ","D#","E ","F ","F#","G ","G#","A ","A#","B "};

const char * CRecord::getNoteStr( int id )
{
	if( id == -1 )
		return "  ";
	else
		return note[id%12];
}

float CRecord::getNoteFreq( int id )
{
	float result = game[0];

	if( id == -1 )
		return 0.0;
	
	for(int i = 0 ; i < id ; i++)
		result *= HALF_TONE;
	return result;

}
int CRecord::getNoteId( void )
{
	float freq = fft->getFreq();
	if(freq == 0.0)
		return -1;
	int gamme = 0;
	while( gamme < 8 ) {
		for( int i = 0 ; i < 12 ; i++ )
			if(freq < gameSep[i])
				return (i+12*gamme);
		freq/=2.0;
		gamme++;
	}
	return -1;
}
