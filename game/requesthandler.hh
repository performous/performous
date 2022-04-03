#pragma once
#ifdef USE_WEBSERVER
#define _TURN_OFF_PLATFORM_STRING

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
        std::vector<std::string> GetTranslationKeys();
        std::shared_ptr<Song> GetSongFromJSON(web::json::value);

        web::http::experimental::listener::http_listener m_listener;

        Songs& m_songs;
};
#undef _TURN_OFF_PLATFORM_STRING
#else
class Songs;

class RequestHandler
{
public:
    RequestHandler(Songs&) {}
};
#endif