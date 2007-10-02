#ifndef AUDIO_DEV_HPP_INCLUDED
#define AUDIO_DEV_HPP_INCLUDED

#include "audio.hpp"
#include <map>
#include <stdexcept>

namespace da {
	template<typename> class register_pattern {
	};

	class record::dev {
		friend class record;
		typedef dev* (*create_func_t)(settings& s);
		typedef std::map<std::string, create_func_t> map_t;
	  protected:
		static map_t& map() {
			static map_t dev;
			return dev;
		}
		class reg_dev {
			std::string name;
		  public:
			reg_dev(std::string const& name, create_func_t f): name(name) {
				map_t& m = map();
				if (m.find(name) != m.end()) throw std::invalid_argument("Internal error: record::dev::reg_dev(" + name + "): name already reserved");
				m[name] = f;
			}
			~reg_dev() { map().erase(name); }
		};
		virtual ~dev() {}
	};
}

#endif
