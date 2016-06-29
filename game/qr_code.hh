#pragma once

#include <qrencode.h>
#include <boost/filesystem/path.hpp>

namespace fs = boost::filesystem;

struct BIH {   // a private definition of BITMAPINFOHEADER
    unsigned int   sz;
    int            width, height;
    unsigned short planes;
    short          bits;
    unsigned int   compress, szimage;
    int            xppm, yppm;
    unsigned int   clrused, clrimp;
};


class QRCode {
	public:
		/// Constructor
		QRCode(const char* data, std::string path);
	private:
		void WriteBMP(std::string path);
		QRcode* m_qrCode;
		std::string m_path;
};