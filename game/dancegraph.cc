#include "dancegraph.hh"
#include "fs.hh"

/// Constructor
DanceGraph::DanceGraph(Audio& audio, Song const& song):
  m_audio(audio),
  m_song(song),
  m_input(input::DANCEPAD),
  m_arrow("arrow.svg"),
  m_cx(0.0, 0.2),
  m_width(0.5, 0.4),
  m_stream(),
  m_dead(1000),
  m_text(getThemePath("sing_timetxt.svg"), config["graphic/text_lod"].f()),
  m_correctness(0.0, 5.0),
  m_score(),
  m_scoreFactor(),
  m_streak(),
  m_longestStreak()
{
	//TODO	
	
	
}


/// Handles input
void DanceGraph::engine() {
	//TODO	
	
}


/// Handles scoring and such
void DanceGraph::dance(double time, input::Event const& ev) {
	//TODO: 
	
}


/// Draws the dance graph
void DanceGraph::draw(double time) {
	//TODO
	
}


/// Draws a single note (or hold)
void DanceGraph::drawNote(int fret, glutil::Color, float tBeg, float tEnd) {
	//TODO
	
}
