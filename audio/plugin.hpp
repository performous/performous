#ifndef PLUGIN_HPP_INCLUDED
#define PLUGIN_HPP_INCLUDED

#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace da {
	namespace util {
		namespace {
			template <typename T> typename T::first_type get_first(T const& pair) { return pair.first; }
		}
		template <typename Base, typename Arg = std::string, typename Key = std::string> class plugin {
		  public:
			struct invalid_key_error: public std::logic_error {
				invalid_key_error(std::string const& msg): logic_error(msg) {}
			};
			class regbase;
		  private:
			typedef std::map<Key, regbase*> map_t;
			// Important: the map has to be wrapped inside a function instead of it
			// being a static member variable of this class to ensure that it gets
			// initialized when it is used the first time. If it were static member,
			// the initialization (on program startup) might occur too late.
			static map_t& map() {
				static map_t m;
				return m;
			}
		  public:
			class iterator: public std::iterator<std::bidirectional_iterator_tag, Key> {
				typename map_t::const_iterator m_it;
			  public:
				iterator(typename map_t::iterator it): m_it(it) {}
				iterator(typename map_t::const_iterator it): m_it(it) {}
				Key const& operator*() const { return m_it->first; }
				Key const* operator->() const { return &m_it->first; }
				Base* operator()(Arg arg) { return (*m_it->second)(arg); }
				bool operator==(iterator const& other) const { return m_it == other.m_it; }
				bool operator!=(iterator const& other) const { return m_it != other.m_it; }
				iterator& operator++() { ++m_it; return *this; }
				iterator& operator--() { --m_it; return *this; }
				iterator operator++(int) { return iterator(m_it++); }
				iterator operator--(int) { return iterator(m_it--); }
			};
			static iterator begin() { return map().begin(); }
			static iterator end() { return map().end(); }
			static iterator find(Key const& key) {
				map_t const& m = map();
				typename map_t::const_iterator it = m.find(key);
				if (it == m.end()) throw invalid_key_error("plugin::find: Requested key not found");
				return it;
			}
			static typename map_t::size_type size() { return map().size(); }
			static bool empty() { return map().empty(); }
			/** @short An abstract base for classes that can create Base*. **/
			class regbase {
				typename map_t::iterator m_it;
			  public:
				regbase(Key const& key) {
					std::pair<typename map_t::iterator, bool> p = map().insert(std::pair<Key, regbase*>(key, this));
					// Note: throwing normally crashes the application.
					if (!p.second) throw invalid_key_error("plugin::regbase: Key already used");
					m_it = p.first;
				}
				virtual ~regbase() { map().erase(m_it); }
				virtual Base* operator()(Arg) const = 0;
			};
			/** @short A construct implementation that passes arg to the constructor of Class. **/
			template <typename Class> struct reg: public regbase {
				reg(Key const& key): regbase(key) {}
				Base* operator()(Arg arg) const { return new Class(arg); }
			};
		};
	}
}

#endif

