#include "players.hh"
#include "unicode.hh"

#include "configuration.hh"
#include "fs.hh"
#include "libxml++-impl.hh"

#include <algorithm>
#include <unicode/stsearch.h>

void Players::load(xmlpp::NodeSet const& n) {
	for (auto const& elem: n) {
		xmlpp::Element& element = dynamic_cast<xmlpp::Element&>(*elem);
		xmlpp::Attribute* a_name = element.get_attribute("name");
		if (!a_name) throw PlayersException("Attribute name not found");
		xmlpp::Attribute* a_id = element.get_attribute("id");
		if (!a_id) throw PlayersException("Attribute id not found");
		std::optional<PlayerId> id;
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

std::optional<PlayerId> Players::lookup(std::string const& name) const {
	for (auto const& p: m_players) {
		if (p.name == name) return p.id;
	}
	return std::nullopt;
}

std::optional<std::string> Players::lookup(const PlayerId& id) const {
	const auto it = m_players.find(PlayerItem(id));
	if (it == m_players.end()) 
		return std::nullopt;

	return it->name;
}

void Players::addPlayer (std::string const& name, std::string const& picture, std::optional<PlayerId> id) {
	PlayerItem pi;
	pi.name = name;
	pi.picture = picture;

	pi.id = id.value_or(assign_id_internal());

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
	const auto ret = m_players.insert(pi);
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

PlayerId Players::assign_id_internal() {
	const auto it = std::max_element(m_players.begin(),m_players.end());
	
	if (it != m_players.end() && it->id) 
		return it->id + 1;
	
	return 0;
}

void Players::filter_internal() {
	m_dirty = false;
	auto selection = current();

	try {
		fplayers_t filtered;
		if (m_filter.empty()) filtered = fplayers_t(m_players.begin(), m_players.end());
		else {

			auto filter = icu::UnicodeString::fromUTF8(
				UnicodeUtil::convertToUTF8(m_filter)
				);
			icu::ErrorCode icuError;

			std::copy_if (m_players.begin(), m_players.end(), std::back_inserter(filtered), [&](PlayerItem it){
			icu::StringSearch search = icu::StringSearch(filter, icu::UnicodeString::fromUTF8(it.name), UnicodeUtil::m_searchCollator.get(), nullptr, icuError);
			return (search.first(icuError) != USEARCH_DONE);
			});
		}
		m_filtered.swap(filtered);
	} catch (...) {
		fplayers_t(m_players.begin(), m_players.end()).swap(m_filtered);  // Invalid regex => copy everything
	}
	math_cover.reset();

	// Restore old selection
	std::ptrdiff_t pos = 0;
	if (selection.name != "") {
		auto it = std::find(m_filtered.begin(), m_filtered.end(), selection);
		math_cover.setTarget(0, 0);
		if (it != m_filtered.end()) pos = it - m_filtered.begin();
	}
	math_cover.setTarget(pos, count());
}

PlayerItem Players::operator[](unsigned pos) const {
    if (pos < count()) 
        return m_filtered[pos];
    
    return PlayerItem();
}

void Players::advance(std::ptrdiff_t diff) {
    const unsigned size = count();
    if (size == 0) return; // Do nothing if no players are available
    std::ptrdiff_t current = 0;
        current = (static_cast<std::ptrdiff_t>(math_cover.getTarget()) + diff) % size;
    if (current < 0)
        current += count();
    math_cover.setTarget(current, count());
}

PlayerItem Players::current() const {
    if (math_cover.getTarget() < static_cast<ptrdiff_t>(m_filtered.size())) return m_filtered[static_cast<unsigned>(math_cover.getTarget())];
    
    return PlayerItem();
}
