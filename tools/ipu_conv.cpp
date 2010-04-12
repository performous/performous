#include "ipuconv.hh"
#include <iostream>
#include <stdexcept>

IPUConv::IPUConv(std::vector<char> const& indata, std::string const& outfilename, bool pal): infile(indata), outfile(outfilename.c_str()) {
	int sizex;
	int sizey;
	int frames;
	int frame;
	int dct_dc_y;
	int dct_dc_cb;
	int dct_dc_cr;
	int quant;
	int flag;
	int mb;
	int mb_source;
	int intraquant;
	int block;
	int eob;
	int size;
	int diff = 0;
	int absval;
	long int FrameByte;
	unsigned int FrameBit;

	if (0x6970756d != infile.get(32)) throw std::runtime_error("Input data is no IPU");

	infile.get(32);	// Filesize

	sizex=infile.get(8);
	sizex=sizex+infile.get(8)*(1<<8);

	sizey=infile.get(8);
	sizey=sizey+infile.get(8)*(1<<8);

	frames=infile.get(8);
	frames=frames+infile.get(8)*(1<<8);
	frames=frames+infile.get(8)*(1<<16);
	frames=frames+infile.get(8)*(1<<24);

	printf("%dx%d\n",sizex,sizey);
	printf("%02d:%02d:%02d.%02d\n\n",frames/25/60/60,(frames%(25*60*60))/25/60,(frames%(25*60))/25,frames%25);

	std::vector<t_MBData> MBData((sizex/16)*(sizey/16)+1);
	for (frame=0;frame<frames;frame++){
		if (frame % 100 == 0) std::cout << "Frame: " << frame << "/" << frames << "\r" << std::flush;
		flag = infile.get(8);

		if (flag & 32) {
			fprintf (stderr,"Intra VLC format not supported\n");
			exit(1);
		}
		if (frame==0) {
			// Write Sequence Header
			outfile.putbits(0x1b3,32);
			outfile.putbits(sizex,12);
			outfile.putbits(sizey,12);
			outfile.putbits(0x1,4);			// Ascpect Ratio	1     1:1
									//					2     4:3
									//					3    16:9
									//					4  2.21:1

			if(pal) {
				outfile.putbits(0x3,4);			// Framerate		3  25 fps
			} else {
				outfile.putbits(0x4,4);			// Framerate		4  29.97 fps
			}

			outfile.putbits(0x30d4,18);	// Bitrate ($3FFFF=Variabel)
			outfile.putbits(1,1);			// Marker, soll immer 1 sein
			outfile.putbits(112,10);		// VBV
			outfile.putbits(0,1);			// Constrained Parameter Flag
			outfile.putbits(0,1);			// Intra Matrix Standard
			outfile.putbits(0,1);			// Non-Intra Matrix Standard
			//outfile.putbits(0x2cee100,32);

			if (!(flag & 128)) {
				// Sequence Extension
				outfile.putbits(0x1b5,32);
				outfile.putbits(0x1,4);		// Start Code Identifier
				outfile.putbits(0x4,4);		// Main Profil
				outfile.putbits(0x8,4);		// Main Level
				outfile.putbits(0x1,1);		// Progressive Sequence
				outfile.putbits(0x1,2);		// Chroma Format 4:2:0
				outfile.putbits(0x0,2);		// Breite Extension
				outfile.putbits(0x0,2);		// Höhe Extension
				outfile.putbits(0x0,12);	// Bitrate Extension
				outfile.putbits(0x1,1);		// Marker
				outfile.putbits(0x0,8);		// VBV Buffer Extension
				outfile.putbits(0x0,1);		// Low Delay
				outfile.putbits(0x0,2);		// Framerate Extension Numerator
				outfile.putbits(0x0,5);		// Framerate Extension Denominator
				//outfile.putbits(0x148a0001,32);
				//outfile.putbits(0,16);

				// Sequence Display Extension
				/*
				outfile.putbits(0x1b5,32);
				outfile.putbits(0x2,4);		// Start Code Identifier
				outfile.putbits(0x1,3);		// Video Format		1 PAL
				outfile.putbits(0x0,1);		// Bit Color

				outfile.putbits(0x23050504,32);

				outfile.putbits(sizex,14);	// Display Breite
				outfile.putbits(1,1);		// Marker
				outfile.putbits(sizey,14);  // Display Höhe
				outfile.putbits(0,3);
				*/
			}
		}

		// Write GOP Header
		outfile.putbits(0x1b8,32);
		outfile.putbits(0,1);				// Drop Frame
		outfile.putbits(frame/25/60/60,5);	// Stunden
		outfile.putbits((frame%(25*60*60))/25/60,6); // Minuten
		outfile.putbits(1,1);				// Marker
		outfile.putbits((frame%(25*60))/25,6);// Sekunden
		outfile.putbits(frame%25,6);		// Frames
		outfile.putbits(1,1);				// Closed GOP
		outfile.putbits(0,6);

		// Write Picture Header
		outfile.putbits(0x100,32);
		outfile.putbits(0x0,10);			// Temporal Reference
		outfile.putbits(0x1,3);				// Coding Type Intra
		outfile.putbits(0xffff,16);			// VBV Delay
		outfile.putbits(0,3);
//		outfile.putbits(0xffff8,32);

		// Write Picture Coding Extension
		if (!(flag & 128)) {
			outfile.putbits(0x1b5,32);
			outfile.putbits(0x8ffff,20);
			outfile.putbits(flag&3,2);			// Intra DC Precision
			outfile.putbits(3,2);				// Frame Picture
			outfile.putbits(2,3);
			outfile.putbits((flag&64)/64,1);	// QST
			outfile.putbits(0,1);				// Intra VLC Format
			outfile.putbits((flag&16)/16,1);	// Alternate Scan
			outfile.putbits(1,2);
			outfile.putbits(0x80,8);
		}
		// Write Slice Header
		outfile.putbits(0x1,24);
		outfile.putbits(1,8);
		outfile.putbits(1,5);		// Quantiser
		outfile.putbits(0,1);		// Extra Bit clear

		// infile.get Macroblock-infos
		dct_dc_y = 0;
		dct_dc_cb = 0;
		dct_dc_cr = 0;
		quant = 1;
		for(mb=0;mb<(sizex/16)*(sizey/16);mb++) {
			if (mb>0 && !infile.get(1)) throw std::runtime_error("MBA_Incr wrong in IPU");
			// Save position (in IPU) of Macroblock
			infile.getpos(&MBData[mb].Byte,&MBData[mb].Bit);

			if (infile.get(1)) intraquant = 0;
			else {
				if (!infile.get(1)) throw std::runtime_error("MBT wrong in IPU");
				intraquant = 1;
			}

			if (flag & 4) infile.get(1);
			if (intraquant) quant = infile.get(5);
			MBData[mb].quant = quant;

			for(block=0;block<6;block++) {
				if (block<4) {
					if ( (size = infile.get_dcs_y())) {
						diff = infile.get(size);
						if (!(diff & (1 << (size - 1)))) diff = (-1 << size) | (diff + 1);
						dct_dc_y += diff;
					}
					if (block==0) MBData[mb].dct_dc_y = dct_dc_y;
				} else {
					if ( (size = infile.get_dcs_c())) {
						diff = infile.get(size);
						if (!(diff & (1 << (size - 1)))) diff = (-1 << size) | (diff + 1);
					}
					if (block==4) {
						if (size) dct_dc_cb += diff;
						MBData[mb].dct_dc_cb = dct_dc_cb;
					} else {
						if (size) dct_dc_cr += diff;
						MBData[mb].dct_dc_cr = dct_dc_cr;
					}
				}
				do {
					eob = vlc(0);
					if (eob == 0) infile.get(1);
					} while (eob!=1);
			}
		}

		infile.getpos(&FrameByte,&FrameBit);	// Position speichern

		// Write Macroblocks
		dct_dc_y = 0;
		dct_dc_cb = 0;
		dct_dc_cr = 0;
		quant = 1;
		for(mb=0;mb<(sizex/16)*(sizey/16);mb++) {
			mb_source = (mb % (sizex / 16)) * (sizey / 16)+ mb / (sizex / 16);	// Singstar
			// mb_source = mb;							// Other IPUs

			outfile.putbits(1,1);		// MBA_Incr=1

			infile.setpos(MBData[mb_source].Byte,MBData[mb_source].Bit);

			if (infile.get(1)) intraquant = 0;
			else {
				if (!infile.get(1)) throw std::runtime_error("MBT wrong in IPU");
				intraquant = 1;
			}

			if (mb == 0 || (MBData[mb_source].quant != quant)) outfile.putbits(1,2);
			else outfile.putbits(1,1);

			if (flag & 4) outfile.putbits(infile.get(1),1);
			if (intraquant) quant = infile.get(5);

			if (mb == 0 || (MBData[mb_source].quant != quant)) {
				outfile.putbits(MBData[mb_source].quant,5);
				quant = MBData[mb_source].quant;
			}

			for(block=0;block<6;block++) {
				if (block==0) {
					infile.get(infile.get_dcs_y());	// DCT_DC in Eingabestream überspringen
					diff = MBData[mb_source].dct_dc_y - dct_dc_y;
					dct_dc_y = MBData[mb_source].dct_dc_y;

					absval = (diff<0) ? -diff : diff; /* abs(val) */
					size = 0;
					while (absval) {
						absval >>= 1;
						size++;
					}
					outfile.put_dcs_y(size);
					absval = diff;
					if (absval<=0)
						absval += (1<<size) -1;
					outfile.putbits(absval,size);
				} else if (block>3)	{
					infile.get(infile.get_dcs_c());	// DCT_DC in Eingabestream überspringen
					if (block==4) {
						diff = MBData[mb_source].dct_dc_cb - dct_dc_cb;
						dct_dc_cb = MBData[mb_source].dct_dc_cb;
					} else {
						diff = MBData[mb_source].dct_dc_cr - dct_dc_cr;
						dct_dc_cr = MBData[mb_source].dct_dc_cr;
					}
					absval = (diff<0) ? -diff : diff; /* abs(val) */
					size = 0;
					while (absval) {
						absval >>= 1;
						size++;
					}
					outfile.put_dcs_c(size);
					absval = diff;
					if (absval<=0)
						absval += (1<<size) -1;
					outfile.putbits(absval,size);
				} else {
					size = infile.get_dcs_y();
					outfile.put_dcs_y(size);
					diff = infile.get(size);
					outfile.putbits(diff,size);		// DCT_DC kopieren (Blöcke 1,2,3)
					if (size) {
						if (!(diff&(1<<(size - 1))))
							diff = (-1 << size) | (diff + 1);
						dct_dc_y += diff;
					}
				}
				do {
					eob = vlc(1);
					if (eob == 0)
						outfile.putbits(infile.get(1),1);
				} while (eob!=1);
			}
		}
		outfile.putbuf();

		infile.setpos(FrameByte,FrameBit);

		// Jump to End of Frame
		if (!infile.next_start_code()) throw std::runtime_error("End of Stream");
		if (infile.get(32) != 0x000001b0) throw std::runtime_error("No 1b0");
	}

	outfile.putbits(0x1b7,32);		// Ende
}

int IPUConv::vlc(int write){
	int bits;
	int level=0;

	bits = infile.get(2);
	if (write) outfile.putbits(bits,2);
	if(bits ==	2)
		return(1);		// 10 - EOB
	if(bits ==	3)
		return(0);		// 11
	if(bits ==	1) {
		bits = infile.get(1);
		if (write) outfile.putbits(bits,1);
		if (bits)
			return(0);
		else {
			bits = infile.get(1);
			if (write) outfile.putbits(bits,1);
			return(0);
		}
	}
	// 00
	bits = infile.get(1);
	if (write) outfile.putbits(bits,1);
	if (bits) {
		// 001
		bits = infile.get(2);
		if (write) outfile.putbits(bits,2);
		if (bits < 1) {
			// 00100
			bits = infile.get(3);
			if (write) outfile.putbits(bits,3);
		}
		// 001xx
		return(0);
	} else {
	// 000
		bits = infile.get(3);
		if (write) outfile.putbits(bits,3);
		if(bits >=	4)
			return(0);
		if(bits >=	2) {
			bits = infile.get(1);
			if (write) outfile.putbits(bits,1);
			return(0);
		}
		if(bits) {
			bits = infile.get(18);
			if (write) outfile.putbits(bits,18);
			return(2);	// Escape
		}
		bits = infile.get(1);
		if (write) outfile.putbits(bits,1);
		if(bits) {
			bits = infile.get(3);
			if (write) outfile.putbits(bits,3);
			return(0);
		}
		do {
			bits = infile.get(1);
			if (write) outfile.putbits(bits,1);
			level++;
		} while (!bits && level<6);
		if (level<6) {
			bits = infile.get(4);
			if (write) outfile.putbits(bits,4);
			return(0);
		} else throw std::runtime_error("Invalid VLC");
	}
}

