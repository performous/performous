#include "downloader.hh"


class BSDownloader : public Downloader {

	public:
		BSDownloader();
		~BSDownloader();
		void pause(bool state);
		void pauseResume(std::string sha1);
		void addDownload(std::string url);
		void removeDownload(std::string sha1);
		std::vector<DLCContent> getDownloads() const;
		int getUploadRate() const;
		int getDownloadRate() const;
		void downloadFile(std::string hostname, std::string port, std::string filename,
		 	unsigned int timeout, std::string outputFilename);
		
		
	//private:
};
