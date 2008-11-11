#include "bitfiles.h"

struct t_MBData		/*	Macroblock data	*/
{
	long int Byte;
	unsigned int Bit;
	int dct_dc_y;
	int dct_dc_cb;
	int dct_dc_cr;
	int quant;
};

class IPUConv {
	inBitFile infile;
	outBitFile outfile;
	int vlc(int write);
  public:
	IPUConv(std::vector<char> const& indata, std::string const& outfilename);
};

