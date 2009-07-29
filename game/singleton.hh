#pragma once
#ifndef __SINGLETON_H__
#define __SINGLETON_H__

/// template for creating singletons
template <class T> class Singleton {
  protected:
	/// pointer to object
	static T* ms_Singleton;

  public:
	Singleton() { ms_Singleton = static_cast<T*>(this); }
	~Singleton() { ms_Singleton = 0; }
	/// gets reference to singleton object
	inline static T& getSingleton() { return *ms_Singleton; }
	/// gets pointer to singleton object
	inline static T* getSingletonPtr() { return ms_Singleton; }
};

#endif
