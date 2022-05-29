#pragma once
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
