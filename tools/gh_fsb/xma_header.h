/*
  by Luigi Auriemma
*/

#ifdef WIN32
    #include <windows.h>
#else
    typedef unsigned int    DWORD;
    typedef unsigned short  WORD;
    typedef unsigned char   BYTE;
typedef struct tWAVEFORMATEX {
	WORD wFormatTag;
	WORD nChannels;
	DWORD nSamplesPerSec;
	DWORD nAvgBytesPerSec;
	WORD nBlockAlign;
	WORD wBitsPerSample;
	WORD cbSize;
} WAVEFORMATEX,*PWAVEFORMATEX,*LPWAVEFORMATEX;
#endif

typedef struct XMASTREAMFORMAT
{
    DWORD   PsuedoBytesPerSec;  // Used by encoder
    DWORD   SampleRate;         // Sample rate for the stream.
    DWORD   LoopStart;          // Loop start offset (in bits).
    DWORD   LoopEnd;            // Loop end offset (in bits).

    // Format for SubframeData: eeee ssss.
    // e: Subframe number of loop end point [0,3].
    // s: Number of subframes to skip before decoding and outputting at the loop start point [1,4].

    BYTE    SubframeData;       // Data for decoding subframes.  See above.
    BYTE    Channels;           // Number of channels in the stream (1 or 2).
    WORD    ChannelMask;        // Channel assignments for the channels in the stream (same as
                                // lower 16 bits of dwChannelMask in WAVEFORMATEXTENSIBLE).
} XMASTREAMFORMAT, *PXMASTREAMFORMAT, *NPXMASTREAMFORMAT, *LPXMASTREAMFORMAT;
typedef const XMASTREAMFORMAT *LPCXMASTREAMFORMAT;

typedef struct XMAWAVEFORMAT
{
    WORD            FormatTag;     // Audio format type (always WAVE_FORMAT_XMA).
    WORD            BitsPerSample; // Bit depth (currently required to be 16).
    WORD            EncodeOptions; // Options for XMA encoder/decoder.
    WORD            LargestSkip;   // Largest skip used in interleaving streams.
    WORD            NumStreams;    // Number of interleaved audio streams.
    BYTE            LoopCount;     // Number of loop repetitions (255 == infinite).
    BYTE            Version;       // Version of the encoder that generated this.
    XMASTREAMFORMAT XmaStreams[1]; // Format info for each stream (can grow based on wNumStreams).
} XMAWAVEFORMAT, *PXMAWAVEFORMAT, *NPXMAWAVEFORMAT, *LPXMAWAVEFORMAT;
typedef XMAWAVEFORMAT *LPCXMAWAVEFORMAT;

typedef const WAVEFORMATEX *LPCWAVEFORMATEX;

typedef struct XMA2WAVEFORMATEX
{
    WAVEFORMATEX wfx;
    // Meaning of the WAVEFORMATEX fields here:
    //    wFormatTag;        // Audio format type; always WAVE_FORMAT_XMA2
    //    nChannels;         // Channel count of the decoded audio
    //    nSamplesPerSec;    // Sample rate of the decoded audio
    //    nAvgBytesPerSec;   // Used internally by the XMA encoder
    //    nBlockAlign;       // Decoded sample size; channels * wBitsPerSample / 8
    //    wBitsPerSample;    // Bits per decoded mono sample; always 16 for XMA
    //    cbSize;            // Size in bytes of the rest of this structure (34)

    WORD  NumStreams;        // Number of audio streams (1 or 2 channels each)
    DWORD ChannelMask;       // Spatial positions of the channels in this file,
                             // stored as SPEAKER_xxx values (see audiodefs.h)
    DWORD SamplesEncoded;    // Total number of PCM samples the file decodes to
    DWORD BytesPerBlock;     // XMA block size (but the last one may be shorter)
    DWORD PlayBegin;         // First valid sample in the decoded audio
    DWORD PlayLength;        // Length of the valid part of the decoded audio
    DWORD LoopBegin;         // Beginning of the loop region in decoded sample terms
    DWORD LoopLength;        // Length of the loop region in decoded sample terms
    BYTE  LoopCount;         // Number of loop repetitions; 255 = infinite
    BYTE  EncoderVersion;    // Version of XMA encoder that generated the file
    WORD  BlockCount;        // XMA blocks in file (and entries in its seek table)
} XMA2WAVEFORMATEX, *PXMA2WAVEFORMATEX;

typedef struct XMA2STREAMFORMAT
{
    BYTE Channels;           // Number of channels in the stream (1 or 2)
    BYTE RESERVED;           // Reserved for future use
    WORD ChannelMask;        // Spatial positions of the channels in the stream
} XMA2STREAMFORMAT;

// Legacy XMA2 format structure (big-endian byte ordering)
typedef struct XMA2WAVEFORMAT
{
    BYTE  Version;           // XMA encoder version that generated the file.
                             // Always 3 or higher for XMA2 files.
    BYTE  NumStreams;        // Number of interleaved audio streams
    BYTE  RESERVED;          // Reserved for future use
    BYTE  LoopCount;         // Number of loop repetitions; 255 = infinite
    DWORD LoopBegin;         // Loop begin point, in samples
    DWORD LoopEnd;           // Loop end point, in samples
    DWORD SampleRate;        // The file's decoded sample rate
    DWORD EncodeOptions;     // Options for the XMA encoder/decoder
    DWORD PsuedoBytesPerSec; // Used internally by the XMA encoder
    DWORD BlockSizeInBytes;  // Size in bytes of this file's XMA blocks (except
                             // possibly the last one).  Always a multiple of
                             // 2Kb, since XMA blocks are arrays of 2Kb packets.
    DWORD SamplesEncoded;    // Total number of PCM samples encoded in this file
    DWORD SamplesInSource;   // Actual number of PCM samples in the source
                             // material used to generate this file
    DWORD BlockCount;        // Number of XMA blocks in this file (and hence
                             // also the number of entries in its seek table)
    XMA2STREAMFORMAT Streams[1]; // Per-stream format information; the actual
                                 // array length is in the NumStreams field.
} XMA2WAVEFORMAT;



inline unsigned xma_le16(unsigned n) {
    static const int    endianess = 1;
    if(*(char *)&endianess) return(n);
    n = (((n & 0xff00) >> 8) |
         ((n & 0x00ff) << 8));
    return(n);
}

inline unsigned xma_le32(unsigned n) {
    static const int    endianess = 1;
    if(*(char *)&endianess) return(n);
    n = (((n & 0xff000000) >> 24) |
         ((n & 0x00ff0000) >>  8) |
         ((n & 0x0000ff00) <<  8) |
         ((n & 0x000000ff) << 24));
    return(n);
}

inline unsigned xma_be16(unsigned n) {
    static const int    endianess = 1;
    if(!*(char *)&endianess) return(n);
    n = (((n & 0xff00) >> 8) |
         ((n & 0x00ff) << 8));
    return(n);
}

inline unsigned xma_be32(unsigned n) {
    static const int    endianess = 1;
    if(!*(char *)&endianess) return(n);
    n = (((n & 0xff000000) >> 24) |
         ((n & 0x00ff0000) >>  8) |
         ((n & 0x0000ff00) <<  8) |
         ((n & 0x000000ff) << 24));
    return(n);
}

int xma_quick_mask(int chans) { // made on the fly, not important and probably wrong
    int     i,
            mask;

    //return(0x80000000);   // SPEAKER_ALL
    mask = 0;
    for(i = 0; i < chans; i++) {
        mask = 1 << 1;
    }
    return(mask);
}



// XMA1:
//  fmt
//  data
//  seek

int xma1_header(FILE *fd, int freq, int chans, int bits, int rawlen, unsigned char *seek, int seeklen, int samples) {
    XMAWAVEFORMAT   fmt;
    mywav_chunk     chunk;

    if(freq  <= 0) freq  = 44100;
    if(chans <= 0) chans = 1;
    if(bits  <= 0) bits  = 16;
    //if(!seek || (seeklen <= 0)) { seek = &samples; seeklen = 4; }

    fmt.FormatTag                       = xma_le16(0x0165);
    fmt.BitsPerSample                   = xma_le16(bits);
    fmt.EncodeOptions                   = xma_le16(0x10d6);
    fmt.LargestSkip                     = xma_le16(0);
    fmt.NumStreams                      = xma_le16(1);
    fmt.LoopCount                       = 0;
    fmt.Version                         = 2;

    fmt.XmaStreams->PsuedoBytesPerSec   = xma_le32(rawlen); // used only by the encoder
    fmt.XmaStreams->SampleRate          = xma_le32(freq);
    fmt.XmaStreams->LoopStart           = xma_le32(0);
    fmt.XmaStreams->LoopEnd             = xma_le32(0);
    fmt.XmaStreams->SubframeData        = 0;
    fmt.XmaStreams->Channels            = chans;
    fmt.XmaStreams->ChannelMask         = xma_le16(xma_quick_mask(chans));

    memcpy(chunk.id, "RIFF", 4);
    chunk.size =
        4 + sizeof(mywav_chunk)             // RIFF
        + sizeof(fmt)                       // fmt
        + sizeof(mywav_chunk) + rawlen      // data
        + sizeof(mywav_chunk) + seeklen;    // seek

    if(mywav_fwchunk(fd, &chunk)      < 0) return(-1);
    if(mywav_fwmem(fd, "WAVE", 4)     < 0) return(-1);

    memcpy(chunk.id, "fmt ", 4);
    chunk.size = sizeof(fmt) + seeklen;
    if(mywav_fwchunk(fd, &chunk)      < 0) return(-1);
    //if(mywav_fwfmtchunk(fd, &fmt)     < 0) return(-1);
    if(fwrite(&fmt, 1, sizeof(fmt), fd) != sizeof(fmt)) return(-1);

    memcpy(chunk.id, "seek", 4);
    chunk.size = seeklen;
    if(mywav_fwchunk(fd, &chunk)      < 0) return(-1);
    if(fwrite(seek, 1, seeklen, fd) != seeklen) return(-1);

    // data must be placed at the end so that the main tool can write the data after it
    memcpy(chunk.id, "data", 4);
    chunk.size = rawlen;
    if(mywav_fwchunk(fd, &chunk)      < 0) return(-1);
    return(0);
}



// XMA2:
//  fmt
//  data
//  seek

int xma2_header(FILE *fd, int freq, int chans, int bits, int rawlen, unsigned char *seek, int seeklen, int samples) {
    XMA2WAVEFORMATEX    fmt;
    mywav_chunk     chunk;

    if(freq  <= 0) freq  = 44100;
    if(chans <= 0) chans = 1;
    if(bits  <= 0) bits  = 16;
    //if(!seek || (seeklen <= 0)) { seek = &samples; seeklen = 4; }

    fmt.wfx.wFormatTag          = xma_le16(0x0166);
	fmt.wfx.nChannels           = xma_le16(chans);
	fmt.wfx.nSamplesPerSec      = xma_le32(freq);
	fmt.wfx.nAvgBytesPerSec     = xma_le32(rawlen); // used only by the encoder
	fmt.wfx.nBlockAlign         = xma_le16(4);
	fmt.wfx.wBitsPerSample      = xma_le16(bits);
	fmt.wfx.cbSize              = xma_le16(34);

    fmt.NumStreams              = xma_le16(1);
    fmt.ChannelMask             = xma_le32(xma_quick_mask(chans));
    fmt.SamplesEncoded          = xma_le32(samples);
    fmt.BytesPerBlock           = xma_le32(0x10000);
    fmt.PlayBegin               = xma_le32(0);
    fmt.PlayLength              = xma_le32(samples);
    fmt.LoopBegin               = xma_le32(0);
    fmt.LoopLength              = xma_le32(0);
    fmt.LoopCount               = 0;
    fmt.EncoderVersion          = 3;    // or 4
    fmt.BlockCount              = xma_le16(1); 

    memcpy(chunk.id, "RIFF", 4);
    chunk.size =
        4 + sizeof(mywav_chunk)             // RIFF
        + sizeof(fmt)                       // fmt
        + sizeof(mywav_chunk) + rawlen      // data
        + sizeof(mywav_chunk) + seeklen;    // seek

    if(mywav_fwchunk(fd, &chunk)      < 0) return(-1);
    if(mywav_fwmem(fd, "WAVE", 4)     < 0) return(-1);

    memcpy(chunk.id, "fmt ", 4);
    chunk.size = sizeof(fmt) + seeklen;
    if(mywav_fwchunk(fd, &chunk)      < 0) return(-1);
    //if(mywav_fwfmtchunk(fd, &fmt)     < 0) return(-1);
    if(fwrite(&fmt, 1, sizeof(fmt), fd) != sizeof(fmt)) return(-1);

    memcpy(chunk.id, "seek", 4);
    chunk.size = seeklen;
    if(mywav_fwchunk(fd, &chunk)      < 0) return(-1);
    if(fwrite(seek, 1, seeklen, fd) != seeklen) return(-1);

    // data must be placed at the end so that the main tool can write the data after it
    memcpy(chunk.id, "data", 4);
    chunk.size = rawlen;
    if(mywav_fwchunk(fd, &chunk)      < 0) return(-1);
    return(0);
}



// XMA2xact:
//  data
//  XMA2
//  seek

int xma2xact_header(FILE *fd, int freq, int chans, int bits, int rawlen, unsigned char *seek, int seeklen, int samples) {
    XMA2WAVEFORMAT  fmt;
    mywav_chunk     chunk;

    if(freq  <= 0) freq  = 44100;
    if(chans <= 0) chans = 1;
    if(bits  <= 0) bits  = 16;
    //if(!seek || (seeklen <= 0)) { seek = &samples; seeklen = 4; }

    fmt.Version                 = 3;    // or 4
    fmt.NumStreams              = 1;
    fmt.RESERVED                = 0;
    fmt.LoopCount               = 255;
    fmt.LoopBegin               = xma_be32(0);
    fmt.LoopEnd                 = xma_be32(samples);
    fmt.SampleRate              = xma_be32(freq);
    fmt.EncodeOptions           = xma_be32(0x10d6);
    fmt.PsuedoBytesPerSec       = xma_be32(rawlen); // used only by the encoder
    fmt.BlockSizeInBytes        = xma_be32(0x10000);
    fmt.SamplesEncoded          = xma_be32(samples);
    fmt.SamplesInSource         = xma_be32(samples);
    fmt.BlockCount              = xma_be32(1);

    fmt.Streams->Channels       = chans;
    fmt.Streams->RESERVED       = 0;
    fmt.Streams->ChannelMask    = xma_be16(xma_quick_mask(chans));

    memcpy(chunk.id, "RIFF", 4);
    chunk.size =
        4 + sizeof(mywav_chunk)             // RIFF
        + sizeof(fmt)                       // fmt
        + sizeof(mywav_chunk) + rawlen      // data
        + sizeof(mywav_chunk) + seeklen;    // seek

    if(mywav_fwchunk(fd, &chunk)      < 0) return(-1);
    if(mywav_fwmem(fd, "WAVE", 4)     < 0) return(-1);

    memcpy(chunk.id, "XMA2", 4);
    chunk.size = sizeof(fmt) + seeklen;
    if(mywav_fwchunk(fd, &chunk)      < 0) return(-1);
    //if(mywav_fwfmtchunk(fd, &fmt)     < 0) return(-1);
    if(fwrite(&fmt, 1, sizeof(fmt), fd) != sizeof(fmt)) return(-1);

    memcpy(chunk.id, "seek", 4);
    chunk.size = seeklen;
    if(mywav_fwchunk(fd, &chunk)      < 0) return(-1);
    if(fwrite(seek, 1, seeklen, fd) != seeklen) return(-1);

    // data must be placed at the end so that the main tool can write the data after it
    memcpy(chunk.id, "data", 4);
    chunk.size = rawlen;
    if(mywav_fwchunk(fd, &chunk)      < 0) return(-1);
    return(0);
}


