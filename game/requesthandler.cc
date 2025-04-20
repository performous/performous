#include "requesthandler.hh"
#include "unicode.hh"
#include "game.hh"
#include "screen_sing.hh"
#include "screen_playlist.hh"

#include <cstdint>

#ifdef USE_WEBSERVER
RequestHandler::RequestHandler(Game& game, Songs& songs)
	: m_game(game), m_songs(songs) {
}

RequestHandler::RequestHandler(Game& game, std::string url, Songs& songs)
	: m_listener(utility::conversions::to_string_t(url)), m_game(game), m_songs(songs) {
	m_listener.support(web::http::methods::GET, std::bind(&RequestHandler::Get, this, std::placeholders::_1));
	m_listener.support(web::http::methods::PUT, std::bind(&RequestHandler::Put, this, std::placeholders::_1));
	m_listener.support(web::http::methods::POST, std::bind(&RequestHandler::Post, this, std::placeholders::_1));
	m_listener.support(web::http::methods::DEL, std::bind(&RequestHandler::Delete, this, std::placeholders::_1));

}

void RequestHandler::Error(pplx::task<void>& t) {
	try {
		t.get();
	}
	catch (...) {
	}
}

void RequestHandler::HandleFile(web::http::http_request request, std::string filePath) {
	auto path = filePath != "" ? utility::conversions::to_utf8string(utility::conversions::to_string_t(filePath)) : utility::conversions::to_utf8string(request.relative_uri().path());
	path = utility::conversions::to_utf8string(web::uri::decode(utility::conversions::to_string_t(path)));
	auto fileName = path.substr(path.find_last_of("/\\") + 1);

	std::string fileToSend;
	std::string clientIp = utility::conversions::to_utf8string(request.remote_address());
	auto path = filePath != "" ? utility::conversions::to_utf8string(utility::conversions::to_string_t(filePath)) : utility::conversions::to_utf8string(request.relative_uri().path());
	auto const fileName = fs::path(path).filename().string();

	try {
		fileToSend = findFile(fileName).string();
	}
	catch (...) {
		try {
			// We need to ensure that files will only be picked from trusted locations.
			std::string folderCheck = path.substr(path.find_first_not_of("/\\"));
			std::vector<std::string> subFolderChecks = { "images", "fonts", "css", "js" };

			if (folderCheck.find_first_of("/\\") == std::string::npos) {
				request.reply(web::http::status_codes::NotFound, utility::conversions::to_string_t("NOT FOUND "));
				return;
			}

			bool isValid = false;

			for (std::string check : subFolderChecks) {
				if (folderCheck.rfind(check, 0)) {
					isValid = true;
					break;
				}
			}

			if (!isValid ) {
				request.reply(web::http::status_codes::NotFound, utility::conversions::to_string_t("NOT FOUND "));
				return;
			}
			fileName = folderCheck.substr(folderCheck.find_first_of("/\\") + 1);
			fileToSend = findFile(fileName).string();
		}
		catch (...) {
			SpdLogger::error(LogSystem::WEBSERVER, std::string("HandleFile() File Not Found. Client {}. {}"), clientIp, e.what());
			auto const errorMsg = std::string("INTERNAL ERROR, MISSING FILE: ") + e.what();
			request.reply(web::http::status_codes::NotFound, utility::conversions::to_string_t(errorMsg));
			return;
		}
	}

	concurrency::streams::fstream::open_istream(utility::conversions::to_string_t(fileToSend), std::ios::in).then([=](concurrency::streams::istream is) {
		std::string content_type = "";
		if (path.find(".html") != std::string::npos) {
			content_type = "text/html";
		}
		else if (path.find(".js") != std::string::npos) {
			content_type = "text/javascript";
		}
		else if (path.find(".css") != std::string::npos) {
			content_type = "text/css";
		}
		else
		{
			// This will identify most common image types or use "application/octet-stream"
			content_type = getImageMimeType(path);
		}

		request.reply(web::http::status_codes::OK, is, utility::conversions::to_string_t(content_type)).then([](pplx::task<void> t) {
			try {
				t.get();
			}
			catch (...) {
				//
			}
			});

		}).then([=](pplx::task<void>t) {
			try {
				t.get();
			}
			catch (const web::http::http_exception &e) {
				SpdLogger::error(LogSystem::WEBSERVER, std::string("HandleFile() HTTP Exception. Client {}. {}"), clientIp, e.what());
				auto const errorMsg = std::string("HTTP ERROR: ") + e.what();
				request.reply(web::http::status_codes::InternalError, utility::conversions::to_string_t(errorMsg));
			}
			catch (const std::exception &e)
			{
				SpdLogger::error(LogSystem::WEBSERVER, std::string("HandleFile() Std. Exception. Client {}. {}"), clientIp, e.what());
				auto const errorMsg = std::string("INTERNAL ERROR: ") + e.what();
				request.reply(web::http::status_codes::InternalError, utility::conversions::to_string_t(errorMsg));
			}
			catch (...) {
				SpdLogger::error(LogSystem::WEBSERVER, "HandleFile() Unknown Exception. Client {}.", clientIp);
				request.reply(web::http::status_codes::InternalError, utility::conversions::to_string_t("INTERNAL ERROR: Unknown Exception"));
			}
		});
}

void RequestHandler::Get(web::http::http_request request)
{
	std::string content_type = "text/html";
	auto uri = request.relative_uri().path();
	auto query = utility::conversions::to_utf8string(request.relative_uri().query());
	if (query != "") {
		uri += utility::conversions::to_string_t("?") + request.relative_uri().query();
	}
	SpdLogger::debug(LogSystem::WEBSERVER, "RequestHandler GET request, path={}", utility::conversions::to_utf8string(uri));
	auto path = utility::conversions::to_utf8string(request.relative_uri().path());
	if (path == "/") {
		HandleFile(request, findFile("index.html").string());
	}
	else if (path == "/api/getDataBase.json") { //get database
		m_songs.setFilter("");
		// parse query to key-value pairs
		auto queryParams = web::uri::split_query(web::uri::decode(utility::conversions::to_string_t(query)));

		// we want to fall back to the current sort number
		unsigned short sort = m_songs.sortNum();
		bool descending = queryParams.count(utility::conversions::to_string_t("order")) && utility::conversions::to_utf8string(queryParams.at(utility::conversions::to_string_t("order"))) == "descending";
		size_t offset = 0;
		size_t limit = 0;

		std::map<std::string, unsigned short> sortTypes = {
			{"", 2},
			{"random", 0},
			{"title", 1},
			{"artist", 2},
			{"edition", 3},
			{"genre", 4},
			{"folder", 5},
			{"language", 6},
			{"creator", 10}
		};

		if (queryParams.count(utility::conversions::to_string_t("sort")) && sortTypes.count(utility::conversions::to_utf8string(queryParams.at(utility::conversions::to_string_t("sort"))))) {
			sort = sortTypes.at(utility::conversions::to_utf8string(queryParams.at(utility::conversions::to_string_t("sort"))));
		}

		m_songs.sortSpecificChange(sort, descending);

		if (queryParams.count(utility::conversions::to_string_t("query"))) {
			m_songs.setFilter(utility::conversions::to_utf8string(queryParams.at(utility::conversions::to_string_t("query"))));
		}

		if (queryParams.count(utility::conversions::to_string_t("offset"))) {
			sscanf(utility::conversions::to_utf8string(queryParams.at(utility::conversions::to_string_t("offset"))).c_str(), "%zu", &offset);
		}

		if (queryParams.count(utility::conversions::to_string_t("limit"))) {
			sscanf(utility::conversions::to_utf8string(queryParams.at(utility::conversions::to_string_t("limit"))).c_str(), "%zu", &limit);
		}

		// make sure to apply the filtering
		m_songs.update();
		web::json::value jsonRoot = SongsToJsonObject(offset, limit);
		request.reply(web::http::status_codes::OK, jsonRoot);
		return;
	}
	else if (path == "/api/language") {
		auto localeMap = GenerateLocaleDict();
		web::json::value jsonRoot = web::json::value::object();
		for (auto const& kv : localeMap) {
			std::string key = kv.first;
			//Hack to get an easy key value pair within the json object.
			if (key == "Web interface by Niek Nooijens and Arjan Speiard, for full credits regarding Performous see /docs/Authors.txt") {
				key = "Credits";
			}
			std::replace(key.begin(), key.end(), ' ', '_');
			key = UnicodeUtil::toLower(key);
			jsonRoot[utility::conversions::to_string_t(key)] = web::json::value(utility::conversions::to_string_t(kv.second));
		}
		request.reply(web::http::status_codes::OK, jsonRoot);
		return;
	}
	else if (path == "/api/getCurrentPlaylist.json") {
		web::json::value jsonRoot = web::json::value::array();
		unsigned i = 0;
		for (auto const& song : m_game.getCurrentPlayList().getList()) {
			web::json::value songObject = web::json::value::object();
			songObject[utility::conversions::to_string_t("Title")] = web::json::value::string(utility::conversions::to_string_t(song->title));
			songObject[utility::conversions::to_string_t("Artist")] = web::json::value::string(utility::conversions::to_string_t(song->artist));
			songObject[utility::conversions::to_string_t("Edition")] = web::json::value::string(utility::conversions::to_string_t(song->edition));
			songObject[utility::conversions::to_string_t("Language")] = web::json::value::string(utility::conversions::to_string_t(song->language));
			songObject[utility::conversions::to_string_t("Creator")] = web::json::value::string(utility::conversions::to_string_t(song->creator));
			songObject[utility::conversions::to_string_t("Duration")] = web::json::value(song->getDurationSeconds());
			songObject[utility::conversions::to_string_t("HasError")] = web::json::value::boolean(song->loadStatus == Song::LoadStatus::PARSERERROR);
			songObject[utility::conversions::to_string_t("ProvidedBy")] = web::json::value(utility::conversions::to_string_t(song->providedBy));
			songObject[utility::conversions::to_string_t("Comment")] = web::json::value(utility::conversions::to_string_t(song->comment));
			songObject[utility::conversions::to_string_t("Path")] = web::json::value(utility::conversions::to_string_t(std::filesystem::path(song->path.parent_path()).filename()));
			jsonRoot[i] = songObject;
			i++;
		}

		request.reply(web::http::status_codes::OK, jsonRoot);
		return;
	}
	else if (path == "/api/getplaylistTimeout") {
		request.reply(web::http::status_codes::OK, std::to_string(config["game/playlist_screen_timeout"].ui()));
		return;
	}
	else if (path == "/api/getCurrentSong") {
		auto const& screen = m_game.getCurrentScreen();
		
		if (!screen || (screen->getName() != "Sing" && screen->getName() != "Playlist")) {
			request.reply(web::http::status_codes::OK, web::json::value::null());
			return;
		}
		if (screen->getName() == "Playlist") {
			ScreenPlaylist& sp = dynamic_cast<ScreenPlaylist&>(*m_game.getScreen("Playlist"));
			request.reply(web::http::status_codes::OK, web::json::value::number(sp.getTimer()));
			return;
		}

		ScreenSing& ss = dynamic_cast<ScreenSing&>(*m_game.getScreen("Sing"));

		web::json::value songObject = web::json::value::object();
		songObject[utility::conversions::to_string_t("Title")] = web::json::value::string(utility::conversions::to_string_t(ss.getSong()->title));
		songObject[utility::conversions::to_string_t("Artist")] = web::json::value::string(utility::conversions::to_string_t(ss.getSong()->artist));
		songObject[utility::conversions::to_string_t("Edition")] = web::json::value::string(utility::conversions::to_string_t(ss.getSong()->edition));
		songObject[utility::conversions::to_string_t("Language")] = web::json::value::string(utility::conversions::to_string_t(ss.getSong()->language));
		songObject[utility::conversions::to_string_t("Creator")] = web::json::value::string(utility::conversions::to_string_t(ss.getSong()->creator));
		songObject[utility::conversions::to_string_t("Duration")] = web::json::value(ss.getSong()->getDurationSeconds());
		songObject[utility::conversions::to_string_t("HasError")] = web::json::value::boolean(ss.getSong()->loadStatus == Song::LoadStatus::ERROR);
		songObject[utility::conversions::to_string_t("ProvidedBy")] = web::json::value(utility::conversions::to_string_t(ss.getSong()->providedBy));
		songObject[utility::conversions::to_string_t("Comment")] = web::json::value(utility::conversions::to_string_t(ss.getSong()->comment));
		songObject[utility::conversions::to_string_t("Path")] = web::json::value(utility::conversions::to_string_t(std::filesystem::path(ss.getSong()->path.parent_path()).filename()));
		songObject[utility::conversions::to_string_t("Position")] = web::json::value(ss.getSongPosition());

		request.reply(web::http::status_codes::OK, songObject);
		return;
	}
	else {
		HandleFile(request);
	}
}

void RequestHandler::Post(web::http::http_request request)
{
	auto uri = request.relative_uri().path();
	auto query = utility::conversions::to_utf8string(request.relative_uri().query());
	if (query != "") {
		uri += utility::conversions::to_string_t("?") + request.relative_uri().query();
	}
	SpdLogger::debug(LogSystem::WEBSERVER, "RequestHandler POST request, path={}", utility::conversions::to_utf8string(uri));

	auto path = utility::conversions::to_utf8string(request.relative_uri().path());

	web::json::value jsonPostBody = ExtractJsonFromRequest(request);

	if (jsonPostBody == web::json::value::null()) {
		request.reply(web::http::status_codes::BadRequest, "Post body is malformed. Please make a valid request.");
		return;
	}

	if (path == "/api/add" || path == "/api/add/priority" || path =="/api/add/play") {
		m_songs.setFilter("");
		std::shared_ptr<Song> songPointer = GetSongFromJSON(jsonPostBody);
		if (!songPointer) {
			auto artist = utility::conversions::to_utf8string(jsonPostBody[utility::conversions::to_string_t("Artist")].as_string());
			auto title = utility::conversions::to_utf8string(jsonPostBody[utility::conversions::to_string_t("Title")].as_string());
			request.reply(web::http::status_codes::NotFound, "Song \"" + artist + " - " + title + "\" was not found.");
			return;
		}
		else if (songPointer->loadStatus == Song::LoadStatus::PARSERERROR) {
			auto artist = utility::conversions::to_utf8string(jsonPostBody[utility::conversions::to_string_t("Artist")].as_string());
			auto title = utility::conversions::to_utf8string(jsonPostBody[utility::conversions::to_string_t("Title")].as_string());
			request.reply(web::http::status_codes::NotFound, "Song \"" + artist + " - " + title + "\" Song load status is error. Please check what's wrong with it.");
			return;
		}
		else {
			SpdLogger::debug(LogSystem::WEBSERVER, "Adding {} - {} to the playlist.", songPointer->artist, songPointer->title);
			m_game.getCurrentPlayList().addSong(songPointer);
			if (path == "/api/add/priority" || path == "/api/add/play") {
				m_game.getCurrentPlayList().move(m_game.getCurrentPlayList().getList().size() - 1, 0);

				if (path == "/api/add/play") {
					std::shared_ptr<Song> songPointer = m_game.getCurrentPlayList().getNext();
					m_game.activateScreen("Sing");
					ScreenSing* m_pp = dynamic_cast<ScreenSing*>(m_game.getScreen("Sing"));
					m_pp->setSong(songPointer);
				}
			}
			ScreenPlaylist* m_pp = dynamic_cast<ScreenPlaylist*>(m_game.getScreen("Playlist"));
			m_pp->triggerSongListUpdate();

			request.reply(web::http::status_codes::OK, "success");
			return;
		}
	}
	else if (path == "/api/remove") {
		if (m_game.getCurrentPlayList().isEmpty()) {
			request.reply(web::http::status_codes::BadRequest, "Playlist is empty.");
			return;
		}
		try {
			unsigned songIdToDelete = jsonPostBody[utility::conversions::to_string_t("songId")].as_number().to_uint32();
			m_game.getCurrentPlayList().removeSong(songIdToDelete);
			ScreenPlaylist* m_pp = dynamic_cast<ScreenPlaylist*>(m_game.getScreen("Playlist"));
			m_pp->triggerSongListUpdate();

			request.reply(web::http::status_codes::OK, "success");
			return;
		}
		catch (web::json::json_exception const& e) {
			std::string str = std::string("JSON Exception: ") + e.what();
			request.reply(web::http::status_codes::BadRequest, str);
			return;
		}
	}
	else if (path == "/api/setposition") {
		if (m_game.getCurrentPlayList().isEmpty()) {
			request.reply(web::http::status_codes::BadRequest, "Playlist is empty.");
			return;
		}
		try {
			unsigned songIdToMove = jsonPostBody[utility::conversions::to_string_t("songId")].as_number().to_uint32();
			unsigned positionToMoveTo = jsonPostBody[utility::conversions::to_string_t("position")].as_number().to_uint32();
			unsigned sizeOfPlaylist = static_cast<unsigned>(m_game.getCurrentPlayList().getList().size());
			if (songIdToMove > sizeOfPlaylist - 1) {
				request.reply(web::http::status_codes::BadRequest, "Not gonna move the unknown song you've provided \"" + std::to_string(songIdToMove + 1) + "\". Please make a valid request.");
				return;
			}
			if (positionToMoveTo <= sizeOfPlaylist - 1) {
				m_game.getCurrentPlayList().move(songIdToMove, positionToMoveTo);
				ScreenPlaylist* m_pp = dynamic_cast<ScreenPlaylist*>(m_game.getScreen("Playlist"));
				m_pp->triggerSongListUpdate();
				request.reply(web::http::status_codes::OK, "success");
				return;
			}
			else {
				request.reply(web::http::status_codes::BadRequest, "Not gonna move the song to \"" + std::to_string(positionToMoveTo + 1) + "\" since the list ain't that long. Please make a valid request.");
				return;
			}
		}
		catch (web::json::json_exception const& e) {
			std::string str = std::string("JSON Exception: ") + e.what();
			request.reply(web::http::status_codes::BadRequest, str);
			return;
		}
	}
	else if (path == "/api/play") {
		if (m_game.getCurrentPlayList().isEmpty()) {
			request.reply(web::http::status_codes::BadRequest, "Playlist is empty.");
			return;
		}
		try {
			unsigned songIdToPlay = jsonPostBody[utility::conversions::to_string_t("songId")].as_number().to_uint32();
			std::shared_ptr<Song> songPointer = m_game.getCurrentPlayList().getSong(songIdToPlay);
			ScreenPlaylist* m_pp = dynamic_cast<ScreenPlaylist*>(m_game.getScreen("Playlist"));
			m_pp->triggerSongListUpdate();
			m_game.activateScreen("Sing");
			ScreenSing* m_pp2 = dynamic_cast<ScreenSing*>(m_game.getScreen("Sing"));
			m_pp2->setSong(songPointer);

			request.reply(web::http::status_codes::OK, "success");
			return;
		}
		catch (web::json::json_exception const& e) {
			std::string str = std::string("JSON Exception: ") + e.what();
			request.reply(web::http::status_codes::BadRequest, str);
			return;
		}
	}
	else if (path == "/api/search") {
		try {
			auto query = utility::conversions::to_utf8string(jsonPostBody[utility::conversions::to_string_t("query")].as_string());
			m_songs.setFilter(query);
			size_t offset = 0;
			size_t limit = 0;

			if (!jsonPostBody[utility::conversions::to_string_t("offset")].is_null()) {
				offset = jsonPostBody[utility::conversions::to_string_t("offset")].as_integer();
			}
			if (!jsonPostBody[utility::conversions::to_string_t("limit")].is_null()) {
				limit = jsonPostBody[utility::conversions::to_string_t("limit")].as_integer();
			}

			web::json::value jsonRoot = SongsToJsonObject(offset, limit);

			request.reply(web::http::status_codes::OK, jsonRoot);
			return;
		}
		catch (web::json::json_exception const& e) {
			std::string str = std::string("JSON Exception: ") + e.what();
			request.reply(web::http::status_codes::BadRequest, str);
			return;
		}
	}
	else {
		request.reply(web::http::status_codes::NotFound, "The path \"" + path + "\" was not found.");
		return;
	}
}

void RequestHandler::Delete(web::http::http_request request)
{
	request.reply(web::http::status_codes::OK);
}

void RequestHandler::Put(web::http::http_request request)
{
	request.reply(web::http::status_codes::OK);
}

web::json::value RequestHandler::ExtractJsonFromRequest(web::http::http_request request) {
	web::json::value jsonBody = web::json::value::null();
	request.extract_json().then([&jsonBody](pplx::task<web::json::value> task)
		{
			try
			{
				jsonBody = task.get();
			}
			catch (web::json::json_exception const& e)
			{
				SpdLogger::error(LogSystem::WEBSERVER, "CPPRest JSON ERROR. Exception={}", e.what());
			}
		}).wait();

		return jsonBody;
}


web::json::value RequestHandler::SongsToJsonObject(size_t start, size_t limit) {
	size_t startIndex = start;
	size_t endIndex;
	size_t limitCount = limit;
	size_t songCount = m_songs.size();

	if (startIndex > songCount) {
		startIndex = songCount;
	}
	if (limitCount < 1) {
		limitCount = songCount;
	}
	endIndex = startIndex + limitCount;
	if (endIndex > songCount) {
		endIndex = songCount;
	}

	web::json::value jsonRoot = web::json::value::array();
	for (size_t i = startIndex; i < endIndex; i++) {
		web::json::value songObject = web::json::value::object();
		songObject[utility::conversions::to_string_t("Title")] = web::json::value::string(utility::conversions::to_string_t(m_songs[i]->title));
		songObject[utility::conversions::to_string_t("Artist")] = web::json::value::string(utility::conversions::to_string_t(m_songs[i]->artist));
		songObject[utility::conversions::to_string_t("Edition")] = web::json::value::string(utility::conversions::to_string_t(m_songs[i]->edition));
		songObject[utility::conversions::to_string_t("Language")] = web::json::value::string(utility::conversions::to_string_t(m_songs[i]->language));
		songObject[utility::conversions::to_string_t("Creator")] = web::json::value::string(utility::conversions::to_string_t(m_songs[i]->creator));
		songObject[utility::conversions::to_string_t("name")] = web::json::value::string(utility::conversions::to_string_t(m_songs[i]->artist + " " + m_songs[i]->title));
		songObject[utility::conversions::to_string_t("HasError")] = web::json::value::boolean(m_songs[i]->loadStatus == Song::LoadStatus::PARSERERROR);
		songObject[utility::conversions::to_string_t("ProvidedBy")] = web::json::value(utility::conversions::to_string_t(m_songs[i]->providedBy));
		songObject[utility::conversions::to_string_t("Comment")] = web::json::value(utility::conversions::to_string_t(m_songs[i]->comment));
		songObject[utility::conversions::to_string_t("Path")] = web::json::value(utility::conversions::to_string_t(std::filesystem::path(m_songs[i]->path.parent_path()).filename()));
		jsonRoot[i - startIndex] = songObject;
	}

	return jsonRoot;
}

std::shared_ptr<Song> RequestHandler::GetSongFromJSON(web::json::value jsonDoc) {
	m_songs.setFilter("");

	for (size_t i = 0; i < m_songs.size(); i++) {
		if (m_songs[i]->title == utility::conversions::to_utf8string(jsonDoc[utility::conversions::to_string_t("Title")].as_string()) &&
			m_songs[i]->artist == utility::conversions::to_utf8string(jsonDoc[utility::conversions::to_string_t("Artist")].as_string()) &&
			m_songs[i]->edition == utility::conversions::to_utf8string(jsonDoc[utility::conversions::to_string_t("Edition")].as_string()) &&
			m_songs[i]->language == utility::conversions::to_utf8string(jsonDoc[utility::conversions::to_string_t("Language")].as_string()) &&
			m_songs[i]->creator == utility::conversions::to_utf8string(jsonDoc[utility::conversions::to_string_t("Creator")].as_string()) &&
			m_songs[i]->providedBy == utility::conversions::to_utf8string(jsonDoc[utility::conversions::to_string_t("ProvidedBy")].as_string()) &&
			m_songs[i]->comment == utility::conversions::to_utf8string(jsonDoc[utility::conversions::to_string_t("Comment")].as_string())) {
				SpdLogger::info(LogSystem::WEBSERVER, "Found requested song, {} - {}", m_songs[i]->artist, m_songs[i]->title);
				return m_songs[i];
		}
	}
	SpdLogger::info(LogSystem::WEBSERVER, "Couldn't find requested song, {} - {}", utility::conversions::to_utf8string(jsonDoc[utility::conversions::to_string_t("Artist")].as_string()), utility::conversions::to_utf8string(jsonDoc[utility::conversions::to_string_t("Title")].as_string()));
	return std::shared_ptr<Song>();
}

std::map<std::string, std::string> RequestHandler::GenerateLocaleDict() {
	std::vector<std::string> translationKeys = GetTranslationKeys();
	std::map<std::string, std::string> localeMap;
	for (auto const& translationKey : translationKeys) {
		localeMap[translationKey] = _(translationKey);
	}
	return localeMap;
}

std::vector<std::string> RequestHandler::GetTranslationKeys() {
	std::vector<std::string> tranlationKeys = {
		translate_noop("Performous web frontend"),
		translate_noop("View database"),
		translate_noop("View playlist"),
		translate_noop("Search and add"),
		translate_noop("Sort by"),
		translate_noop("Artist"),
		translate_noop("Title"),
		translate_noop("Language"),
		translate_noop("Edition"),
		translate_noop("Creator"),
		translate_noop("Sort order"),
		translate_noop("Normal"),
		translate_noop("Inverted"),
		translate_noop("Update every 10 sec"),
		translate_noop("Refresh database"),
		translate_noop("Upcoming songs"),
		translate_noop("Refresh playlist"),
		translate_noop("Web interface by Niek Nooijens and Arjan Speiard, for full credits regarding Performous see /docs/Authors.txt"),
		translate_noop("Search"),
		translate_noop("Available songs"),
		translate_noop("Search for songs"),
		translate_noop("Yes"),
		translate_noop("No"),
		translate_noop("Move up"),
		translate_noop("Move down"),
		translate_noop("Set position"),
		translate_noop("Remove song"),
		translate_noop("Desired position of song"),
		translate_noop("Cancel"),
		translate_noop("Successfully removed song from playlist."),
		translate_noop("Failed removing song from playlist!"),
		translate_noop("Successfully changed position of song."),
		translate_noop("Failed changing position of song!"),
		translate_noop("Successfully moved song up."),
		translate_noop("Failed moving song up!"),
		translate_noop("Successfully moved song down."),
		translate_noop("Failed moving song down!"),
		translate_noop("Successfully added song to the playlist."),
		translate_noop("Failed adding song to the playlist!"),
		translate_noop("No songs found with current filter."),
		translate_noop("Has error")
	};

	return tranlationKeys;
}
#endif
