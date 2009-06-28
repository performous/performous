#ifndef ALSA_HPP_INCLUDED
#define ALSA_HPP_INCLUDED

/**

@file alsa.hpp
@brief A partial low-level C++ API to ALSA.
@version 0.8
@author Lasse Kärkkäinen <tronic>
@license GNU LGPL 2.1 or later

This is a header-only C++ wrapper for the ALSA library. This means that you do
not need to add any new binary files to your build and you will only need to
link with -lasound, as if you were using the C API directly. GCC will probably
optimize all of the wrapper overhead away in any case, leaving you with a safer
and easier API to ALSA, but leaving your binaries just as if you had used C.

The library is designed to be very closely related to the C API, so that you
can still see what is going on under the hood, and also so that porting existing
applications to it is trivial. The interoperatibility between C and C++ APIs is
a major design goal. What this means is that you can freely mix C and C++ API
calls with no problems.


Usage example:

alsa::pcm alsa_pcm("default", SND_PCM_STREAM_PLAYBACK);  // Create a PCM object

The above creates an equivalent for the snd_pcm_t*, which is what you need to
play or record anything. Make sure that the alsa_pcm object stays alive as long
as you need it (and preferrably not longer) by putting it inside a suitable
class or even inside the main() function. The object cannot be copied, but you
can pass references to it to other objects or functions.

The alsa_pcm also automatically converts into snd_pcm_t* as required, so you can
use it as an argument for the C API functions.

Next you'll need to configure it:

unsigned int rate = 44100;
unsigned int period;

alsa::hw_config(alsa_pcm)  // Create a new config space
  .set(SND_PCM_ACCESS_RW_INTERLEAVED)
  .set(SND_PCM_FORMAT_S16_LE)
  .rate_near(rate)
  .channels(1)
  .period_size_first(period)  // Get the smallest available period size
  .commit();  // Apply the config to the PCM

alsa::hw_config(pcm) constructs a new config space, using the current settings
from the PCM, if available, or the "any" space, if not set yet. The any space
contains all available hardware configurations and you have to narrow it down
to exactly one option by setting some parameters. Trying to narrow it too much
(by asking an amount of channels that is not available, for example) causes a
failure.

In case of failure, an alsa::error is thrown. When this happens, the commit part
never gets executed and thus the result is not stored to alsa_pcm and the
failed operation will have no effect (even to the temporary hw_config object,
which gets destroyed when the exception flies). However, all the operations
already performed successfully remain in effect.

The rate_near functions behaves like the C API *_near functions
do: they take the preferred value as an argument and then they modify the
argument, returning the actual rate. For example, if your sound card only
supports 48000 Hz, rate will be set to that on that call, even if some later
part, such as setting the number of channels, fails.

In the example above, a temporary object of type alsa::hw_config was used, but
you can also create a variable of it, should you need to test something in
between, or if you want to call the C API functions directly (hw_config
converts automatically into hw_params, which converts into snd_hw_params_t*).

For this, you may use a block like this:

{
    alsa::hw_config hw(alsa_pcm);
    hw.set(SND_ACCESS_(SND_PCM_ACCESS_MMAP_INTERLEAVED);
    hw.set(SND_PCM_FORMAT_FLOAT_BE);
    if (!snd_pcm_hw_params_is_full_duplex(hw)) hw.channels(2);
    hw.commit();
}

(anyone got a better example?)


Software configuration works in the same way, using alsa::sw_config, just the
parameters to be adjusted are different.

When constructed, both sw_config and hw_config try to load config from the given
PCM. If that fails, sw_config throws, but hw_config still tries loading the any
space. Alternatively, you may supply a snd_pcm_hw/sw_params_t const* as a second
argument for the constructor to get a copy of the contents of that that instead.

The contents may be loaded (overwrites the old contents) with these functions:
  .any()            Get the "any" configuration (hw_config only)
  .current()        Get the currently active configuration from PCM
Once finished with the changes, you should call:
  .commit()         Store current config space to the PCM

For enum values SND_PCM_*, you may use the following functions:
  .get(var)         Get the current setting into the given variable
  .set(val)         Set the value requested (also supports masks)
  .enum_test(val)   The same as .set, except that it does not set anything
  .enum_first(var)  Set the first available option, store the selection in var
  .enum_last(var)   Set the last available option, store the selection in var

The parameter to manipulate is chosen based on the argument type. The enum_*
functions and masks are only available for hardware parameters, not for
sw_param.

For integer values (times, sizes, counts), these functions are available:
  .get_X(var)       Get the current setting into the given variable
  .X(val)           Set the value requested
For ranges, the following can also be used:
  .get_X_min(var)   Get the smallest available value into var
  .get_X_max(var)   Get the largest available value into var
  .X_min(var)       Remove all values smaller than var and store the new
                    smallest value into var.
  .X_max(var)       Remove all values larger than var, store new max in var.
  .X_minmax(v1, v2) Limit to [v1, v2], store new range in v1, v2.
  .X_near(var)      Set the closest available value and store it in var
  .X_first(var)     Set the first available option, store the selection in var
  .X_last(var)      Set the last available option, store the selection in var

For booleans, these functions are available:
  .get_X(var)    Get the current setting (var must be unsigned int or bool)
  .set_X(val)    Set the value (val can be anything that converts into bool)
  
Replace X with the name of the parameter that you want to set. Consult the ALSA
C library reference for available options. All functions that modify their
arguments require the same type as is used in the C API (often unsigned int or
snd_pcm_uframes_t). The only exception is with bool types, where both bool and
unsigned int are accepted.

For those ranged parameters that support the dir argument (see ALSA docs), the
default value is always 0 when writing and NULL (none) when reading. You may
supply the preferred dir value or variable as the second argument and then the
value will be used or the result will be stored in the variable supplied.

For example, the following calls are equivalent:
snd_pcm_hw_params_set_format(pcm, hw, SND_PCM_FORMAT_FLOAT_LE)
  <=> hw.set(SND_PCM_FORMAT_FLOAT_LE)
snd_pcm_hw_params_set_rate_resample(pcm, hw, 1) <=> hw.rate_resample(true)
snd_pcm_hw_params_set_channels_near(pcm, hw, &num) <=> hw.channels_near(num)
snd_pcm_hw_params_get_rate(hw, &rate, NULL) <=> hw.get_rate(rate)
snd_pcm_hw_params_get_rate(hw, &rate, &dir) <=> hw.get_rate(rate, dir)


... except for the fact that the C++ versions also check the return values and
throw alsa::error if anything goes wrong. This inherts from std::runtime_error
and thus eventually from std::exception, so you can catch pretty much everything
by catching that somewhere in your code:

try {
	// do everything here
} catch (std::exception& e) {
	std::cerr << "FATAL ERROR: " << e.what() << std::endl;
}

If you need to know the error code, the ALSA C function name or the C++ function
within which the error originated, you may call e.code(), e.cfunc() or e.func()
after catching alsa::error& e. If you just want a developer-readable error
message that contains all this information, use e.what().

Please note that some programming errors (using the wrapper incorrectly) are
detected and handled within the C++ API. In this case std::logic_error or
something derived from it is thrown instead of the custom exceptions.

When you call the C API functions directly, the ALSA_CHECKED macro may prove to
be useful. It is used internally by the library for testing errors and throwing
exceptions when calling the C functions. It will throw alsa::error with a
description of the error if the function returns a negative value.

The macro is well-behaving, as it only calls an internal helper function,
evaluating the arguments given exactly once. The return value can also still be
used (will return only >= 0):

Usage: ALSA_CHECKED(snd_pcm_whatever, (arg1, arg2, arg2));

Note: a comma between function name and arguments.


MMAP transfers can be done with the alsa::mmap RAII wrapper.

Usage:

// First we need to call avail_update (storing the return value is optional)
snd_pcm_uframes_t count = ALSA_CHECKED(snd_pcm_avail_update, (pcm));
alsa::mmap mmap(pcm, 256) // Begin MMAP transfer, request 256 frames

// Process the audio (mmap.frames() frames of it, accessible via mmap.area(n),
// starting at offset mmap.offset())

mmap.commit();  // If you used less than mmap.frames() frames, commit(n) instead

// You may not use the mmap object after the commit. The transfer is terminated
// properly (by commiting 0 frames w/o error handling) if no commit is done
// before the object goes out of scope.


In case you really want to get low-level, alsa::hw_params and alsa::sw_params
are offered. These only contain the corresponding snd_pcm_*_params_t, but they
allocate and free memory automatically and they can also properly copy the
struct contents when they get copied. Be aware that the structure contents are
not initialized during construction, so you have to initialize it yourself (just
like with the C API). They are used internally by hw_config and sw_config and
normally it should be better to use these instead of dealing directly with the
params.

**/

#include <alsa/asoundlib.h>
#include <stdexcept>
#include <string>

/**
* A macro that executes func with the given args and tests for errors.
* Examples of use:
*   ALSA_CHECKED(snd_pcm_recover, (pcm, e.code(), 0));
*   snd_pcm_uframes_t count = ALSA_CHECKED(snd_pcm_avail_update, (pcm));
* @param func the function name
* @param args arguments in parenthesis
* @return the return value of the function
* @throws alsa::error if the return value is smaller than zero.
**/
#define ALSA_CHECKED(cfunc, args) alsa::internal::check(cfunc args, #cfunc, __PRETTY_FUNCTION__)

namespace alsa {
	/** @short Exception class **/
	class error: public std::runtime_error {
		int m_code;
		std::string m_cfunc;
		std::string m_func;
	  public:
		error(std::string const& cf, int errcode, std::string const& f):
		  std::runtime_error(f + ": " + cf + " failed: " + std::string(snd_strerror(errcode))),
		  m_code(errcode),
		  m_cfunc(cf),
		  m_func(f)
		{}
		~error() throw() {}
		/** Error code returned by the function **/
		int code() const { return m_code; }
		/** Get the C API function that returned the error code **/
		std::string const& cfunc() const { return m_cfunc; }
		/** Get the function that used ALSA_CHECKED **/
		std::string const& func() const { return m_func; }
	};
	
	namespace internal {
		/** For internal use only: a function used by the macro ALSA_CHECKED **/
		template<typename T> T check(T ret, char const* cfunc, std::string prettyfunc) {
			if (ret >= 0) return ret;
			// Remove return type, qualifiers and arguments from __PRETTY_FUNCTION__
			std::string::size_type end = prettyfunc.find('(');
			std::string::size_type begin = prettyfunc.find(' ');
			if (begin == std::string::npos || begin > end) begin = 0; else ++begin;
			for (std::string::size_type tmp; (tmp = prettyfunc.rfind(' ', end)) != std::string::npos && tmp > begin; end = tmp) {};
			throw error(cfunc, ret, prettyfunc.substr(begin, end - begin));
		}
	}

#define ALSA_HPP_NONCOPYABLE(c) c(c const&); c const& operator=(c const&);


	/**
	* @short A minimal RAII wrapper for ALSA ctl
	* Automatically converts into snd_ctl_t* as needed, so the ALSA C API
	* can be used directly with this.
	**/
	class ctl { ALSA_HPP_NONCOPYABLE(ctl)
		snd_ctl_t* m_handle;
	  public:
		ctl(std::string const& devname, int mode = 0) {
			ALSA_CHECKED(snd_ctl_open, (&m_handle, devname.c_str(), mode));
        }
		~ctl() { snd_ctl_close(m_handle); }
		operator snd_ctl_t*() { return m_handle; }
		operator snd_ctl_t const*() const { return m_handle; }
	};

	/**
	* @short A minimal RAII wrapper for ALSA ctl card info
	* Automatically converts into snd_ctl_card_info_t* as needed, so the ALSA C API
	* can be used directly with this.
	**/
	class ctl_card_info {
		snd_ctl_card_info_t* m_handle;
		void init() { ALSA_CHECKED(snd_ctl_card_info_malloc, (&m_handle)); }
	  public:
		ctl_card_info() { init(); snd_ctl_card_info_clear(m_handle); }
		explicit ctl_card_info(snd_ctl_card_info_t const* orig) { init(); *this = orig; }
		ctl_card_info(ctl_card_info const& orig) { init(); *this = orig; }
		~ctl_card_info() { snd_ctl_card_info_free(m_handle); }
		ctl_card_info& operator=(snd_ctl_card_info_t const* orig) { snd_ctl_card_info_copy(m_handle, orig); return *this; }
		ctl_card_info& operator=(ctl_card_info const& orig) { snd_ctl_card_info_copy(m_handle, orig); return *this; }
		operator snd_ctl_card_info_t*() { return m_handle; }
		operator snd_ctl_card_info_t const*() const { return m_handle; }
	};

	/**
	* @short A minimal RAII wrapper for ALSA PCM stream info
	* Automatically converts into snd_pcm_info_t* as needed, so the ALSA C API
	* can be used directly with this.
	**/
	class pcm_info { ALSA_HPP_NONCOPYABLE(pcm_info)
		snd_pcm_info_t* m_handle;
	  public:
		pcm_info(int dev = 0, int subdev = 0, snd_pcm_stream_t stream = SND_PCM_STREAM_PLAYBACK) {
			ALSA_CHECKED(snd_pcm_info_malloc, (&m_handle));
			snd_pcm_info_set_device(m_handle, dev);
			snd_pcm_info_set_subdevice(m_handle, subdev);
			snd_pcm_info_set_stream(m_handle, stream);
        }
		~pcm_info() { snd_pcm_info_free(m_handle); }
		operator snd_pcm_info_t*() { return m_handle; }
		operator snd_pcm_info_t const*() const { return m_handle; }
	};

	/**
	* @short A minimal RAII wrapper for ALSA PCM.
	* Automatically converts into snd_pcm_t* as needed, so the ALSA C API
	* can be used directly with this.
	**/
	class pcm { ALSA_HPP_NONCOPYABLE(pcm)
		snd_pcm_t* m_pcm;
	  public:
		pcm(char const* device, snd_pcm_stream_t stream, int mode = 0): m_pcm() {
			ALSA_CHECKED(snd_pcm_open, (&m_pcm, device, stream, mode));
		}
		~pcm() { snd_pcm_close(m_pcm); }
		operator snd_pcm_t*() { return m_pcm; }
		operator snd_pcm_t const*() const { return m_pcm; }
	};

	// RAII wrapper for snd_pcm_hw/sw_params_t types.

	class status {
		snd_pcm_status_t* m_handle;
		void init() { ALSA_CHECKED(snd_pcm_status_malloc, (&m_handle)); }
	  public:
		status(): m_handle() { init(); }
		~status() { snd_pcm_status_free(m_handle); }
		status(status const& orig): m_handle() { init(); *this = orig; }
		status(snd_pcm_status_t const* orig): m_handle() { init(); *this = orig; }
		status& operator=(status const& params) { *this = params.m_handle; return *this; }
		status& operator=(snd_pcm_status_t const* params) {
			if (m_handle != params) snd_pcm_status_copy(m_handle, params);
			return *this;
		}
		operator snd_pcm_status_t*() { return m_handle; }
		operator snd_pcm_status_t const*() const { return m_handle; }
	};
	
#define ALSA_HPP_PARAMWRAPPER(type) \
	class type##_params {\
		snd_pcm_##type##_params_t* m_handle;\
		void init() { ALSA_CHECKED(snd_pcm_##type##_params_malloc, (&m_handle)); }\
	  public:\
		type##_params(): m_handle() { init(); }\
		~type##_params() { snd_pcm_##type##_params_free(m_handle); }\
		type##_params(type##_params const& orig): m_handle() { init(); *this = orig; }\
		explicit type##_params(snd_pcm_##type##_params_t const* orig): m_handle() { init(); *this = orig; }\
		type##_params& operator=(type##_params const& params) { *this = params.m_handle; return *this; }\
		type##_params& operator=(snd_pcm_##type##_params_t const* params) {\
			if (m_handle != params) snd_pcm_##type##_params_copy(m_handle, params);\
			return *this;\
		}\
		operator snd_pcm_##type##_params_t*() { return m_handle; }\
		operator snd_pcm_##type##_params_t const*() const { return m_handle; }\
	};

	ALSA_HPP_PARAMWRAPPER(hw)
	ALSA_HPP_PARAMWRAPPER(sw)
#undef ALSA_HPP_PARAMWRAPPER

// Various helper macros used for generating code for hw_config and sw_config

#define ALSA_HPP_FUNC(name, suffix) ALSA_HPP_TEMPLATE(& name(), suffix, (m_pcm, m_params))

#define ALSA_HPP_VARGET(name, type) \
  ALSA_HPP_TEMPLATE(& get_##name(type& val), _get_##name, (m_params, &val))\
  ALSA_HPP_TEMPLATE(const& get_##name(type& val) const, _get_##name, (m_params, &val))

#define ALSA_HPP_VAR(name, type) ALSA_HPP_VARGET(name, type)\
  ALSA_HPP_TEMPLATE(& name(type val), _set_##name, (m_pcm, m_params, val))

#define ALSA_HPP_ENUMVARMINIMAL(name) \
  ALSA_HPP_TEMPLATE(& get(snd_pcm_##name##_t& name_), _get_##name, (m_params, &name_))\
  ALSA_HPP_TEMPLATE(const& get(snd_pcm_##name##_t& name_) const, _get_##name, (m_params, &name_))\
  ALSA_HPP_TEMPLATE(& set(snd_pcm_##name##_t name_), _set_##name, (m_pcm, m_params, name_))

#define ALSA_HPP_ENUMVAR(name) ALSA_HPP_ENUMVARMINIMAL(name)\
  ALSA_HPP_TEMPLATE(& enum_test(snd_pcm_##name##_t name), _test_##name, (m_pcm, m_params, name))\
  ALSA_HPP_TEMPLATE(& enum_first(snd_pcm_##name##_t& name), _set_##name##_first, (m_pcm, m_params, &name))\
  ALSA_HPP_TEMPLATE(& enum_last(snd_pcm_##name##_t& name), _set_##name##_last, (m_pcm, m_params, &name))\
  ALSA_HPP_TEMPLATE(& set(snd_pcm_##name##_mask_t* mask), _set_##name##_mask, (m_pcm, m_params, mask))
  
#define ALSA_HPP_BOOLVAR(name) \
  ALSA_HPP_CLASS& get_##name(bool& val) { unsigned int tmp; get_##name(tmp); val = tmp; return *this; }\
  /*ALSA_HPP_CLASS const& get_##name(bool& val) const { unsigned int tmp; get_##name(tmp); val = tmp; return *this; }*/\
  ALSA_HPP_TEMPLATE(& get_##name(unsigned int& val), _get_##name, (m_pcm, m_params, &val))\
  /*ALSA_HPP_TEMPLATE(const& get_##name(unsigned int& val) const, _get_##name, (m_pcm, m_params, &val))*/\
  ALSA_HPP_TEMPLATE(& name(bool val = true), _set_##name, (m_pcm, m_params, val))

#define ALSA_HPP_RANGEVAR(name, type) ALSA_HPP_VAR(name, type)\
  ALSA_HPP_TEMPLATE(& get_##name##_min(type& min), _get_##name##_min, (m_params, &min))\
  ALSA_HPP_TEMPLATE(const& get_##name##_min(type& min) const, _get_##name##_min, (m_params, &min))\
  ALSA_HPP_TEMPLATE(& get_##name##_max(type& max), _get_##name##_max, (m_params, &max))\
  ALSA_HPP_TEMPLATE(const& get_##name##_max(type& max) const, _get_##name##_max, (m_params, &max))\
  ALSA_HPP_TEMPLATE(& name##_min(type& min), _set_##name##_min, (m_pcm, m_params, &min))\
  ALSA_HPP_TEMPLATE(& name##_max(type& max), _set_##name##_max, (m_pcm, m_params, &max))\
  ALSA_HPP_TEMPLATE(& name##_minmax(type& min, type& max), _set_##name##_minmax, (m_pcm, m_params, &min, &max))\
  ALSA_HPP_TEMPLATE(& name##_near(type& val), _set_##name##_near, (m_pcm, m_params, &val))\
  ALSA_HPP_TEMPLATE(& name##_first(type& val), _set_##name##_first, (m_pcm, m_params, &val))\
  ALSA_HPP_TEMPLATE(& name##_last(type& val), _set_##name##_last, (m_pcm, m_params, &val))

#define ALSA_HPP_RANGEVARDIR(name, type) \
  ALSA_HPP_TEMPLATE(& get_##name(type& val), _get_##name, (m_params, &val, NULL))\
  ALSA_HPP_TEMPLATE(const& get_##name(type& val) const, _get_##name, (m_params, &val, NULL))\
  ALSA_HPP_TEMPLATE(& get_##name(type& val, int& dir), _get_##name, (m_params, &val, &dir))\
  ALSA_HPP_TEMPLATE(const& get_##name(type& val, int& dir) const, _get_##name, (m_params, &val, &dir))\
  ALSA_HPP_TEMPLATE(& get_##name##_min(type& min), _get_##name##_min, (m_params, &min, NULL))\
  ALSA_HPP_TEMPLATE(const& get_##name##_min(type& min) const, _get_##name##_min, (m_params, &min, NULL))\
  ALSA_HPP_TEMPLATE(& get_##name##_min(type& min, int& dir), _get_##name##_min, (m_params, &min, &dir))\
  ALSA_HPP_TEMPLATE(const& get_##name##_min(type& min, int& dir) const, _get_##name##_min, (m_params, &min, &dir))\
  ALSA_HPP_TEMPLATE(& get_##name##_max(type& max), _get_##name##_max, (m_params, &max, NULL))\
  ALSA_HPP_TEMPLATE(const& get_##name##_max(type& max) const, _get_##name##_max, (m_params, &max, NULL))\
  ALSA_HPP_TEMPLATE(& get_##name##_max(type& max, int& dir), _get_##name##_max, (m_params, &max, &dir))\
  ALSA_HPP_TEMPLATE(const& get_##name##_max(type& max, int& dir) const, _get_##name##_max, (m_params, &max, &dir))\
  ALSA_HPP_TEMPLATE(& name(type val, int dir = 0), _set_##name, (m_pcm, m_params, val, dir))\
  ALSA_HPP_TEMPLATE(& name##_min(type& min), _set_##name##_min, (m_pcm, m_params, &min, NULL))\
  ALSA_HPP_TEMPLATE(& name##_min(type& min, int& dir), _set_##name##_min, (m_pcm, m_params, &min, &dir))\
  ALSA_HPP_TEMPLATE(& name##_max(type& max), _set_##name##_max, (m_pcm, m_params, &max, NULL))\
  ALSA_HPP_TEMPLATE(& name##_max(type& max, int& dir), _set_##name##_max, (m_pcm, m_params, &max, &dir))\
  ALSA_HPP_TEMPLATE(& name##_minmax(type& min, type& max), _set_##name##_minmax, (m_pcm, m_params, &min, NULL, &max, NULL))\
  ALSA_HPP_TEMPLATE(& name##_minmax(type& min, int& mindir, type& max, int& maxdir), _set_##name##_minmax, (m_pcm, m_params, &min, &mindir, &max, &maxdir))\
  ALSA_HPP_TEMPLATE(& name##_near(type& val), _set_##name##_near, (m_pcm, m_params, &val, NULL))\
  ALSA_HPP_TEMPLATE(& name##_near(type& val, int& dir), _set_##name##_near, (m_pcm, m_params, &val, &dir))\
  ALSA_HPP_TEMPLATE(& name##_first(type& val), _set_##name##_first, (m_pcm, m_params, &val, NULL))\
  ALSA_HPP_TEMPLATE(& name##_first(type& val, int& dir), _set_##name##_first, (m_pcm, m_params, &val, &dir))\
  ALSA_HPP_TEMPLATE(& name##_last(type& val), _set_##name##_last, (m_pcm, m_params, &val, NULL))\
  ALSA_HPP_TEMPLATE(& name##_last(type& val, int& dir), _set_##name##_last, (m_pcm, m_params, &val, &dir))

	/** @short A helper object for modifying hw_params of a PCM. **/
	class hw_config { ALSA_HPP_NONCOPYABLE(hw_config)
		snd_pcm_t* m_pcm;
		hw_params m_params;
	  public:
		/**
		* Construct a new config object, initialized with the current settings
		* of the PCM or with the "any" configuration space, if there are none.
		**/
		hw_config(snd_pcm_t* pcm): m_pcm(pcm), m_params() {
			try { current(); } catch (std::runtime_error&) { any(); }
		}
		/** Construct a new config object, initialized with a copy from given parameters **/
		hw_config(snd_pcm_t* pcm, snd_pcm_hw_params_t const* params): m_pcm(pcm), m_params(params) {}
		/** Load settings from hw_params **/
		hw_config& load(hw_params const& params) { m_params = params; return *this; }
		operator hw_params&() { return m_params; }
		operator hw_params const&() const { return m_params; }
#define ALSA_HPP_CLASS hw_config
#define ALSA_HPP_TEMPLATE(proto, suffix, params) ALSA_HPP_CLASS proto { ALSA_CHECKED(snd_pcm_hw_params##suffix, params); return *this; }
		// Load / store functions
		ALSA_HPP_FUNC(commit,)
		ALSA_HPP_FUNC(any, _any)
		ALSA_HPP_FUNC(current, _current)
		// Enum functions
		ALSA_HPP_ENUMVAR(access)
		ALSA_HPP_ENUMVAR(format)
		ALSA_HPP_ENUMVAR(subformat)
		// Bool functions
		ALSA_HPP_BOOLVAR(rate_resample)
		ALSA_HPP_BOOLVAR(export_buffer)
		// Range functions
		ALSA_HPP_RANGEVAR(channels, unsigned int)
		ALSA_HPP_RANGEVAR(buffer_size, snd_pcm_uframes_t)
		// Range functions with direction argument
		ALSA_HPP_RANGEVARDIR(rate, unsigned int)
		ALSA_HPP_RANGEVARDIR(period_time, unsigned int)
		ALSA_HPP_RANGEVARDIR(period_size, snd_pcm_uframes_t)
		ALSA_HPP_RANGEVARDIR(periods, unsigned int)
		ALSA_HPP_RANGEVARDIR(buffer_time, unsigned int)
#undef ALSA_HPP_TEMPLATE
#undef ALSA_HPP_CLASS
	};

	class sw_config { ALSA_HPP_NONCOPYABLE(sw_config)
		snd_pcm_t* m_pcm;
		sw_params m_params;
	  public:
		sw_config(snd_pcm_t* pcm): m_pcm(pcm), m_params() { current(); }
		/** Construct a new config object, initialized with a copy from given parameters **/
		sw_config(snd_pcm_t* pcm, snd_pcm_sw_params_t const* params): m_pcm(pcm), m_params(params) {}
		/** Load settings from hw_params **/
		sw_config& load(snd_pcm_sw_params_t const* params) { m_params = params; return *this; }
		operator sw_params&() { return m_params; }
		operator sw_params const&() const { return m_params; }
#define ALSA_HPP_CLASS sw_config
#define ALSA_HPP_TEMPLATE(proto, suffix, params) ALSA_HPP_CLASS proto { ALSA_CHECKED(snd_pcm_sw_params##suffix, params); return *this; }
		// Load / store functions
		ALSA_HPP_FUNC(commit,)
		ALSA_HPP_FUNC(current, _current)
		// Enum functions
		typedef snd_pcm_tstamp_t snd_pcm_tstamp_mode_t; // Workaround for inconsistent naming in asound
		ALSA_HPP_ENUMVARMINIMAL(tstamp_mode)
		// Simple variable functions
		ALSA_HPP_VAR(avail_min, snd_pcm_uframes_t)
		ALSA_HPP_VAR(start_threshold, snd_pcm_uframes_t)
		ALSA_HPP_VAR(stop_threshold, snd_pcm_uframes_t)
		ALSA_HPP_VAR(silence_threshold, snd_pcm_uframes_t)
		ALSA_HPP_VAR(silence_size, snd_pcm_uframes_t)
		ALSA_HPP_VAR(tstamp_mode, snd_pcm_tstamp_t)
		// Get-only variable
		ALSA_HPP_VARGET(boundary, snd_pcm_uframes_t)
#undef ALSA_HPP_TEMPLATE
#undef ALSA_HPP_CLASS
	};

#undef ALSA_HPP_FUNC
#undef ALSA_HPP_VAR
#undef ALSA_HPP_VARGET
#undef ALSA_HPP_ENUMVAR
#undef ALSA_HPP_ENUMVARMINIMAL
#undef ALSA_HPP_BOOLVAR
#undef ALSA_HPP_RANGEVAR
#undef ALSA_HPP_RANGEVARDIR

	/** @short A RAII wrapper for snd_pcm_mmap_begin/end block. **/
	class mmap { ALSA_HPP_NONCOPYABLE(mmap)
		snd_pcm_t* m_pcm;
		snd_pcm_channel_area_t const* m_areas;
		snd_pcm_uframes_t m_offset;
		snd_pcm_uframes_t m_frames;
		void test() const {
			if (!m_pcm) throw std::logic_error("alsa::mmap accessed after commit");
		}
	  public:
		/** Access a snd_pcm_channel_area_t. &area(0) to access the array. **/
		snd_pcm_channel_area_t const& area(std::size_t num) { test(); return m_areas[num]; }
		/** Return the offset where the data is within the areas **/
		snd_pcm_uframes_t offset() const { test(); return m_offset; }
		/** Return the maximum number of frames that may be used **/
		snd_pcm_uframes_t frames() const { test(); return m_frames; }
		/**
		* Initiate MMAP transfer.
		* snd_pcm_avail_update must be called directly before constructing the
		* alsa::mmap object, otherwise snd_pcm_mmap_begin may return a wrong
		* count of available frames.
		* @param pcm PCM handle
		* @param req number of frames to request (check .frames() for actual count)
		**/
		mmap(snd_pcm_t* pcm, snd_pcm_uframes_t req): m_pcm(pcm), m_areas(), m_offset(), m_frames(req) {
			ALSA_CHECKED(snd_pcm_mmap_begin, (m_pcm, &m_areas, &m_offset, &m_frames));
		}
		/** End mmap transfer; no data will be commited. **/
		~mmap() {
			if (m_pcm) snd_pcm_mmap_commit(m_pcm, m_offset, 0);
		}
		/** Commit all frames **/
		void commit() {
			commit(frames());
		}
		/** Commit n frames **/
		void commit(snd_pcm_uframes_t n) {
			if (n > frames()) throw std::out_of_range("alsa::mmap::commit(n) with n > frames()");
			ALSA_CHECKED(snd_pcm_mmap_commit, (m_pcm, offset(), n));
			m_pcm = NULL;
			m_areas = NULL;
		}
	};

#undef ALSA_HPP_NONCOPYABLE

}

#endif

