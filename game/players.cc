#include "players.hh"
#include "unicode.hh"

#include "configuration.hh"
#include "fs.hh"
#include "libxml++-impl.hh"

#include <algorithm>
#include <regex>
#include <unicode/stsearch.h>

UErrorCode Players::m_icuError = U_ZERO_ERROR;

Players::Players():
	m_players(),
	m_filtered(),
	m_filter(),
	math_cover(),
	m_dirty(false)
{}

Players::~Players() {}

void Players::load(xmlpp::NodeSet const& n) {
	for (auto const& elem: n) {
		xmlpp::Element& element = dynamic_cast<xmlpp::Element&>(*elem);
		xmlpp::Attribute* a_name = element.get_attribute("name");
		if (!a_name) throw PlayersException("Attribute name not found");
		xmlpp::Attribute* a_id = element.get_attribute("id");
		if (!a_id) throw PlayersException("Attribute id not found");
		int id = -1;
		try { id = std::stoi(a_id->get_value()); } catch (std::exception&) { }
		xmlpp::NodeSet n2 = element.find("picture");
		std::string picture;
		if (!n2.empty()) // optional picture element
		{
			auto tn = xmlpp::get_first_child_text(dynamic_cast<xmlpp::Element&>(**n2.begin()));
			picture = tn->get_content();
		}
		addPlayer(a_name->get_value(), picture, id);
	}
	filter_internal();
}

void Players::save(xmlpp::Element *players) {
	for (auto const& p: m_players) {
		xmlpp::Element* player = xmlpp::add_child_element(players, "player");
		player->set_attribute("name", p.name);
		player->set_attribute("id", std::to_string(p.id));
		if (p.picture != "")
		{
			xmlpp::Element* picture = xmlpp::add_child_element(player, "picture");
			picture->add_child_text(p.picture.string());
		}
	}
}

void Players::update() {
	if (m_dirty) filter_internal();
}

int Players::lookup(std::string const& name) const {
	for (auto const& p: m_players) {
		if (p.name == name) return p.id;
	}

	return -1;
}

std::string Players::lookup(int id) const {
	PlayerItem pi;
	pi.id = id;
	auto it = m_players.find(pi);
	if (it == m_players.end()) return "Unknown Player";
	else return it->name;
}

void Players::addPlayer (std::string const& name, std::string const& picture, int id) {
	PlayerItem pi;
	pi.id = id;
	pi.name = name;
	pi.picture = picture;
	pi.path = "";


	if (pi.id == -1) pi.id = assign_id_internal();

	if (pi.picture != "") // no picture, so don't search path
	{
		try {
			pi.path =  findFile(fs::path("pictures") / pi.picture);
		} catch (std::runtime_error const& e)
		{
			std::cerr << e.what() << std::endl;
		}
	}

	m_dirty = true;
	std::pair<players_t::iterator, bool> ret = m_players.insert(pi);
	if (!ret.second)
	{
		pi.id = assign_id_internal();
		m_players.insert(pi); // now do the insert with the fresh id
	}
}

void Players::setFilter(std::string const& val) {
	if (m_filter == val) return;
	m_filter = val;
	filter_internal();
}

int Players::assign_id_internal() {
	auto it = m_players.rbegin();
	if (it != m_players.rend()) return it->id+1;
	else return 1; // empty set
}

void Players::filter_internal() {
	m_dirty = false;
	PlayerItem selection = current();

	try {
		fplayers_t filtered;
		if (m_filter == std::string()) filtered = fplayers_t(m_players.begin(), m_players.end());
		else {
			icu::UnicodeString filter = icu::UnicodeString::fromUTF8(m_filter);
			std::copy_if (m_players.begin(), m_players.end(), std::back_inserter(filtered), [&](PlayerItem it){
			icu::StringSearch search = icu::StringSearch(filter, icu::UnicodeString::fromUTF8(it.name), &UnicodeUtil::m_dummyCollator, nullptr, m_icuError);
			return (search.first(m_icuError) != USEARCH_DONE);
			});
		}
// 		for (auto const& p: m_players) {
// 			if (regex_search(p.name, regex(m_filter, std::regex_constants::icase))) filtered.push_back(p);
// 		}
		m_filtered.swap(filtered);
	} catch (...) {
		fplayers_t(m_players.begin(), m_players.end()).swap(m_filtered);  // Invalid regex => copy everything
	}
	math_cover.reset();

	// Restore old selection
	int pos = 0;
	if (selection.name != "") {
		auto it = std::find(m_filtered.begin(), m_filtered.end(), selection);
		math_cover.setTarget(0, 0);
		if (it != m_filtered.end()) pos = it - m_filtered.begin();
	}
	math_cover.setTarget(pos, size());
}
