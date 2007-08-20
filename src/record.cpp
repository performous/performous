#include <record.h>
#include <screen.h>
#include <algorithm>
#include <functional>
#include <iostream>
#include <limits>
#include <vector>

#define MAX_FFT_LENGTH 48000
unsigned int rate = MAX_FFT_LENGTH;

class Peak {
	public:
	double freq;
	double db;
	Peak(double freq = 0.0, double db = -std::numeric_limits<double>::infinity()): freq(freq), db(db) {}
};

class Tone {
	protected:
	double m_freqSum; // Sum of the fundamental frequencies of all harmonics
	unsigned int m_harmonics;
	unsigned int m_hEven;
	unsigned int m_hHighest;
	double m_dbHighest;
	unsigned int m_dbHighestH;
	public:
	Tone(): m_freqSum(0.0), m_harmonics(0), m_hEven(0), m_hHighest(0), m_dbHighest(-std::numeric_limits<double>::infinity()), m_dbHighestH(0) {}
	void print() const {
		std::cout << freq() << " Hz, ";
		std::cout << m_harmonics << " harmonics ";
		std::cout << "(" << m_hEven << " are even), ";
		std::cout << "strongest is x" << m_dbHighestH << " @ " << m_dbHighest << " dB, ";
		std::cout << "highest is x" << m_hHighest << std::endl;
	}
	void combine(Peak& p, unsigned int h) {
		m_freqSum += p.freq / h;
		++m_harmonics;
		if (h % 2 == 0) ++m_hEven;
		if (h > m_hHighest) m_hHighest = h;
		if (p.db > m_dbHighest) {
			m_dbHighest = p.db;
			m_dbHighestH = h;
		}
	}
	double db() const { return m_dbHighest; }
	double freq() const { return m_freqSum / m_harmonics; }
	bool operator==(double f) const {
		return fabs(freq() / f - 1.0) < 0.02;
	}
	Tone& operator+=(Tone const& t) {
		m_freqSum += t.m_freqSum;
		m_harmonics += t.m_harmonics;
		m_hEven += t.m_hEven;
		if (t.m_hHighest > m_hHighest) m_hHighest = t.m_hHighest;
		if (t.m_dbHighest > m_dbHighest) {
			m_dbHighest = t.m_dbHighest;
			m_dbHighestH = t.m_dbHighestH;
		}
		return *this;
	}
};


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

static int match(Peak* peaks, int pos, double freq) {
	int best = pos;
	if (fabs(peaks[pos-1].freq - freq) < fabs(peaks[best].freq - freq)) best = pos-1;
	if (fabs(peaks[pos+1].freq - freq) < fabs(peaks[best].freq - freq)) best = pos+1;
	return best;
}

void CFft::measure(int nframes, int overlap, float *indata)
{
	int stepSize = fftSize/overlap;
	double freqPerBin = rate/(double)fftSize;
	double phaseStep = 2.*M_PI*(double)stepSize/(double)fftSize;

	if (!fftSample) fftSample = fftSampleBuffer + (fftSize-stepSize);

	for (int i=0; i<nframes; i++) {
		*fftSample++ = indata[i];
		if (fftSample-fftSampleBuffer < fftSize) continue;

		fftSample = fftSampleBuffer + (fftSize-stepSize);

		for (int k=0; k<fftSize; k++) {
			fftIn[k] = fftSampleBuffer[k] * window[k];
		}
		fftwf_execute(fftPlan);

		Peak peaks[fftSize/2 + 1];

		for (int k=0; k<=fftSize/2; k++) {
			double real = fftOut[k][0];
			double imag = fftOut[k][1];
			double magnitude = 20.*log10(2.*sqrt(real*real + imag*imag)/fftSize);
			double phase = atan2(imag, real);

			// compute phase difference
			double delta = phase - fftLastPhase[k];
			fftLastPhase[k] = phase;
			// subtract expected phase difference
			delta -= k * phaseStep;
			// map delta phase into +/- M_PI interval
			delta = remainder(delta, 2.0 * M_PI);
			// calculate diff from bin center frequency
			delta *= overlap / (2.0 * M_PI);
			// compute the k-th partials' true frequency
			double freq = (k + delta) * freqPerBin;

			peaks[k].freq = freq;
			peaks[k].db = magnitude;
		}
		// Filter out the most significant peaks only
		// TODO: proper handling of tones with "missing fundamental"
		std::vector<Tone> tones;
		for (int k = 1; k < fftSize / 2; ++k) {
			// Prefilter out too silent bins and bins that are weaker than their neighbors
			if (peaks[k].db < -50.0 || peaks[k].db < peaks[k-1].db || peaks[k].db < peaks[k+1].db) continue;
			// Find the base peak (fundamental frequency)
			int harmonic = 1;
			for (int h = 2; k / h > 2; ++h) {
				double freq = peaks[k].freq / h;
				int best = match(peaks, k / h, freq);
				if (peaks[best].db > -60.0 && fabs(peaks[best].freq / freq - 1.0) < .02) harmonic = h;
			}
			std::vector<Tone>::iterator it = std::find(tones.begin(), tones.end(), peaks[k].freq / harmonic);
			if (it == tones.end()) {
				tones.push_back(Tone());
				it = tones.end() - 1;
			}
			it->combine(peaks[k], harmonic);
		}
		/* DEBUG
		if (!tones.empty()) {
			std::cout << tones.size() << " tones\n";
			std::for_each(tones.begin(), tones.end(), std::mem_fun_ref(&Tone::print));
		}
		*/
		// Find the first tone (i.e. lowest frequency) over 80 Hz.
		// TODO: can we do better?
		m_freq = 0.0;
		for (size_t i = 0; i < tones.size(); ++i) {
			if (tones[i].freq() > 80.0) {
				m_freq = tones[i].freq();
			}
		}
		fftFrameCount++;
		// TODO: do not move stuff around, use proper standard library containers instead
		memmove(fftSampleBuffer, fftSampleBuffer+stepSize, (fftSize-stepSize)*sizeof(float));
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

#ifdef USE_GSTREAMER_RECORD
static void _handoff_cb(GstElement *fakesink, GstBuffer *buffer, GstPad *pad, gpointer user_data)
{
	CScreenManager::getSingletonPtr()->getRecord()->getFft()->compute(GST_BUFFER_SIZE(buffer)/2,(signed short int *)GST_BUFFER_DATA(buffer));
	return;
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
#ifdef USE_GSTREAMER_RECORD
	GstElement *source, *audioconvert, *audioresample, *sink;
	GstCaps *caps;

	gst_init (NULL, NULL);
	pipeline = gst_pipeline_new("record-pipeline");

	if (!(source = gst_element_factory_make("alsasrc", "record-source"))) {
		if (!(source = gst_element_factory_make("osssrc", "record-source"))) {
			if (!(source = gst_element_factory_make("osxaudiosrc", "record-source"))) {
				fprintf(stderr, "[Error] Failed to create GStreamer element: 'record-source'\n");
				exit(EXIT_FAILURE);
			}
		}
	}
	if (!(audioconvert = gst_element_factory_make("audioconvert", NULL))) {
		fprintf(stderr, "[Error] Failed to create GStreamer element: 'audioconvert'\n");
		exit(EXIT_FAILURE);
	}
	if (!(audioresample = gst_element_factory_make("audioresample", NULL))) {
		fprintf(stderr, "[Error] Failed to create GStreamer element: 'audioresample'\n");
		exit(EXIT_FAILURE);
	}
	if (!(sink = gst_element_factory_make("fakesink", "record-sink"))) {
		fprintf(stderr, "[Error] Failed to create GStreamer element: 'fakesink'\n");
		exit(EXIT_FAILURE);
	}
	
	gst_bin_add_many(GST_BIN(pipeline), source, audioconvert, audioresample, sink, NULL);
	g_object_set(G_OBJECT(sink), "sync", TRUE, NULL);
	g_object_set(G_OBJECT(sink), "signal-handoffs", TRUE, NULL);
	g_signal_connect(G_OBJECT(sink), "handoff", G_CALLBACK(_handoff_cb), NULL);
	
	/* Link the elements together */
	caps = gst_caps_new_simple("audio/x-raw-int",
		"rate", G_TYPE_INT, MAX_FFT_LENGTH,
		"width", G_TYPE_INT, 16,
		"depth", G_TYPE_INT, 16,
		"channels", G_TYPE_INT, 1, NULL);
	
	if (!gst_element_link_many(source, audioconvert, audioresample, NULL)) {
		fprintf(stderr, "[Error] Failed to link the GStreamer elements together: 'alsasrc' -> 'audioconvert' -> 'audioresample'\n");
		exit(EXIT_FAILURE);
	}
	if (!gst_element_link_filtered(audioresample, sink, caps)) {
		fprintf(stderr, "[Error] Failed to link the GStreamer elements: 'audioresample' -> 'fakesink'\n");
		exit(EXIT_FAILURE);
	}
	gst_caps_unref(caps);
	
	/* TODO: gst_element_set_state(_pipeline, GST_STATE_PAUSED); */
	gst_element_set_state(pipeline, GST_STATE_PLAYING);
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
#ifdef USE_GSTREAMER_RECORD
	if( pipeline == NULL )
		return;

	gst_element_set_state(pipeline, GST_STATE_NULL);
	gst_object_unref(GST_OBJECT(pipeline));
	pipeline = NULL;
#endif
}

const float baseFreq = 440.0;
const int baseId = 33;

const char * CRecord::getNoteStr(int id)
{
	if (id == -1) return "";
	static const char * note[12] = {"C ","C#","D ","D#","E ","F ","F#","G ","G#","A ","A#","B "};
	static char buf[4] = { 0 };
	memcpy(buf, note[id%12], 2);
	buf[2] = '2' + (id / 12);
	return buf;
}

float CRecord::getNoteFreq(int id)
{
	if (id == -1) return 0.0;
	return baseFreq * pow(2.0, (id - baseId) / 12.0);
}

int CRecord::getNoteId()
{
	float freq = fft->getFreq();
	if (freq < 1.0) return -1;
	int id = baseId + (int) (12.0 * log(freq / baseFreq) / log(2) + 0.5);
	return id < 0 ? -1 : id;
}

