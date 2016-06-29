#include "qr_code.hh"
#include <iostream>
#include <fstream>

QRCode::QRCode(const char* data, std::string path): m_qrCode(QRcode_encodeString(data, 4, QR_ECLEVEL_H, QR_MODE_8, 1)), m_path(path) 
{
	QRCode::WriteBMP(path);
}

void QRCode::WriteBMP(std::string filename)
{
	// Asker's Qrc struct delivered as a pointer, from a C API, but this example doesn't mimic that.
	std::ofstream ofs(filename, std::ios_base::trunc | std::ios_base::binary);
	if(!ofs) {
		std::cout << "Writing " << filename << " failed\n";
		return;
	}

	const int side_len            = 33;  // width and height of the (square) QR Code
	const int pixel_side_len      = 4; // QRC element's size in the bmp image (in pixels)
	const int bmp_line_bytes      = side_len * pixel_side_len * 3;
	const int bmp_line_pad_bytes  = (4 - bmp_line_bytes % 4) % 4;  // bmp line data padding size
	const int bmp_data_size       = side_len * (bmp_line_bytes + bmp_line_pad_bytes);

	BIH bih = { sizeof(bih) };      
	bih.width = side_len * pixel_side_len;   // element count * element size
	bih.height = -side_len * pixel_side_len; // negative height => data begins at top of image
	bih.planes = 1;
	bih.bits = 24;


	const int header_size = sizeof(bih) + 14;  // size of the bmp file header
	const int filesize    = header_size + bmp_data_size; // size of the whole file

	ofs.write("BM", 2);
	ofs.write(reinterpret_cast<const char*>(&filesize), 4);
	ofs.write("\0\0\0\0", 4);  // 2x 16-bit reserved fields
	ofs.write(reinterpret_cast<const char*>(&header_size), 4);
	ofs.write(reinterpret_cast<const char*>(&bih), sizeof(bih));

	// pixel colors, as Blue, Green, Red char-valued triples
	// the terminating null also makes these usable as 32bpp BGRA values, with Alpha always 0.
	static const char fg_color[] = "\0\0\0";
	static const char bg_color[] = "\xff\xff\xff";

	auto pd = m_qrCode->data;
	

	// send pixel data directly to the bmp file
	// QRC elements are expanded into squares
	// whose sides are "pixel_side_len" in length.
	for(int y=0; y<side_len; ++y) {
		for(int j=0; j<pixel_side_len; ++j) {
		    auto pdj = pd;
		    for(int x=0; x<side_len; ++x) {
		        for(int i=0; i<pixel_side_len; ++i) {
		            // *pdj will be 0 or 255 (from "fake" Qrc)
		            // Using "*pdj & 1" here, just to match asker's code
		            // without knowing why this was done.
		            ofs.write(*pdj & 1 ? fg_color : bg_color, 3);
		        }
		        ++pdj;
		    }
		    if(bmp_line_pad_bytes) {
		        ofs.write("\0\0\0", bmp_line_pad_bytes);
		    }
		}
		pd += side_len;
	}
}