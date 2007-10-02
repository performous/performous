#ifndef __SINGLETON_H__
#define __SINGLETON_H__

template <class T> class CSingleton {
  protected:
	static T* ms_CSingleton;
  public:
	CSingleton() { ms_CSingleton = static_cast<T*>(this); }
	~CSingleton() { ms_CSingleton = 0; }
	inline static T& getSingleton() { return *ms_CSingleton; }
	inline static T* getSingletonPtr() { return ms_CSingleton; }
};

#endif
