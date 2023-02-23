#pragma once

class Game;
class Songs;

#include "configuration.hh"
#include "fs.hh"
#include "restinio_performous_logger_adapter.hh"
#include "util.hh"

#include <boost/asio.hpp>
#include <boost/asio/ip/network_v4.hpp>
#include <nlohmann/json.hpp>
#include <restinio/cast_to.hpp>
#include <restinio/helpers/http_field_parsers/cache-control.hpp>
#include <restinio/router/express.hpp>
#include <restinio/http_server_run.hpp>
#include <restinio/uri_helpers.hpp>
#include <restinio/version.hpp>

#include <map>
#include <memory>
#include <vector>

using Performous_Router_t = restinio::router::express_router_t<restinio::router::std_regex_engine_t>;

/**
* Class in charge of accepting remote connections. In practice, it's only used if a custom Subnet mask is set for the server.
**/
class Performous_IP_Blocker {
	public:
		/// Determines whether to allow a connection.
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
		/// Calculates all possible hosts in a given subnet.
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
		static boost::asio::ip::address_v4_range hosts() { return m_allowed_subnet.hosts(); }; ///< Just for convenience and shorter ifs.
		static boost::asio::ip::network_v4 m_allowed_subnet; ///< Network describing the subnet we'll allow connections from.
};

/// Basic RESTinio configuration.
struct Performous_Server_Traits : public restinio::default_traits_t {
	using timer_manager_t = restinio::asio_timer_manager_t;
	using logger_t = restinio::shared_performous_logger_t;
	using request_handler_t = Performous_Router_t;
	using strand_t = boost::asio::strand<restinio::default_asio_executor>;
	using ip_blocker_t = Performous_IP_Blocker;
};

using Performous_Server = restinio::http_server_t<Performous_Server_Traits>;
using Performous_Server_Settings = restinio::run_on_thread_pool_settings_t<Performous_Server_Traits>;

/**
* Wrapper class for the RESTinio webserver and everything it needs to function properly.
**/
class RequestHandler
{
	public:
		friend class WebServer;
		RequestHandler(std::string url, unsigned short port, std::unique_ptr<boost::asio::io_context>& io_context, Songs& songs, Game& game);
		~RequestHandler() = default;
		boost::asio::ip::address_v4& getLocalIP() { return m_local_ip; }; ///< Query local IP.

		template < typename RESP >
		RESP init_resp(RESP resp, std::string const& mime = "text/plain; charset=utf-8") {
			resp.append_header("Server", "Performous Webserver using RESTinio/v"+std::to_string(RESTINIO_VERSION));
			resp.append_header_date_field()
			.append_header(restinio::http_field::cache_control, "no-cache, max-age=20, stale-while-revalidate=60")
			.append_header("Content-Type", mime);
			return resp;
		} ///< Helper with HTTP header boilerplate.
		std::string getContentType(std::string const& extension); ///< Returns mimetype for a given extension.
		restinio::request_handling_status_t HandleFile(std::shared_ptr<restinio::request_t> request, fs::path const& filePath); ///< Send file to client.
	private:
		boost::asio::ip::address_v4 getLocalIP(std::string const& service); ///< Queries 1.1.1.1 (CloudFlare DNS) to determine our network interface and thus address.
		Performous_Server_Settings make_server_settings(std::string const& url, unsigned short port); ///< Sets the RESTinio server up prior to stating it.
		nlohmann::json m_contentTypes = nlohmann::json::object();  ///< JSON container for file-extension/mime-type pairings; in practice it behaves the same as a std::map.
		Songs& m_songs; ///< Reference to Songs.
		Game& m_game; ///< Reference to Game.
		auto initWebserverRouter(); ///< Initialize router.
		std::shared_ptr<Performous_IP_Blocker> m_blocker = nullptr; ///< Allow connections only from specific subnet.
		std::unique_ptr<Performous_Server_Settings> m_serverSettings = nullptr;
		std::unique_ptr<Performous_Server> m_restinio_server = nullptr; ///< RESTinio server instance.
		boost::asio::ip::address_v4 m_local_ip; ///< Local IP Address.

};
