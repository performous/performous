#undef DLL_PUBLIC

#if defined _WIN32 || defined __CYGWIN__

#ifdef BUILDING_DLL
#define DLL_PUBLIC __declspec(dllexport)
#else
#define DLL_PUBLIC __declspec(dllimport)
#endif

#else

#if __GNUC__ >= 4
#define DLL_PUBLIC __attribute__ ((visibility("default")))
#else
#define DLL_PUBLIC
#endif

#endif

