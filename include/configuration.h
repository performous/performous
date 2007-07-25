#ifndef __CONFIGURATION_H__
#define __CONFIGURATION_H__

#include "../config.h"

class CConfiguration {
	public:
		CConfiguration() {};
		virtual ~CConfiguration() {};
		virtual bool isLast()=0;
		virtual bool isFirst()=0;
		virtual void setNext()=0;
		virtual void setPrevious()=0;
		const char * getDescription() {return description;};
		virtual char * getValue()=0;
	protected:
		virtual void apply()=0;
		const char * description;
};

class CConfigurationFullscreen : public CConfiguration {
	public:
		CConfigurationFullscreen();
		~CConfigurationFullscreen();
		bool isLast();
		bool isFirst();
		void setNext();
		void setPrevious();
		char * getValue();
	protected:
		void apply();
		bool fullscreen;
};

class CConfigurationDifficulty : public CConfiguration {
	public:
		CConfigurationDifficulty();
		~CConfigurationDifficulty();
		bool isLast();
		bool isFirst();
		void setNext();
		void setPrevious();
		char * getValue();
	protected:
		void apply();
		unsigned int difficulty;
};

class CConfigurationAudioVolume : public CConfiguration {
	public:
		CConfigurationAudioVolume();
		~CConfigurationAudioVolume();
		bool isLast();
		bool isFirst();
		void setNext();
		void setPrevious();
		char * getValue();
	protected:
		void apply();
		unsigned int audioVolume;
		char value[32];
};
#endif
