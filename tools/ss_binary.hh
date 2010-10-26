#include "adpcm.h"
#include "ipuconv.hh"

unsigned getLE16(char* buf) { unsigned char* b = reinterpret_cast<unsigned char*>(buf); return b[0] | (b[1] << 8); }
unsigned getLE32(char* buf) { unsigned char* b = reinterpret_cast<unsigned char*>(buf); return b[0] | (b[1] << 8) | (b[2] << 16) | (b[3] << 24); }

void writeWavHeader(std::ostream& outfile, unsigned ch, unsigned sr, unsigned samples) {
	unsigned bps = ch * 2; // Bytes per sample
	unsigned datasize = bps * samples;
	unsigned size = datasize + 0x2C;
	outfile.write("RIFF" ,4); // RIFF chunk
	{ unsigned int tmp=size-0x8 ; outfile.write((char*)(&tmp),4); } // RIFF chunk size
	outfile.write("WAVEfmt ",8); // WAVEfmt header
	{ int   tmp=0x00000010 ; outfile.write((char*)(&tmp),4); } // Always 0x10
	{ short tmp=0x0001     ; outfile.write((char*)(&tmp),2); } // Always 1
	{ short tmp = ch; outfile.write((char*)(&tmp),2); } // Number of channels
	{ int   tmp = sr; outfile.write((char*)(&tmp),4); } // Sample rate
	{ int   tmp = bps * sr; outfile.write((char*)(&tmp),4); } // Bytes per second
	{ short tmp = bps; outfile.write((char*)(&tmp),2); } // Bytes per frame
	{ short tmp = 16; outfile.write((char*)(&tmp),2); } // Bits per sample
	outfile.write("data",4); // data chunk
	{ int   tmp = datasize; outfile.write((char*)(&tmp),4); }
}

void writeMusic(fs::path const& filename, std::vector<short> const& buf, unsigned sr) {
	std::ofstream f(filename.string().c_str(), std::ios::binary);
	writeWavHeader(f, 2, sr, buf.size());
	f.write(reinterpret_cast<char const*>(&buf[0]), buf.size() * sizeof(short));
}

void video_us(Song& song, PakFile const& iavFile, PakFile const& indFile, fs::path const& outPath) {
	// Tracks on my example
	// 0 => video (ipu)
	// 1 and 2 => adpcm song (left/right)
	// 3 and 4 => adpcm vocals (left/right)

	std::vector<char> ipudata;
	std::vector<char> data;
	std::vector<char> ind_file;
	indFile.get(ind_file);

	unsigned int iav_offset = 0;
	unsigned int frame = 0;
	for( unsigned int ind_offset = 0x68 ; ind_offset < ind_file.size() ; ind_offset+=2) {
		unsigned int size = getLE16(&ind_file[ind_offset]) << 4;
		switch(frame % 5) {
		case 0:
			// first 4 bytes are packet length
			iavFile.get(data, iav_offset, size);
			{
				unsigned int consumed = 0;
				while(consumed < size) {
					unsigned int opaque_footer_size = 3 * sizeof(int);
					unsigned int chunk = getLE32(&data[consumed]);
					ipudata.insert(ipudata.end(), data.begin() + 4 + consumed, data.begin() + consumed + chunk - opaque_footer_size);
					consumed += chunk;
				}
			}
			iav_offset += size;
			break;
		case 1:
		case 2:
		case 3:
		case 4:
			// audio
			iav_offset += size;
			break;
		}
		frame++;
	}

	IPUConv(ipudata, (outPath / "video.mpg").string(), song.pal);
	song.video = outPath / "video.mpg";
}

void music_us(Song& song, PakFile const& iavFile, PakFile const& indFile, fs::path const& outPath) {
	// Tracks on my example
	// 0 => video (ipu)
	// 1 and 2 => adpcm song (left/right)
	// 3 and 4 => adpcm vocals (left/right)
	// std::cout << "  >>> IAV file size: " << iavFile.size << std::endl;
	// std::cout << "  >>> IND file size: " << indFile.size << std::endl;

	std::vector<char> ind_file;
	indFile.get(ind_file);
	unsigned int sr = getLE32(&ind_file[0x60]);
	// std::cout << "  >>> sample rate: " << sr << std::endl;

	const unsigned decodeChannels = 4; // Do not change!
	Adpcm adpcm(0, decodeChannels);
	std::vector<short> pcm[2];

	bool karaoke = false;
	unsigned int iav_offset = 0;
	unsigned int frame = 0;
	unsigned int video_size, audio_size = 0;
	for( unsigned int ind_offset = 0x68 ; ind_offset < ind_file.size() ; ind_offset+=2) {
		unsigned int size = getLE16(&ind_file[ind_offset]) << 4;
		switch(frame%5) {
			case 0:
				video_size = size;
				iav_offset += video_size;
				break;
			case 1:
				// song left
				audio_size = size;
				break;
			case 2:
				// song right
				audio_size += size;
				break;
			case 3:
				// vocals left
				audio_size += size;
				break;
			case 4:
				// vocals right
				audio_size += size;
				adpcm.interleave(size);
				for (unsigned pos = 0, end; (end = pos + 2 * adpcm.chunkBytes()) <= audio_size; pos = end) {
					std::vector<char> data;
					iavFile.get(data, iav_offset + pos, end - pos);
					std::vector<short> pcmtmp(adpcm.chunkFrames() * decodeChannels);
					adpcm.decodeChunk(&data[0], pcmtmp.begin());
					for (size_t s = 0; s < pcmtmp.size(); s += 4) {
						short l1 = pcmtmp[s];
						short r1 = pcmtmp[s + 1];
						short l2 = pcmtmp[s + 2];
						short r2 = pcmtmp[s + 3];
						pcm[0].push_back(l1);
						pcm[0].push_back(r1);
						pcm[1].push_back(l2);
						pcm[1].push_back(r2);
						if (l2 != 0 || r2 != 0) karaoke = true;
					}
				}
				iav_offset += audio_size;
				break;
		}
		frame++;
	}
	std::string ext;
	writeMusic(song.music = outPath / ("music.wav"), pcm[0], sr);
	if (karaoke) writeMusic(song.vocals = outPath / ("vocals.wav"), pcm[1], sr);
}

void music(Song& song, PakFile const& dataFile, PakFile const& headerFile, fs::path const& outPath) {
	std::vector<char> data;
	headerFile.get(data);
	unsigned sr = getLE16(&data[12]);
	unsigned interleave = getLE16(&data[16]);
	const unsigned decodeChannels = 4; // Do not change!
	Adpcm adpcm(interleave, decodeChannels);
	std::vector<short> pcm[2];
	bool karaoke = false;
	for (unsigned pos = 0, end; (end = pos + 2 * adpcm.chunkBytes()) <= dataFile.size; pos = end) {
		dataFile.get(data, pos, end - pos);
		std::vector<short> pcmtmp(adpcm.chunkFrames() * decodeChannels);
		adpcm.decodeChunk(&data[0], pcmtmp.begin());
		for (size_t s = 0; s < pcmtmp.size(); s += 4) {
			short l1 = pcmtmp[s];
			short r1 = pcmtmp[s + 1];
			short l2 = pcmtmp[s + 2];
			short r2 = pcmtmp[s + 3];
			pcm[0].push_back(l1);
			pcm[0].push_back(r1);
			pcm[1].push_back(l2);
			pcm[1].push_back(r2);
			if (l2 != 0 || r2 != 0) karaoke = true;
		}
	}
	std::string ext;
	writeMusic(song.music = outPath / ("music.wav"), pcm[0], sr);
	if (karaoke) writeMusic(song.vocals = outPath / ("vocals.wav"), pcm[1], sr);
}

