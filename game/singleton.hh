#ifndef __SINGLETON_H__
#define __SINGLETON_H__

/// template for creating singletons
template <class T> class CSingleton {
  protected:
	/// pointer to object
	static T* ms_CSingleton;

  public:
	CSingleton() { ms_CSingleton = static_cast<T*>(this); }
	~CSingleton() { ms_CSingleton = 0; }
	/// gets reference to singleton object
	inline static T& getSingleton() { return *ms_CSingleton; }
	/// gets pointer to singleton object
	inline static T* getSingletonPtr() { return ms_CSingleton; }
};

#endif
