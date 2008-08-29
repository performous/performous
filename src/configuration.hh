#ifndef __CONFIGURATION_H__
#define __CONFIGURATION_H__

#include <string>

class CConfiguration {
  public:
	CConfiguration(std::string const& description): m_description(description) {};
	virtual ~CConfiguration() {};
	virtual void setNext() = 0;
	virtual void setPrevious() = 0;
	std::string const& getDescription() const { return m_description; };
	virtual std::string getValue() const = 0;
  private:
	std::string m_description;
};

class CConfigurationFullscreen: public CConfiguration {
  public:
	CConfigurationFullscreen();
	void setNext() { m_fs = !m_fs; apply(); }
	void setPrevious() { setNext(); }
	std::string getValue() const;
  private:
	void apply();
	bool m_fs;
};

class CConfigurationAudioVolume: public CConfiguration {
  public:
	CConfigurationAudioVolume(std::string const& title, unsigned int& volume);
	void setNext();
	void setPrevious();
	std::string getValue() const;
  private:
	void apply();
	unsigned int& m_volume;
};

#endif
