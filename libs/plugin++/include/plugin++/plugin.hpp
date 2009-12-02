#pragma once
#ifndef PLUGIN_HPP_INCLUDED
#define PLUGIN_HPP_INCLUDED

#include <map>
#include <stdexcept>
#include <string>

#if defined _WIN32 || defined __CYGWIN__

#ifdef PLUGIN_MASTER
#define PLUGIN_REGISTRY_API __declspec(dllexport)
#else
#define PLUGIN_REGISTRY_API __declspec(dllimport)
#endif

#else

#if __GNUC__ >= 4
#define PLUGIN_REGISTRY_API __attribute__ ((visibility("default")))
#else
#define PLUGIN_REGISTRY_API
#endif

#endif

namespace plugin {

    /**
    * @short A polymorphic plugin factory.
    *
    * This class provides facilities for registering implementations of Base
    * and for requesting instances of implementations by their names (of
    * type Key), passing an argument of type Arg to the constructor of the
    * implementation.
    *
    * More generically, a Base* is returned using a handler previously
    * registered with a specific Key, calling handler's operator()(Arg arg).
    *
    * See documentation for usage examples.
    **/
    template <typename Base,
      typename Arg = std::string const&,
      typename Key = std::string>
    class PLUGIN_REGISTRY_API registry {
      public:
        typedef Base base_type;
        typedef Arg arg_type;
        typedef Key key_type;
        struct invalid_key_error: public std::logic_error {
            invalid_key_error(std::string const& msg): logic_error(msg) {}
        };
        class handler;
      private:
        typedef std::multimap<Key, handler*> map_t;
        // Important: the map has to be wrapped inside a function instead of it
        // being a static member variable of this class to ensure that it gets
        // initialized when it is used the first time. If it were static member,
        // the initialization (on program startup) might occur too late.
        static map_t& map() {
            static map_t m;
            return m;
        }
      public:
        /** An iterator for browsing the available keys. **/
        class iterator:
          public std::iterator<std::bidirectional_iterator_tag, Key>
        {
            typename map_t::const_iterator m_it;
          public:
            iterator(typename map_t::iterator it): m_it(it) {}
            iterator(typename map_t::const_iterator it): m_it(it) {}
            /** Access the key. **/
            Key const& operator*() const { return m_it->first; }
            /** Access a member of the key. **/
            Key const* operator->() const { return &m_it->first; }
            /**
            * Activate the handler attached to the current key.
            * Normally new's an object passing arg to its constructor.
            **/
            Base* operator()(Arg arg) { return (*m_it->second)(arg); }
            bool operator==(iterator const& r) const { return m_it == r.m_it; }
            bool operator!=(iterator const& r) const { return m_it != r.m_it; }
            iterator& operator++() { ++m_it; return *this; }
            iterator& operator--() { --m_it; return *this; }
            iterator operator++(int) { return iterator(m_it++); }
            iterator operator--(int) { return iterator(m_it--); }
        };
        /** Get an iterator to the first key. **/
        static iterator begin() { return map().begin(); }
        /** Get the end iterator. **/
        static iterator end() { return map().end(); }
        /**
        * Find a specific key.
        * @throw plugin::invalid_key_error if the key is not found
        * @return iterator to the option matching the key
        **/
        static iterator find(Key const& key) {
            map_t const& m = map();
            typename map_t::const_iterator it = m.find(key);
            if (it == m.end())
              throw invalid_key_error("plugin::find: Requested key not found");
            return it;
        }
        /** Get the number of options. **/
        static typename map_t::size_type size() { return map().size(); }
        /** Test if there are any options. **/
        static bool empty() { return map().empty(); }
        /**
        * @short An abstract base for handler classes.
        * Registers itself on construct, unregisters on destruct.
        **/
        class handler {
            typename map_t::iterator m_it;
          public:
			handler(Key const& key) {
				// MSVC9 gives warning about using this in initlist, so we do it here:
				m_it = map().insert(std::pair<Key, handler*>(key, this));
            }
            virtual ~handler() { map().erase(m_it); }
            virtual Base* operator()(Arg) const = 0;
        };
    };
    /**
    * @short A handler that returns new Class(arg).
    * An instance of this class is normally created as a static (file scope)
    * variable so that the constructor gets called on program startup and
    * that the destructor gets called on program exit, or similarly on
    * dynamic library load/unload.
    **/
    template <typename Plugin, typename Class>
    struct simple: public Plugin::handler {
        simple(typename Plugin::key_type const& key): Plugin::handler(key) {}
        typename Plugin::base_type* operator()(typename Plugin::arg_type arg) const {
            return new Class(arg);
        }
    };
    template <typename Plugin, typename Class>
    struct type0: public Plugin::handler {
        type0(typename Plugin::key_type const& key): Plugin::handler(key) {}
        typename Plugin::base_type* operator()(typename Plugin::arg_type) const {
            return new Class();
        }
    };
}

#endif

