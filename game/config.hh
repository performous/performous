#include <boost/any.hpp>
#include <map>

extern std::map<std::string, boost::any> config;

template<int min, int max> class Integer {
  public:
	Integer(int value): m_value(clamp(value, min, max)) {}
	Integer& operator=(int value) { m_value = clamp(value, min, max); return *this; }
	int val() const { return m_value; }
	int& val() { return m_value; }
  private:
	int m_value;
};

