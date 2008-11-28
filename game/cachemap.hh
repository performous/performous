#include <boost/ptr_container/ptr_map.hpp>
#include <list>

template <typename Key, typename Value> class Cachemap {
	typedef boost::ptr_map<Key, Value> Map;
	Map m_map;
	std::list<Key> m_history;
	std::size_t m_size;
	void access(Key const& key) {
		m_history.remove(key);
		while (m_history.size() >= m_size) { m_map.erase(m_history.front()); m_history.pop_front(); }
		m_history.push_back(key);
	}
  public:
	Cachemap(std::size_t size): m_size(size) {}
	typename Map::iterator insert(Key key, Value* value) {
		access(key);
		return m_map.insert(key, value).first;
	}
	Value& operator[](Key const& key) {
		typename Map::iterator it = m_map.find(key);
		if (it == m_map.end()) it = insert(key, new Value(key)); else access(key);
		return *it->second;
	}
	void clear() {
		m_history.clear();
		m_map.clear();
	}
};

