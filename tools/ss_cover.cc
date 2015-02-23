#include "ss_cover.hh"

#include "pak.h"
#include "../common/image.hh"

#include <boost/algorithm/string/case_conv.hpp>
#include <boost/lexical_cast.hpp>
#include <libxml++/libxml++.h>

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <vector>


namespace TX2 {
	unsigned short getWidth(char * buffer) {
		return (*(unsigned short*)&buffer[0x0c])&0x7fff;
	}
	unsigned short getHeight(char * buffer) {
		return (*(unsigned short*)&buffer[0x0e])&0x7fff;
	}

	std::vector<unsigned char> getBuffer(char * src) {
		unsigned int pixels = getWidth(src) * getHeight(src);
		std::vector<unsigned char> buffer(pixels*3);

		char * src_image = src+0x100;
		char * src_palette = src+0x100+pixels+0x100;

		for(unsigned int i = 0; i < pixels; i++){
			unsigned char id = src_image[i];
			buffer[i*3+0] = src_palette[id*4+0]; // red
			buffer[i*3+1] = src_palette[id*4+1]; // green
			buffer[i*3+2] = src_palette[id*4+2]; // blue
		}
		return buffer;
	}

	void transform( char *dst, char * src ) {
		unsigned short width  = TX2::getWidth(src);
		unsigned short height = TX2::getHeight(src);
		char * src_image = src+0x100;
		char * src_palette_header = src+0x100+width*height;
		char * src_palette = src+0x100+width*height+0x100;
		char * dst_image = dst+0x100;
		char * dst_palette_header = dst+0x100+width*height;
		char * dst_palette = dst+0x100+width*height+0x100;
		// Recopy the header
		memcpy(dst,src,0x100);
		// unswizzle the texture
		for (int y = 0; y < height; ++y) {
			for (int x = 0; x < width; ++x) {
				int block_location = (y&(~0xf))*width + (x&(~0xf))*2;
				unsigned int swap_selector = (((y+2)>>2)&0x1)*4;
				int posY = (((y&(~3))>>1) + (y&1))&0x7;
				int column_location = posY*width*2 + ((x+swap_selector)&0x7)*4;
				int byte_num = ((y>>1)&1) + ((x>>2)&2);	// 0,1,2,3
	
				unsigned char uPen = ((unsigned char *) src_image)[block_location + column_location + byte_num];
				// Bitshift the palette
				unsigned char l = (uPen&0x10)>>1;
				unsigned char m = (uPen&0x08)<<1;
				unsigned char o = (uPen&0xe7);
				((unsigned char *) dst_image)[y*width+x] = o|l|m;
			}
		}
		// Recopy the palette header
		memcpy(dst_palette_header,src_palette_header,0x100);
		// Bitshift the palette
		memcpy(dst_palette,src_palette,0x400);
	}
}

SingstarCover::SingstarCover(const std::string pak_file, unsigned int track_id) {
	xmlpp::Node::PrefixNsMap nsmap;
	nsmap["ss"] = "http://www.singstargame.com";
	std::string filename("export/covers.xml");

	// extract cover information from XML
	Pak p(pak_file);
	std::vector<char> buf;
	p[filename].get(buf);
	std::string id(std::string("cover_") + boost::lexical_cast<std::string>(track_id));
	std::string xpath = std::string("/ss:TPAGE_BIT_SET/ss:TPAGE_BIT[@NAME='") + id + "']";
	xmlpp::DomParser dom;
	std::string tmp( buf.begin(), buf.end() );
	dom.parse_memory(tmp);
	xmlpp::NodeSet n = dom.get_document()->get_root_node()->find(xpath, nsmap);
	if (n.empty()) {
		std::string xpath_old = std::string("/TPAGE_BIT_SET/TPAGE_BIT[@NAME='") + id + "']";
		n = dom.get_document()->get_root_node()->find(xpath_old, nsmap);
		if (n.empty())
			throw std::runtime_error("Unable to find cover information");
	}
	xmlpp::Element& e = dynamic_cast<xmlpp::Element&>(*n[0]);
	m_u = boost::lexical_cast<unsigned int>(e.get_attribute("U")->get_value());
	m_v = boost::lexical_cast<unsigned int>(e.get_attribute("V")->get_value());
	m_width = boost::lexical_cast<unsigned int>(e.get_attribute("WIDTH")->get_value());
	m_height = boost::lexical_cast<unsigned int>(e.get_attribute("HEIGHT")->get_value());
	std::string texture_filename = std::string("export/textures/") + e.get_attribute("TEXTURE")->get_value() + ".tx2";

	// extract and unswizzle image from pak
	std::vector<char> buf_image;
	p[texture_filename].get(buf_image);
	m_image = buf_image;
	TX2::transform(&m_image[0],&buf_image[0]);
}

SingstarCover::~SingstarCover() {}

void SingstarCover::write(const fs::path filename) {
	// grab format from tx2 file
	Bitmap img;
	img.width = TX2::getWidth(&m_image[0]);
	img.height = TX2::getHeight(&m_image[0]);
	img.fmt = pix::RGB;
	img.buf = TX2::getBuffer(&m_image[0]);

	// crop image according to information stored in xml
	img.crop(m_width, m_height, m_u, m_v);

	auto ext = boost::algorithm::to_lower_copy(filename.extension().string());
	if (ext == ".png") writePNG(filename, img);
	//else if (ext == ".jpg" || ext == ".jpeg") writeJPEG(filename, img);
	else throw std::runtime_error("Don't know how to write format: " + ext);
}

