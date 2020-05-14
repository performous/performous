#pragma once
#ifdef USE_WEBSERVER

#include <cpprest/http_listener.h>
#include <cpprest/filestream.h>
#include <nlohmann/json.hpp>
#include <boost/asio.hpp>
#include <restinio/all.hpp>

#include "screen_playlist.hh"
#include "restinio_performous_logger_adapter.hh"

#ifdef USE_BOOST_REGEX
#include <restinio/router/boost_regex_engine.hpp>
using Performous_Router_t = restinio::router::express_router_t<restinio::router::boost_regex_engine_t>;
#else
#include <restinio/router/std_regex_engine.hpp>
using Performous_Router_t = restinio::router::express_router_t<restinio::router::std_regex_engine_t>;
#endif

using Performous_Server_Traits = restinio::traits_t<
         restinio::asio_timer_manager_t,
         restinio::shared_performous_logger_t,
         Performous_Router_t,
         boost::asio::strand<boost::asio::executor>>;
       
using Performous_Server_Settings = restinio::run_on_thread_pool_settings_t<Performous_Server_Traits>;

class RequestHandler
{
    public:
    	friend class WebServer;
        RequestHandler(Songs& songs);
        RequestHandler(std::string url, unsigned short port, Songs& songs);
        virtual ~RequestHandler();

        pplx::task<void>open() { return m_listener.open(); }
        pplx::task<void>close() { return m_listener.close(); }

		template < typename RESP >
static	RESP
		init_resp( RESP resp )
		{
			resp.append_header( "Server", "Performous sample server using RESTinio/v.0.6.7" );
			resp.append_header_date_field()
			.append_header( "Content-Type", "text/plain; charset=utf-8" );

			return resp;
		}
		
        restinio::ip_blocker::inspection_result_t inspect(const restinio::ip_blocker::incoming_info_t & info) noexcept {
            if(some_condition(info)) {
                return restinio::ip_blocker::deny();
                }
            else {
                return restinio::ip_blocker::allow();
                }
        }


    protected:

    private:
        
        void Get(web::http::http_request request);
        void Put(web::http::http_request request);
        void Post(web::http::http_request request);
        void Delete(web::http::http_request request);
        void Error(pplx::task<void>& t);
        
        static std::unique_ptr<Performous_Router_t> init_webserver_router();

        web::json::value ExtractJsonFromRequest(web::http::http_request request);
        nlohmann::json ExtractJsonFromRequest_New(web::http::http_request request);
        
        nlohmann::json convertFromCppRest(web::json::value const& jsonDoc);
        web::json::value convertToCppRest(nlohmann::json const& jsonDoc);

        void HandleFile(web::http::http_request request, std::string filePath = "");
        nlohmann::json SongsToJsonObject_New();
        web::json::value SongsToJsonObject();
        std::map<std::string, std::string> GenerateLocaleDict();
        std::vector<std::string> GetTranslationKeys();
        std::shared_ptr<Song> GetSongFromJSON_New(nlohmann::json);
        std::shared_ptr<Song> GetSongFromJSON(web::json::value);
		static Performous_Server_Settings make_server_settings(const std::string &url, unsigned short port);
        web::http::experimental::listener::http_listener m_listener;
        Songs& m_songs;
        restinio::http_server_t<Performous_Server_Traits> m_restinio_server;
};
#else
class Songs;

class RequestHandler
{
public:
    RequestHandler(Songs&) {}
};
#endif