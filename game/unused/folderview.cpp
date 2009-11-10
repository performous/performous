#include <boost/shared_ptr.hpp>
#include <boost/filesystem.hpp>
#include <algorithm>
#include <iostream>
#include <vector>

namespace fs = boost::filesystem;

class Song
{
  public:
	Song(std::string const& path_, std::string const& filename_ = "") :
		path(path_), filename(filename_)
	{}

	std::string path; ///< path of songfile
	std::string filename; ///< name of songfile
};

namespace {
	class IsFolder: public std::unary_function<boost::shared_ptr<Song>,bool> {
	  public:
		bool operator() (boost::shared_ptr<Song> const song) const
		{
			return song->filename.empty();
		}
	};
	class IsDirectBelow: public std::unary_function<boost::shared_ptr<Song>,bool> {
	  public:
		IsDirectBelow(fs::path path) :
			m_path(path)
		{}
		bool operator() (boost::shared_ptr<Song> const song) const
		{
			fs::path base (song->path);
			base.remove_leaf();
			// std::cout << "base: " << base << "\tm_path: " << m_path << std::endl;
			return base == m_path;
		}
	  private:
		fs::path m_path;
	};
	class IsBelow: public std::unary_function<boost::shared_ptr<Song>,bool> {
	  public:
		IsBelow(fs::path path) :
			m_path(path)
		{}
		bool operator() (boost::shared_ptr<Song> const song) const
		{
			fs::path path (song->path);
			while (!path.empty())
			{
				path.remove_leaf();
				if (path == m_path) return true;
			}
			return false;
		}
	  private:
		fs::path m_path;
	};
}

class Songs
{
  public:
	typedef std::vector<boost::shared_ptr<Song> > SongVector;

	Songs() :
		m_songs()
	{
		m_songs.push_back(boost::shared_ptr<Song>(new Song("/home/songs")));
		m_songs.push_back(boost::shared_ptr<Song>(new Song("/home/songs/protected")));
		m_songs.push_back(boost::shared_ptr<Song>(new Song("/home/songs/protected/Demo", "Demo")));
		m_songs.push_back(boost::shared_ptr<Song>(new Song("/home/songs/protected/Great", "Great")));
		m_songs.push_back(boost::shared_ptr<Song>(new Song("/home/songs/protected/Sub")));
		m_songs.push_back(boost::shared_ptr<Song>(new Song("/home/songs/protected/Sub/Song", "Yeah")));
		m_songs.push_back(boost::shared_ptr<Song>(new Song("/home/songs/free")));
		m_songs.push_back(boost::shared_ptr<Song>(new Song("/home/songs/free/Demo", "Demo")));
		m_songs.push_back(boost::shared_ptr<Song>(new Song("/home/songs/free/Great", "Great")));
		m_songs.push_back(boost::shared_ptr<Song>(new Song("/media/performous")));
		m_songs.push_back(boost::shared_ptr<Song>(new Song("/media/performous/Internet")));
		m_songs.push_back(boost::shared_ptr<Song>(new Song("/media/performous/Internet/Song", "Song")));
		m_songs.push_back(boost::shared_ptr<Song>(new Song("/media/performous/Hits")));
	}
	void only_folders()
	{
		m_songs.erase(std::remove_if(m_songs.begin(), m_songs.end(), std::not1(IsFolder())), m_songs.end());
	}
	void only_songs()
	{
		m_songs.erase(std::remove_if(m_songs.begin(), m_songs.end(), IsFolder()), m_songs.end());
	}
	void only_path(std::string path)
	{
		m_songs.erase(std::remove_if(m_songs.begin(), m_songs.end(), std::not1(IsDirectBelow (path))), m_songs.end());
	}
	void only_root()
	{
		SongVector::iterator end = m_songs.end();
		for (SongVector::const_iterator it = m_songs.begin(); it != end; ++it)
		{
			std::cout << "Removing below: " << (*it)->path << std::endl;
			end = std::remove_if(m_songs.begin(), end, IsBelow ((*it)->path));
		}
		m_songs.erase(end, m_songs.end());
	}
	~Songs()
	{ }
	friend std::ostream & operator<< (std::ostream & os, Songs const& s);
  private:
	SongVector m_songs;

};

std::ostream & operator<< (std::ostream & os, Songs const& s)
{
	for (Songs::SongVector::const_iterator it = s.m_songs.begin(); it!= s.m_songs.end(); ++it)
	{
		os << "Song " << (*it)->path << "/ " << (*it)->filename << std::endl;
	}
	return os;
}

class ScreenSongs
{};

int main(int argc, char**argv)
{
	/*
	if (argc!=2) return 1;
	std::string path = argv[1];
	*/
	Songs songs;
	songs.only_root();
	// songs.only_path(path);
	// songs.only_songs();
	std::cout << songs;
}
