#include "songorder.hh"

#include <chrono>

struct RecentlySungSongOrder : public SongOrder {
	std::string getDescription() const override;

	void initialize(SongCollection const& songs, Database const& database) override;

	void update(SongPtr const& songs, Database const& database) override;

	bool operator()(Song const& a, Song const& b) const override;

  private:
	std::map<Song const*, std::chrono::seconds> m_dateMap;
	bool initialized;
};

