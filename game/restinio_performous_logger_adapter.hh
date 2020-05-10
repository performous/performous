/*
	restinio
*/

/*!
	Ready to use logger implementation for using with std::ostream.
*/

#pragma once

#include "log.hh"
#include <string>
#include <iostream>
#include <chrono>
#include <mutex>

#include <restinio/impl/include_fmtlib.hpp>
#include <restinio/ostream_logger.hpp>

namespace restinio
{
//
// performous_logger_t
//

//! Logger for std::ostream.
/*!
	\note It is not efficient.
*/
template < typename Lock >
class performous_logger_t
{
	public:
		performous_logger_t( const performous_logger_t & ) = delete;
		performous_logger_t & operator = ( const performous_logger_t & ) = delete;

		performous_logger_t() noexcept
			:	m_out{ &std::clog }
		{}

		performous_logger_t( std::ostream & out ) noexcept
			:	m_out{ &out }
		{}

		template< typename Message_Builder >
		void
		trace( Message_Builder && msg_builder )
		{
			log_message( "webserver/debug", msg_builder() );
		}

		template< typename Message_Builder >
		void
		info( Message_Builder && msg_builder )
		{
			log_message( "webserver/info", msg_builder() );
		}

		template< typename Message_Builder >
		void
		warn( Message_Builder && msg_builder )
		{
			log_message( "webserver/warning", msg_builder() );
		}

		template< typename Message_Builder >
		void
		error( Message_Builder && msg_builder )
		{
			log_message( "webserver/error", msg_builder() );
		}

	private:
		void
		log_message( const char * tag, const std::string & msg )
		{
			std::unique_lock< Lock > lock{ m_lock };

			auto now = std::chrono::system_clock::now();
			auto ms = std::chrono::duration_cast<
					std::chrono::milliseconds >( now.time_since_epoch() );
			std::time_t unix_time = std::chrono::duration_cast<
					std::chrono::seconds >( ms ).count();
// 			std::clog << "logger/debug: msg is " << msg << std::endl;
			( *m_out )
				<<		fmt::format(
						"{}: [{:%Y-%m-%d %H:%M:%S}.{:03d}] : {}",
						tag,
						make_localtime( unix_time ),
						static_cast< int >( ms.count() % 1000u ),
						msg )
				<< std::endl;
		}

		Lock m_lock;
		std::ostream * m_out;
};

using single_threaded_performous_logger_t = performous_logger_t< null_lock_t >;
using shared_performous_logger_t = performous_logger_t< std::mutex >;

} /* namespace restinio */
