/*
This is the heaser file of DLC class. DLC class is responsible for providing a DLC mechanism for performous game. This class uses downloader.hh class for downloading mechanism using torrent.
This class uses cpp-netlib to 
1.get a list of songs (also called catalog) from performous server and 
2.parse the calatog and 
3.store the list somewhere safe

DLC cycle can be something like this
-on startup check to see if user checked update-on-startup. if so try to get the new catalog from server. alternatively user can press update-catalog from dlc menu anytime he/she wants.
-

*/


#include <string>
using namespace std;

struct DlcConfig{
	DlcConfig(){};
	std::string DlcAddr;
	int DlcPort;
};

class Dlc {
	public:
		//constructor and desctructor declaration
		Dlc();
		Dlc(DlcConfig dlcConfig);
		~Dlc();
		
		/*	this function trys to download catalog.xml from server 
			using the server address and port specified before.	*/
		//void getTheCatalog();
		void getCatalog();
		
		/*	This function gets new Dlc Configuration 
			then set and save them.	*/
		void setDlcConfig(DlcConfig newDlcConfig);
		
		/*	This function gets a songID and try to add it to
			download list	*/
		void addDownload(int songId);
		
		void removeDownload(int songId);
		
		/*
		static bool enabled() {
			#ifdef USE_DLC
				return true;
			#else
				return false;
			#endif
		}
		*/	

	private:
		DlcConfig dlcConfig;
		//Catalog catalog;
};
