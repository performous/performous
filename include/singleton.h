#ifndef __SINGLETON_H__
#define __SINGLETON_H__

template <class T> class CSingleton {
	protected:
	static T* ms_CSingleton;
	public:
	CSingleton(void) { ms_CSingleton = static_cast<T*>(this); }
	~CSingleton(void) { ms_CSingleton = NULL; }
	inline static T& getSingleton(void) { return *ms_CSingleton; }
	inline static T* getSingletonPtr(void) { return ms_CSingleton; }
};

#endif
