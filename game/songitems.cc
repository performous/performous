#include "songitems.hh"

#include "unicode.hh"
#include "libxml++-impl.hh"

#include <algorithm>
#include <memory>
#include <string>


void SongItems::load(xmlpp::NodeSet const& n) {
    for (auto const& elem : n) {
        xmlpp::Element& element = dynamic_cast<xmlpp::Element&>(*elem);

        xmlpp::Attribute* a_id = element.get_attribute("id");
        if (!a_id)
            throw SongItemsException("No attribute id");

        xmlpp::Attribute* a_artist = element.get_attribute("artist");
        if (!a_artist)
            throw SongItemsException("No attribute artist");

        xmlpp::Attribute* a_title = element.get_attribute("title");
        if (!a_title)
            throw SongItemsException("No attribute title");

        auto a_broken = element.get_attribute("broken");
        auto const broken = (a_broken && a_broken->get_value() == "true");

        addSongItem(a_artist->get_value(), a_title->get_value(), broken, std::stoi(a_id->get_value()));
    }
}

void SongItems::save(xmlpp::Element* songs) {
    for (auto const& song : m_songs) {
        xmlpp::Element* element = xmlpp::add_child_element(songs, "song");
        element->set_attribute("id", std::to_string(song.id));
        element->set_attribute("artist", song.artist);
        element->set_attribute("title", song.title);
		element->set_attribute("broken", song.isBroken() ? "true" : "false");
    }
}

SongId SongItems::addSongItem(std::string const& artist, std::string const& title, bool broken, std::optional<SongId> _id) {
    SongItem si;
    si.id = _id.value_or(assign_id_internal());
    si.artist = artist;
    si.title = title;
    si.setBroken(broken);
    std::pair<songs_t::iterator, bool> ret = m_songs.insert(si);
    if (!ret.second)
    {
        si.id = assign_id_internal();
        m_songs.insert(si); // now do the insert with the fresh id
    }
    return si.id;
}

void SongItems::addSong(SongPtr song) {
    SongItem si;
    // Do NOT use .value_or() here; it gets evaluated and addSongItem() runs regardless of whether we have a value, which results in duplicate entries in the database.
    auto val = lookup(song);
    si.id = val ? val.value() : (addSongItem(song->artist, song->title));;
    auto it = m_songs.find(si);
    if (it == m_songs.end())
        throw SongItemsException("Cant find song which was added just before");
    // it->song.reset(song); // does not work, it is a read only structure...

    // fill up the rest of the information
    si.artist = it->artist;
    si.title = it->title;
    si.setBroken(it->isBroken());
    si.setSong(song);

    m_songs.erase(it);
    m_songs.insert(si);
}

std::optional<SongId> SongItems::lookup(Song const& song) const {
    auto const si = std::find_if(m_songs.begin(), m_songs.end(), [&song](SongItem const& si) {

        // This is not always really correct but in most cases these inputs should have been normalized into unicode at one point during their life time.
        if (song.artist.length() != si.artist.length() || song.title.length() != si.title.length())
            return false;

        return UnicodeUtil::caseEqual(song.artist, si.artist, true) && UnicodeUtil::caseEqual(song.title, si.title, true);
    });

    if (si != m_songs.end())
        return si->id;

    return std::nullopt;
}

SongId SongItems::getSongId(SongPtr const& song) const {
    auto const it = std::find_if(m_songs.begin(), m_songs.end(), [song](auto const& item) { return item.getSong() == song; });

    if (it == m_songs.end())
        throw std::logic_error("SongItems::getSongId: Did not find an item matching to song!");

    return it->id;
}

SongPtr SongItems::getSong(SongId id) const
{
	auto const it = std::find_if(m_songs.begin(), m_songs.end(), [id](auto const& item) { return item.id == id; });

	if (it == m_songs.end())
		return {};

	return it->getSong();
}

std::optional<std::string> SongItems::lookup(const SongId& id) const {
    SongItem si;
    si.id = id;
    auto it = m_songs.find(si);
    if (it == m_songs.end())
        return std::nullopt;
    if (!it->getSong())
        return it->artist + " - " + it->title;
    return it->getSong()->artist + " - " + it->getSong()->title;
}

SongId SongItems::assign_id_internal() const {
    // use the last one with highest id
    auto it = std::max_element(m_songs.begin(), m_songs.end());
    if (it != m_songs.end())
        return it->id + 1;
    return 0; // empty set
}


std::shared_ptr<Song> SongItem::getSong() const {
    return m_song;
}

void SongItem::setSong(std::shared_ptr<Song> song) {
    if (song)
        song->setBroken(m_broken);

    m_song = song;
}

bool SongItem::isBroken() const {
    if (m_song)
        return m_song->isBroken();

    return m_broken;
}

void SongItem::setBroken(bool broken) {
    if (m_song)
        m_song->setBroken(broken);

    m_broken = broken;
}
