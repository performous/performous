#include <cstdlib>
#include <cstring>
#include <iostream>
#include <stdexcept>

// TODO: rewrite with libxml++
#include <libxml/parser.h>
#include <libxml/tree.h>

#include <Magick++.h>

#include "pak.h"

struct CoverFile {
	std::string filename;
	int u;
	int v;
	int w;
	int h;
};

struct CoverFile cover;

bool parseCover(xmlNode *cur_node, char * song_id) {
	bool found = false;
	int u = 0, v = 0, w = 0 ,h = 0;
	std::string filename;
	std::string id(std::string("cover_") + std::string(song_id));
	for ( xmlAttr *cur_attr = cur_node->properties; cur_attr; cur_attr = cur_attr->next) {
		if( !strcmp( (char *)cur_attr->name, "NAME" ) ) {
			std::string tmp((char *)xmlGetProp(cur_node, cur_attr->name));
			found = (tmp == id);
		} else if( !strcmp( (char *)cur_attr->name, "TEXTURE" ) ) {
			std::string tmp((char *)xmlGetProp(cur_node, cur_attr->name));
			filename.swap(tmp);
		} else if( !strcmp( (char *)cur_attr->name, "U" ) ) {
			u = atoi((char *)xmlGetProp(cur_node, cur_attr->name));
		} else if( !strcmp( (char *)cur_attr->name, "V" ) ) {
			v = atoi((char *)xmlGetProp(cur_node, cur_attr->name));
		} else if( !strcmp( (char *)cur_attr->name, "WIDTH" ) ) {
			w = atoi((char *)xmlGetProp(cur_node, cur_attr->name));
		} else if( !strcmp( (char *)cur_attr->name, "HEIGHT" ) ) {
			h = atoi((char *)xmlGetProp(cur_node, cur_attr->name));
		}
	}
	if( found ) {
		std::string tmp(std::string("export/textures/") + filename + std::string(".tx2"));
		cover.filename.swap(tmp);
		cover.u = u;
		cover.v = v;
		cover.w = w;
		cover.h = h;
	}
	return found;
}

bool walk_tree(xmlNode * a_node, unsigned int depth, char * song_id ) {
	for ( xmlNode *cur_node = a_node; cur_node; cur_node = cur_node->next) {
		if (cur_node->type == XML_ELEMENT_NODE) {
			if( depth == 0 ) {
				if( !strcmp( (char *)cur_node->name, "TPAGE_BIT_SET" ) )
					void();
				else
					throw std::runtime_error("Wrong 0 element");
			}
			if( depth == 1 ) {
				if( !strcmp( (char *)cur_node->name, "TPAGE_BIT" ) ) {
					if( parseCover(cur_node,song_id) )
						return true;
				} else {
					std::cerr << (char *)cur_node->name << std::endl;
					throw std::runtime_error("Wrong 1 element");
				}
			}
		}
		if( walk_tree(cur_node->children, depth+1,song_id) )
			return true;
	}
	return false;
}

unsigned short getWidthFromTX2(char * buffer) {
	return (*(unsigned short*)&buffer[0x0c])&0x7fff;
}
unsigned short getHeightFromTX2(char * buffer) {
	return (*(unsigned short*)&buffer[0x0e])&0x7fff;
}

unsigned char * getBufferFromTX2(char * src) {
	unsigned short width  = getWidthFromTX2(src);
	unsigned short height = getHeightFromTX2(src);
	unsigned char *buffer = new unsigned char[width*height*4];

	char * src_image = src+0x100;
	char * src_palette = src+0x100+width*height+0x100;

	for(unsigned short i = 0; i < width*height; i++){
		unsigned char id = src_image[i];
		buffer[i*4+0] = src_palette[id*4+0]; // blue
		buffer[i*4+1] = src_palette[id*4+1]; // green
		buffer[i*4+2] = src_palette[id*4+2]; // blue;
		buffer[i*4+3] = src_palette[id*4+3]; // alpha
		buffer[i*4+3] = 0xff;
	}
	return buffer;
}

void transformTX2( char *dst, char * src ) {
	unsigned short width  = getWidthFromTX2(src);
	unsigned short height = getHeightFromTX2(src);
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

int main( int argc, char **argv) {
	xmlDoc *doc = NULL;
	xmlNode *root_element = NULL;
	if(argc != 4 ) {
		std::cout << "Usage: " << argv[0] << " [pak_file] [track_id] [output_image]" << std::endl;
		return EXIT_FAILURE;
	}

	Pak p(argv[1]);

	std::string filename("export/covers.xml");
	std::vector<char> buf;
	p[filename].get(buf);

	doc = xmlReadMemory(&buf[0], buf.size(),filename.c_str(),NULL,0);
	root_element = xmlDocGetRootElement(doc);
	if( root_element == NULL ) {
		std::cout << "Cannot open xml file export/covers.xml" << std::endl;
		return EXIT_FAILURE;
	}
	
	if( !walk_tree(root_element,0,argv[2]) )
		throw std::runtime_error("Cannot find cover informations");
		
	xmlFreeDoc(doc);
	xmlCleanupParser();

	std::vector<char> buf_image;
	p[cover.filename].get(buf_image);

	std::vector<char> buf_image1(buf_image);
	transformTX2(&buf_image1[0],&buf_image[0]);

	unsigned short width = getWidthFromTX2(&buf_image1[0]);
	unsigned short height = getHeightFromTX2(&buf_image1[0]);
	unsigned char * buffer = getBufferFromTX2(&buf_image1[0]);

	Magick::Blob blob( buffer, width*height*4);
	Magick::Image image;
	char geometry[16];
	sprintf(geometry,"%dx%d",width,height);
	image.size(geometry);
	image.depth(8);
	image.magick( "RGBA" ); 
	image.read(blob);
	image.crop( Magick::Geometry(cover.w,cover.h, cover.u, cover.v) );
	image.write(argv[3]);

	return EXIT_SUCCESS;
}
