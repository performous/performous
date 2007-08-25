#include <record.h>
#include <screen.h>
#include <algorithm>
#include <functional>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>

Tone::Tone():
  m_freqSum(0.0),
  m_harmonics(0),
  m_hEven(0),
  m_hHighest(0),
  m_dbHighest(-std::numeric_limits<double>::infinity()),
  m_dbHighestH(0)
{}

void Tone::print() const {
	std::cout << freq() << " Hz, " << m_harmonics << " harmonics (" << m_hEven << " are even), strongest is x" << m_dbHighestH << " @ " << m_dbHighest << " dB, highest is x" << m_hHighest << std::endl;
}

void Tone::combine(Peak& p, unsigned int h) {
	m_freqSum += p.freq / h;
	++m_harmonics;
	if (h % 2 == 0) ++m_hEven;
	if (h > m_hHighest) m_hHighest = h;
	if (p.db > m_dbHighest) {
		m_dbHighest = p.db;
		m_dbHighestH = h;
	}
}

bool Tone::isWeak() const {
	if (m_harmonics > 2) return db() < -45.0;
	if (m_harmonics > 1) return db() < -35.0;
	return db() < -25.0;
}

bool Tone::operator==(double f) const {
	return fabs(freq() / f - 1.0) < 0.03;
}

Tone& Tone::operator+=(Tone const& t) {
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

CFft::CFft(size_t fftSize, size_t fftStep):
  fftSize(fftSize),
  fftStep(fftStep),
  fftIn((float*)fftwf_malloc(sizeof(float) * 2 * (fftSize/2+1))),
  fftOut((fftwf_complex *)fftIn),
  fftPlan(fftwf_plan_dft_r2c_1d(fftSize, fftIn, fftOut, FFTW_MEASURE)),
  fftLastPhase(fftSize/2+1),
  window(fftSize),
  m_peak(-std::numeric_limits<double>::infinity()),
  m_freq(0.0)
{
  	// Hamming window
	for (size_t i=0; i<fftSize; i++) {
		window[i] = 0.53836f - 0.46164f * cos(2.0f * M_PI * i / (fftSize - 1));
	}
}

CFft::~CFft()
{
	fftwf_destroy_plan(fftPlan);
	fftwf_free(fftIn);
}

static float sampleConvert(signed short sample) {
	return sample / 32768.0f;
}

static int match(std::vector<Peak> const& peaks, int pos, double freq) {
	int best = pos;
	if (fabs(peaks[pos-1].freq - freq) < fabs(peaks[best].freq - freq)) best = pos-1;
	if (fabs(peaks[pos+1].freq - freq) < fabs(peaks[best].freq - freq)) best = pos+1;
	return best;
}

void CFft::process(size_t nframes, signed short* indata)
{
	if (m_rate == 0.0) throw std::logic_error("Rate not set before calling CFft::process");
	// Precalculated constants
	const double freqPerBin = m_rate/(double)fftSize;
	const double phaseStep = 2.*M_PI*(double)fftStep/(double)fftSize;
	const double normCoeff = 4.0 / ((double)fftSize * fftSize);
	const double minMagnitude = pow(10, -60.0 / 10.0) / normCoeff;
	
	std::transform(indata, indata + nframes, std::back_inserter(sampleBuffer), sampleConvert);

	while (sampleBuffer.size() >= fftSize) {
		float peak = 0.0;
		for (size_t k=0; k<fftSize; k++) {
			float sample = sampleBuffer[k];
			peak = std::max(peak, sample * sample);
			fftIn[k] = sample * window[k];
		}
		m_peak = 10.0 * log10(peak); // Critical: atomic write
		
		sampleBuffer.erase(sampleBuffer.begin(), sampleBuffer.begin() + fftStep);

		fftwf_execute(fftPlan);

		// Process only up to 3000 Hz
		size_t kMax = std::min(fftSize / 2, (size_t)(3000.0 / freqPerBin));
		std::vector<Peak> peaks(kMax + 1);

		for (size_t k = 0; k <= kMax; ++k) {
			double real = fftOut[k][0];
			double imag = fftOut[k][1];
			double magnitude = real*real + imag*imag;
			double phase = atan2(imag, real);
			
			// process phase difference
			double delta = phase - fftLastPhase[k];
			fftLastPhase[k] = phase;
			// subtract expected phase difference
			delta -= k * phaseStep;
			// map delta phase into +/- M_PI interval
			delta = remainder(delta, 2.0 * M_PI);
			// calculate diff from bin center frequency
			delta /= phaseStep; // ((double)fftSize / fftStep) / (2.0 * M_PI);
			// process the k-th partials' true frequency
			double freq = (k + delta) * freqPerBin;
			
			if (magnitude > minMagnitude) {
				peaks[k].freq = freq;
				peaks[k].db = 10.0 * log10(normCoeff * magnitude);
			}
		}
		// Find the tones (collections of harmonics) from the array of peaks
		// TODO: proper handling of tones with "missing fundamental" (is this needed?)
		std::vector<Tone> tones;
		double db = -std::numeric_limits<double>::infinity();
		for (size_t k = 2; k < kMax; ++k) {
			// Prefilter out too low freqs, too silent bins and bins that are weaker than their neighbors
			if (peaks[k].freq < 80.0 || peaks[k].db < -60.0 || peaks[k].db < peaks[k-1].db || peaks[k].db < peaks[k+1].db) continue;
			// Find the base peak (fundamental frequency)
			int harmonic = 1;
			int misses = 0;
			for (int h = 2; k / h > 2; ++h) {
				double freq = peaks[k].freq / h;
				if (freq < 40.0 || ++misses > 3) break;
				int best = match(peaks, k / h, freq);
				if (peaks[best].db < -60.0 || fabs(peaks[best].freq / freq - 1.0) > .03) continue;
				misses = 0;
				harmonic = h;
			}
			std::vector<Tone>::iterator it = std::find(tones.begin(), tones.end(), peaks[k].freq / harmonic);
			if (it == tones.end()) {
				tones.push_back(Tone());
				it = tones.end() - 1;
			}
			it->combine(peaks[k], harmonic);
			db = std::max(db, it->db());
		}
		// Remove weak tones
		tones.erase(std::remove_if(tones.begin(), tones.end(), std::mem_fun_ref(&Tone::isWeak)), tones.end());

		/* Debug printout
		if (!tones.empty()) {
			std::cerr << "\x1B[2J\x1B[1;1H" << tones.size() << " tones\n" << std::fixed << std::setprecision(1);
			std::for_each(tones.begin(), tones.end(), std::mem_fun_ref(&Tone::print));
		}
		*/

		// The following block controls the tones list output.
		// - A tone is only enabled if it is found in old (m_oldTones) and current (tones) list.
		// - A tone is disabled if it is not found in either of these lists
		// In addition, a tone is always updated with the latest information, if it is found in
		// either of these two internal lists.
		{
			std::vector<Tone> is, un, tmp;
			// Calculate intersection: only those that are in both the old and the current tones (use the current one)
			std::set_intersection(tones.begin(), tones.end(), m_oldTones.begin(), m_oldTones.end(), std::back_inserter(is));
			// Calculate union: all those that are either tones (use the most recent one if both are available)
			std::set_union(tones.begin(), tones.end(), m_oldTones.begin(), m_oldTones.end(), std::back_inserter(un));
			// Take from m_tones only those that have played recently, into tmp (use the one from union)
			std::set_intersection(un.begin(), un.end(), m_tones.begin(), m_tones.end(), std::back_inserter(tmp));
			ScopedLock l(m_mutex); // Critical section: writing to m_tones
			m_tones.clear();
			// Combine tmp with the stable new tones (the intersection), into m_tones.
			std::set_union(tmp.begin(), tmp.end(), is.begin(), is.end(), back_inserter(m_tones));
		}

		// Find the singing frequency
		double freq = 0.0;
		for (size_t i = 0; i < m_tones.size(); ++i) {
			if (m_tones[i].db() < db - 4.0) continue;
			freq = m_tones[i].freq();
		}
		m_freq = freq; // Critical: atomic modification

		m_oldTones = tones;
	}
}

CRecord::CRecord(RecordCallback& callback, size_t rate, std::string deviceName):
  m_callback(callback),
  m_rate(rate),
  captureDevice(deviceName)
{}

CRecord::~CRecord()
{
}

#ifdef USE_ALSA_RECORD
int thread_func(void* userData)
{
	CRecord& rec = *(CRecord*)userData;
	try {
		signed short int buf[4096];
		int frames = 256;
		int nFrames;
		snd_pcm_t *alsaHandle = rec.getAlsaHandle();

		while( !CScreenManager::getSingletonPtr()->isFinished() ) {
			nFrames = 0;
			nFrames = snd_pcm_readi(alsaHandle, buf, frames);
		
			if(nFrames < 0) {
				snd_pcm_state_t s = snd_pcm_state(alsaHandle);
				switch (s) {
				  case SND_PCM_STATE_OPEN:
					throw std::runtime_error("ALSA device in open state - configuration failed");
				  case SND_PCM_STATE_PREPARED:
				  case SND_PCM_STATE_RUNNING:
				  case SND_PCM_STATE_DRAINING:
					throw std::runtime_error("ALSA snd_pcm_readi failed with no apparent reason");
				  case SND_PCM_STATE_SETUP:
				  case SND_PCM_STATE_XRUN:
					if (snd_pcm_prepare(alsaHandle) < 0) throw std::runtime_error("ALSA prepare failed");
					break;
				  case SND_PCM_STATE_PAUSED:
					throw std::runtime_error("ALSA device is paused, but it shouldn't be");
				  case SND_PCM_STATE_SUSPENDED:
					if (snd_pcm_resume(alsaHandle) < 0 && snd_pcm_prepare(alsaHandle) < 0 && snd_pcm_reset(alsaHandle) < 0) {
						throw std::runtime_error("ALSA resume from suspend failed");
					}
					break;
				  case SND_PCM_STATE_DISCONNECTED:
					throw std::runtime_error("ALSA device disconnected");
				};
				nFrames = snd_pcm_readi(alsaHandle, buf, frames);
				if (nFrames < 0) throw std::runtime_error("ALSA read failure, unable to recover");
			}
			if (nFrames > 0) rec.callback().process(nFrames, buf);
		}
	} catch (std::exception& e) {
		std::cerr << "FATAL ERROR: " << e.what() << std::endl;
	}
	return 1;
}
#endif

#ifdef USE_PORTAUDIO19_RECORD
static int recordCallback( const void *inputBuffer, void *outputBuffer, unsigned long framesPerBuffer,
			const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags, void *userData )
{
	((CRecord*)userData)->callback().process(framesPerBuffer,(signed short int *)inputBuffer);
	return paContinue;
}
#endif
#ifdef USE_PORTAUDIO18_RECORD
static int recordCallback( void *inputBuffer, void *outputBuffer, unsigned long framesPerBuffer,
			PaTimestamp outTime, void *userData )
{
	((CRecord*)userData)->callback().process(framesPerBuffer,(signed short int *)inputBuffer);
	return 0;
}
#endif

#ifdef USE_GSTREAMER_RECORD
static void _handoff_cb(GstElement *fakesink, GstBuffer *buffer, GstPad *pad, gpointer user_data)
{
	((CRecord*)userData)->callback().process(GST_BUFFER_SIZE(buffer)/2,(signed short int *)GST_BUFFER_DATA(buffer));
}
#endif

void CRecord::startThread()
{
#ifdef USE_ALSA_RECORD
	int result;
	snd_pcm_hw_params_t *hw_params;

	if ((result = snd_pcm_open(&alsaHandle, captureDevice.c_str(), SND_PCM_STREAM_CAPTURE, 0)) < 0) {
		throw std::runtime_error("Cannot open audio device " + captureDevice + ": " + snd_strerror(result));
	}

	snd_pcm_hw_params_malloc(&hw_params);
	snd_pcm_hw_params_any(alsaHandle, hw_params);
	snd_pcm_hw_params_set_access(alsaHandle, hw_params,SND_PCM_ACCESS_RW_INTERLEAVED);
	snd_pcm_hw_params_set_format(alsaHandle, hw_params,SND_PCM_FORMAT_S16_LE);
	snd_pcm_hw_params_set_rate_near(alsaHandle, hw_params, &m_rate, 0);
	snd_pcm_hw_params_set_channels(alsaHandle, hw_params, 1);
	snd_pcm_hw_params(alsaHandle, hw_params);
	snd_pcm_hw_params_free(hw_params);
	snd_pcm_prepare(alsaHandle);
	m_callback.setRate(m_rate);
	thread = SDL_CreateThread(thread_func, this);
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
	m_callback.setRate(DEFAULT_RATE);

	err = Pa_OpenStream( &stream, &inputParameters, NULL, DEFAULT_RATE, 50, paClipOff, recordCallback, this);
	#endif
	#ifdef USE_PORTAUDIO18_RECORD
	m_callback.setRate(DEFAULT_RATE);
	err = Pa_OpenStream( &stream, Pa_GetDefaultInputDeviceID(), 1, paInt16, NULL, paNoDevice, 0, paInt16, NULL, DEFAULT_RATE, 50, 0, 0, recordCallback, this);
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
		fprintf(stderr, "[Error] Failed to create GStreamer element: 'alsasrc'\n");
		exit(EXIT_FAILURE);
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
	g_signal_connect(G_OBJECT(sink), "handoff", G_CALLBACK(_handoff_cb), this);
	m_callback.setRate(DEFAULT_RATE);
	
	/* Link the elements together */
	caps = gst_caps_new_simple("audio/x-raw-int",
		"rate", G_TYPE_INT, DEFAULT_RATE,
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


std::string MusicalScale::getNoteStr(double freq) const {
	int id = getNoteId(freq);
	if (id == -1) return std::string();
	static const char * note[12] = {"C ","C#","D ","D#","E ","F ","F#","G ","G#","A ","A#","B "};
	std::ostringstream oss;
	// Acoustical Society of America Octave Designation System
	oss << note[id%12] << 2 + id / 12 << " (" << (int)round(freq) << " Hz)";
	return oss.str();
}

double MusicalScale::getNoteFreq(int id) const
{
	if (id == -1) return 0.0;
	return baseFreq * pow(2.0, (id - baseId) / 12.0);
}

int MusicalScale::getNoteId(double freq) const
{
	if (freq < 1.0) return -1;
	int id = baseId + 12.0 * log(freq / baseFreq) / log(2) + 0.5;
	return id < 0 ? -1 : id;
}

double MusicalScale::getNoteOffset(double freq) const {
	double frac = freq / getNoteFreq(getNoteId(freq));
	return 12.0 * log(frac) / log(2);
}

