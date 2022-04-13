#pragma once
#ifdef USE_WEBSERVER
#include "util.hh"

#include <boost/asio.hpp>
#include <boost/asio/ip/network_v4.hpp>
#include <chrono>
#include <nlohmann/json.hpp>
#include <restinio/all.hpp>
#include <restinio/helpers/http_field_parsers/cache-control.hpp>

#ifdef USE_BOOST_REGEX
#include <restinio/router/boost_regex_engine.hpp>
using Performous_Router_t = restinio::router::express_router_t<restinio::router::boost_regex_engine_t>;
#else
#include <restinio/router/std_regex_engine.hpp>
using Performous_Router_t = restinio::router::express_router_t<restinio::router::std_regex_engine_t>;
#endif

#include "screen_playlist.hh"
#include "restinio_performous_logger_adapter.hh"
class Performous_IP_Blocker {
	public:
		restinio::ip_blocker::inspection_result_t inspect(restinio::ip_blocker::incoming_info_t const& info) noexcept {
			boost::asio::ip::address_v4 remote_ip = info.remote_endpoint().address().to_v4();
			if (config["webserver/access"].ui() != 3) {
				return restinio::ip_blocker::allow(); // IP-blocking is turned off.
			}
			if (Performous_IP_Blocker::hosts().find(remote_ip) != hosts().end() || remote_ip.is_loopback()) {
					return restinio::ip_blocker::allow();
				}
			return restinio::ip_blocker::deny();
		}
		static void setAllowedSubnet(boost::asio::ip::address_v4 const& ipAddress) {
			if (config["webserver/access"].ui() == 3) {
				unsigned slashNotation = stou(config["webserver/netmask"].getEnumName());
				std::uint64_t subNetValue = 1;
				subNetValue <<= 32;
				subNetValue -= (1 << (32 - slashNotation));
				boost::asio::ip::address_v4 subnetAddress = boost::asio::ip::make_address_v4(static_cast<unsigned>(subNetValue));
				m_allowed_subnet = boost::asio::ip::make_network_v4(ipAddress, subnetAddress);
			}
		}
	private:
		static boost::asio::ip::address_v4_range hosts() { return m_allowed_subnet.hosts(); };
		static boost::asio::ip::network_v4 m_allowed_subnet;
};

struct Performous_Server_Traits : public restinio::default_traits_t {
	using timer_manager_t = restinio::asio_timer_manager_t;
	using logger_t = restinio::shared_performous_logger_t;
	using request_handler_t = Performous_Router_t;
	using strand_t = boost::asio::strand<boost::asio::executor>;
	using ip_blocker_t = Performous_IP_Blocker;
};
	 
using Performous_Server_Settings = restinio::run_on_thread_pool_settings_t<Performous_Server_Traits>;

class RequestHandler
{
    public:
    	friend class WebServer;
        RequestHandler(Game &game, Songs& songs);
        RequestHandler(Game &game, std::string url, unsigned short port, Songs& songs);
        ~RequestHandler();
        const boost::asio::ip::address_v4& getLocalIP() const { return m_local_ip; };
        pplx::task<void>open() { return m_listener.open(); }
        pplx::task<void>close() { return m_listener.close(); }

		template < typename RESP >
		static	RESP
		init_resp(RESP resp, const std::string& mime = "text/plain; charset=utf-8") {
			resp.append_header("Server", "Performous Webserver using RESTinio/v"+std::to_string(RESTINIO_VERSION));
			resp.append_header_date_field()
			.append_header(restinio::http_field::cache_control, "no-cache, max-age=20, stale-while-revalidate=60")
			.append_header("Content-Type", mime);
			return resp;
		}

    protected:
		std::string getContentType(const std::string& extension);

    private:
        void Get(web::http::http_request request);
        void Put(web::http::http_request request);
        void Post(web::http::http_request request);
        void Delete(web::http::http_request request);
        void Error(pplx::task<void>& t);
        
        std::unique_ptr<Performous_Router_t> init_webserver_router();
        static std::string m_ip_address;
        static boost::asio::ip::address_v4 getLocalIP(const std::string& service);

        web::json::value ExtractJsonFromRequest(web::http::http_request request);
        nlohmann::json ExtractJsonFromRequest_New(web::http::http_request request);
        
        nlohmann::json convertFromCppRest(web::json::value const& jsonDoc);
        web::json::value convertToCppRest(nlohmann::json const& jsonDoc);

        void HandleFile(web::http::http_request request, std::string filePath = "");
        web::json::value SongsToJsonObject();
        restinio::request_handling_status_t HandleFile(std::shared_ptr<restinio::request_t> request, const fs::path& filePath);
        nlohmann::json SongsToJsonObject();
        std::map<std::string, std::string> GenerateLocaleDict();
        std::vector<std::string> GetTranslationKeys();
        std::shared_ptr<Song> GetSongFromJSON(nlohmann::json);
	Performous_Server_Settings make_server_settings(const std::string &url, unsigned short port);
        web::http::experimental::listener::http_listener m_listener;
        Game& m_game;
        Songs& m_songs;
        restinio::http_server_t<Performous_Server_Traits> m_restinio_server;
        boost::asio::ip::address_v4 m_local_ip;
};
#else
class Songs;
class Game;

class RequestHandler
{
public:
    RequestHandler(Game&, Songs&) {}
};
#endif
