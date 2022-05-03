#pragma once
#ifdef USE_WEBSERVER

#include <cpprest/http_listener.h>
#include <cpprest/filestream.h>

#include "screen_playlist.hh"

class RequestHandler
{
    public:
        RequestHandler(Songs& songs);
        RequestHandler(std::string url, Songs& songs);
        virtual ~RequestHandler();

        pplx::task<void>open() { return m_listener.open(); }
        pplx::task<void>close() { return m_listener.close(); }

    protected:

    private:
        void Get(web::http::http_request request);
        void Put(web::http::http_request request);
        void Post(web::http::http_request request);
        void Delete(web::http::http_request request);
        void Error(pplx::task<void>& t);

        web::json::value ExtractJsonFromRequest(web::http::http_request request);

        void HandleFile(web::http::http_request request, std::string filePath = "");
        web::json::value SongsToJsonObject();
        std::map<std::string, std::string> GenerateLocaleDict();
        const std::vector<std::string> &GetTranslationKeys() const;
        std::shared_ptr<Song> GetSongFromJSON(web::json::value);

        web::http::experimental::listener::http_listener m_listener;

        Songs& m_songs;
};
#else
#include <string>

#include <httplib.h>

#include "songs.hh"

class RequestHandler {
public:
    RequestHandler(std::string, Songs &songs);

    void open(const std::string &addr, int port) { m_server.listen(addr.c_str(), port); }
    void close() { m_server.stop(); }

private:
    const std::vector<std::string>& GetTranslationKeys() const;

private:
    httplib::Server m_server;
    Songs& m_songs;
};
#endif
