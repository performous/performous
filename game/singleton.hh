#pragma once

/// template for creating singletons
/// FIXME This class is completey broken:
/// a singlton should afford that one and only one instance of a class can
/// exists... and this do not guaranty nothing....
/// Doing so, there should be a factory to create the object so that the
/// "Instance" is manaed internally.  Here, uniqueness is not handled, nor
/// lifetime... so this is more or less a handler for a global pointer... that
/// can be overwritten without any check....  Taht said, singleton is an anti
/// patern, so better drop it in future reworks.
template <class T> class Singleton {
  protected:
	/// pointer to object
	inline static T* ms_Singleton{};

  public:
	Singleton() { ms_Singleton = static_cast<T*>(this); }
	~Singleton() { ms_Singleton = 0; }
	/// gets reference to singleton object
	inline static T& getSingleton() { return *ms_Singleton; }
	/// gets pointer to singleton object
	inline static T* getSingletonPtr() { return ms_Singleton; }
};
