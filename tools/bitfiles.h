#include <algorithm>
#include <fstream>
#include <vector>

class outBitFile {
  public:
	outBitFile(const char* filename): outcnt(0), outfile(filename, std::ios::binary) {}
	~outBitFile() { putbuf(); }

	void putbits(unsigned int data, unsigned int n) {
		unsigned int mask = 1 << (n-1);
		for( unsigned int i = 0 ; i < n; i++) {
			outbuf <<= 1;
			if (data & mask) outbuf |= 1;
			mask >>= 1;
			outcnt++;
			if (outcnt == 8) {
				outfile.put(outbuf);
				outcnt = 0;
			}
		}
	}

	void putbuf() {
		if (outcnt > 0) putbits(0, 8-outcnt);
	}

	void put_dcs_y(int len) {
		switch (len) {
			case 0: putbits(4,3); break;
			case 1: putbits(0,2); break;
			case 2: putbits(1,2); break;
			case 3: putbits(5,3); break;
			case 4: putbits(6,3); break;
			case 5: putbits(14,4); break;
			case 6: putbits(30,5); break;
			case 7: putbits(62,6); break;
			case 8: putbits(126,7); break;
			case 9: putbits(254,8); break;
			case 10: putbits(510,9); break;
			case 11: putbits(511,9); break;
		}
	}

	void put_dcs_c(int len) {
		switch (len) {
			case 0: putbits(0,2); break;
			case 1: putbits(1,2); break;
			case 2: putbits(2,2); break;
			case 3: putbits(6,3); break;
			case 4: putbits(14,4); break;
			case 5: putbits(30,5); break;
			case 6: putbits(62,6); break;
			case 7: putbits(126,7); break;
			case 8: putbits(254,8); break;
			case 9: putbits(510,9); break;
			case 10: putbits(1022,10); break;
			case 11: putbits(1023,10); break;
		}
	}

  private:
	int outcnt;
	std::ofstream outfile;
	unsigned char outbuf;
	std::vector<char> outdata;
};

#define BUFFER_SIZE 4096
#define START_CODE 0x000001
#define BYTE_ALIGN 0x80
#define BYTE_START 0x80

class inBitFile {
  public:
	inBitFile(std::vector<char> const& data): indata(data) {
		wdMask = BYTE_START;
		wdIndex = 0;
	}

	void setpos(long int byte, unsigned int bit) {
		wdIndex = byte;
		wdMask = BYTE_START;
		get(bit);
	}

	void getpos( long int *byte, unsigned int *bit) {
		unsigned wdMaskSave = wdMask;
		unsigned count;
		for(count = 0; wdMaskSave != 0x80; count++) wdMaskSave <<= 1;
		*bit = count; // 0x80=0,0x40=1,0x20=2 usw
		*byte = wdIndex;
	}

	int get(int num_bits) {
		unsigned int buf;
		if (!get_bits(&buf, num_bits)) return 0;
		return buf;
	}

	int next_start_code() {
		unsigned int buf;
		/* locate next start code */
		if (wdMask != BYTE_ALIGN) {
			/* not byte aligned */
			/* skip stuffed zero bits */
			wdMask = BYTE_ALIGN;
			wdIndex++;
		}
		if (!next_bits(&buf, 24)) return 0; /* end of bitstream */
		while (buf != START_CODE) {
			if (!get_bits(&buf, 8)) return 0;/* zero byte */
			if (!next_bits(&buf, 24)) return 0;
		}
		return 1;
	}

	int get_dcs_y() {
		int bits;
		if(!(bits = get(2))) return 1; // 00
		if(bits == 1)        return 2; // 01
		bits <<= 1;
		bits |= get(1);
		if(bits == 4) return 0; // 100
		if(bits == 5) return 3; // 101
		if(bits == 6) return 4; // 110
		if(!get(1)) return 5;
		if(!get(1)) return 6;
		if(!get(1)) return 7;
		if(!get(1)) return 8;
		if(!get(1)) return 9;
		if(!get(1)) return 10;
		return 11;
	}

	int get_dcs_c() {
		int bits;
		if(!(bits = get(2))) return 0; // 00
		if(bits == 1) return 1; // 01
		if(bits == 2) return 2; // 10
		if(!get(1)) return 3;
		if(!get(1)) return 4;
		if(!get(1)) return 5;
		if(!get(1)) return 6;
		if(!get(1)) return 7;
		if(!get(1)) return 8;
		if(!get(1)) return 9;
		if(!get(1)) return 10;
		return 11;
	}
  private:
	int get_bits(unsigned int *destBuf, unsigned int num_bits) {
		*destBuf = 0;
		for (unsigned int index = 0; index < num_bits; ++index) {
			if (wdIndex >= indata.size()) return 0;
			/* get next bit */
			*destBuf <<= 1;
			if (indata[wdIndex] & wdMask) *destBuf |= 1;
			/* update bit pointer */
			if (wdMask > 1) wdMask >>= 1;
			else {
				wdIndex++;
				wdMask = BYTE_START;
			}
		}
		return 1;
	}
	
	int next_bits(unsigned int *destBuf, int num_bits) {
		/* save current buffer state */
		unsigned wdMaskSave = wdMask;
		unsigned wdIndexSave = wdIndex;
		int ret = get_bits(destBuf, num_bits);
		/* restore previous buffer state */
		wdMask = wdMaskSave;
		wdIndex = wdIndexSave;
		return ret;
	}
	
	unsigned wdIndex;
	unsigned wdMask;
	std::vector<char> const& indata;
};
