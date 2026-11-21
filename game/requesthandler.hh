#pragma once

#include <string>

#ifdef USE_WEBSERVER

#include <nlohmann/json.hpp>

#include <httplib.h>

#include <memory>
#include <string>

class Game;
class Songs;
class Song;
class ScreenPlaylist;

class RequestHandler {
public:
	RequestHandler(Game& game, const std::string& host, int port, Songs& songs);
	~RequestHandler() = default;

	// Blocking listen; returns when the server is stopped.
	void listen();
	// Stop the listener (safe to call from another thread).
	void stop();
	// True while the listener thread is serving requests.
	bool is_running() const;

private:
	void setupRoutes();

	// API handlers, return httplib status code; body set via res.
	void handleGetDatabase(const httplib::Request& req, httplib::Response& res);
	void handleLanguage(const httplib::Request& req, httplib::Response& res);
	void handleGetCurrentPlaylist(const httplib::Request& req, httplib::Response& res);
	void handlePlaylistTimeout(const httplib::Request& req, httplib::Response& res);

	void handleAdd(const httplib::Request& req, httplib::Response& res);
	void handleRemove(const httplib::Request& req, httplib::Response& res);
	void handleSetPosition(const httplib::Request& req, httplib::Response& res);
	void handleSearch(const httplib::Request& req, httplib::Response& res);

	static nlohmann::json extractJson(const httplib::Request& req);
	nlohmann::json songsToJson() const;
	std::shared_ptr<Song> getSongFromJson(const nlohmann::json& doc) const;
	static std::map<std::string, std::string> generateLocaleDict();
	static std::vector<std::string> getTranslationKeys();

	std::unique_ptr<httplib::Server> m_server;
	std::string m_bindHost;
	int m_bindPort = 0;
	Game& m_game;
	Songs& m_songs;
};

#else
class Songs;
class Game;

class RequestHandler {
public:
	RequestHandler(Game&, const std::string&, int, Songs&) {}
};
#endif
