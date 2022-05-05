#include "players.hh"
#include "unicode.hh"

#include "configuration.hh"
#include "fs.hh"
#include "libxml++-impl.hh"

#include <algorithm>
#include <unicode/stsearch.h>

void Players::load(xmlpp::NodeSet const& n) {
	for (auto const& elem: n) {
		auto player = PlayerItem();
		auto& element = dynamic_cast<xmlpp::Element&>(*elem);
		auto a_name = element.get_attribute("name");
		if (!a_name) throw PlayersException("Attribute name not found");
		auto a_id = element.get_attribute("id");
		if (!a_id) throw PlayersException("Attribute id not found");
		std::optional<PlayerId> id;
		try {
			id = std::stoi(a_id->get_value());
		} catch (std::exception&) { }

		player.id = id;
		player.setName(a_name->get_value());

		auto n2 = element.find("picture");
		if (!n2.empty()) // optional picture element
		{
			auto tn = xmlpp::get_first_child_text(dynamic_cast<xmlpp::Element&>(**n2.begin()));
			auto const picture = fs::path(tn->get_content());

			player.picture = picture;
			player.path = findFile(picture);
		}

		auto active = element.get_attribute("active");
		if (!active)
			player.setActive(true);
		else
			player.setActive(active->get_value() == "true");

		addPlayer(player);
	}
	filter_internal();
}

void Players::save(xmlpp::Element *players) {
	std::cout << "Players::save" << std::endl;
	for (auto const& player: m_players) {
		std::cout << "save " << player->getName() << std::endl;
		auto playerNode = xmlpp::add_child_element(players, "player");
		playerNode->set_attribute("name", player->getName());
		playerNode->set_attribute("id", std::to_string(player->id));
		playerNode->set_attribute("active", player->isActive() ? "true" : "false");
		if (player->picture != "")
		{
			std::cout << "save image" << std::endl;
			auto pictureNode = xmlpp::add_child_element(playerNode, "picture");
			pictureNode->add_child_text(player->picture.string());
		}
	}
}

void Players::update() {
	filter_internal();
}

std::optional<PlayerId> Players::lookup(std::string const& name) const {
	for (auto const& p: m_players) {
		if (p->name == name) return p->id;
	}
	return std::nullopt;
}

std::optional<std::string> Players::lookup(const PlayerId& id) const {
	const auto it = std::find_if(m_players.begin(), m_players.end(), [id](auto const& player){ return player->id == id;});
	if (it == m_players.end())
		return std::nullopt;

	return (*it)->name;
}

void Players::addPlayer(PlayerItem const& p) {
	auto player = p;

	//pi.id = id.value_or(assign_id_internal());
	//if (player.id == PlayerItem::UndefinedPlayerId)
	//	player.id = assign_id_internal();

	if (player.picture != "") // no picture, so don't search path
	{
		try {
			player.path = findFile("pictures" / player.picture);
		} catch (std::runtime_error const& e) {
			std::cerr << e.what() << std::endl;
			try {
				player.path = findFile("avatar" / player.picture);
			} catch (std::runtime_error const& e) {
				std::cerr << e.what() << std::endl;
			}
		}
	}

	const auto ret = m_players.emplace(std::make_shared<PlayerItem>(player));
	if (!ret.second)
	{
		player.id = assign_id_internal();
		m_players.emplace(std::make_shared<PlayerItem>(player)); // now do the insert with the fresh id
	}
}

void Players::addPlayer(std::string const& name, std::string const& picture, std::optional<PlayerId> id) {
	PlayerItem pi;
	
	pi.id = id.value_or(assign_id_internal());
	pi.name = name;
	pi.picture = picture;

	addPlayer(pi);
}

void Players::setFilter(std::string const& val) {
	if (m_filter == val) return;
	m_filter = val;
	filter_internal();
}

PlayerId Players::assign_id_internal() {
	const auto it = std::max_element(m_players.begin(),m_players.end());

	if (it != m_players.end() && it->id) 
		return (*it)->id + 1;

	return 0;
}

void Players::filter_internal() {
	auto const selection = current();

	try {
		auto filtered = fplayers_t();

		auto filter = icu::UnicodeString::fromUTF8(
			UnicodeUtil::convertToUTF8(m_filter)
			);
		icu::ErrorCode icuError;

		std::copy_if(m_players.begin(), m_players.end(), std::back_inserter(filtered),
			[&](auto const& it){
				std::cout << "filter player " << it->getName() << " active: " << it->isActive() << std::endl;

				if(!it->isActive())
					return false;

				if(m_filter.empty())
					return true;

				auto search = icu::StringSearch(filter, icu::UnicodeString::fromUTF8(it->getName()), UnicodeUtil::m_searchCollator.get(), nullptr, m_icuError);

				return (search.first(m_icuError) != USEARCH_DONE);
			});

		std::cout << "filtered players: " << filtered.size() << std::endl;
		m_filtered.swap(filtered);
	} catch (...) {
		fplayers_t(m_players.begin(), m_players.end()).swap(m_filtered);  // Invalid regex => copy everything
	}
	math_cover.reset();

	// Restore old selection
	std::ptrdiff_t pos = 0;
	if (selection.name != "") {
		auto it = std::find_if(m_filtered.begin(), m_filtered.end(), [selection](auto const& player){ return *player ==  selection;});
		math_cover.setTarget(0, 0);
		if (it != m_filtered.end()) pos = it - m_filtered.begin();
	}
	math_cover.setTarget(pos, count());
}

PlayerItem const& Players::operator[](unsigned pos) const {
	if (pos < count())
		return *m_filtered[pos];

	throw std::runtime_error("No player with at pos " + std::to_string(pos) + " found!");
}

PlayerItem& Players::operator[](unsigned pos) {
	if (pos < count())
		return *m_filtered[pos];

	throw std::runtime_error("No player with at pos " + std::to_string(pos) + " found!");
}

void Players::advance(std::ptrdiff_t diff) {
    const unsigned size = count();
    if (size == 0) 
    	return; // Do nothing if no players are available
    std::ptrdiff_t current = (static_cast<std::ptrdiff_t>(math_cover.getTarget()) + diff) % size;
	if (current < 0)
		current += count();
	math_cover.setTarget(current, count());
}

PlayerItem const& Players::current() const {
    if (math_cover.getTarget() < static_cast<ptrdiff_t>(m_filtered.size())) 
    	return m_filtered[static_cast<unsigned>(math_cover.getTarget())];

	static PlayerItem empty;

	return empty;
}
