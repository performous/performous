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
		char * getDescription() {return description;};
		virtual char * getValue()=0;
	protected:
		virtual void apply()=0;
		char * description;
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

#endif
