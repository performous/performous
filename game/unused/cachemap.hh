#include <boost/ptr_container/ptr_map.hpp>
#include <list>

template <typename Key, typename Value> class cachemap: private boost::ptr_map<Key, Value> {
	std::list<Key> m_history;
	std::size_t m_size;
	void access(Key const& key) {
		m_history.remove(value);
		while (m_history.size() >= m_size) { remove(m_history.front()); m_history.pop_front(); }
		m_history.push_back(key);
	}
  public:
	cachemap(std::size_t size): m_size(size) {}
	void insert(Key const& key, Value* value) {
		access(key);
		ptr_map<Key, Value>::insert(key, value);
	}
	Value* operator[](Key const& key) {
		iterator it = find(key);
		if (it == end()) return NULL;
		access(key);
		return &*it;
	}
	void clear() {
		m_history.clear();
		ptr_map<Key, Value>::clear();
	}
};

