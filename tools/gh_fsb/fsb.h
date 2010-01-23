/*
from http://www.fmod.org/forum/viewtopic.php?t=1551
*/

#pragma pack(2)

#define F_FLOAT     float

#define FSOUND_FSB_NAMELEN  30

/*
    Defines for FSOUND_FSB_HEADER->mode field
*/
#define FSOUND_FSB_SOURCE_FORMAT        0x00000001  /* all samples stored in their original compressed format */
#define FSOUND_FSB_SOURCE_BASICHEADERS  0x00000002  /* samples should use the basic header structure */

/*
    Defines for FSOUND_FSB_HEADER->version field
*/
#define FSOUND_FSB_VERSION_3_1          0x00030001  /* FSB version 3.1 */

/*
    16 byte header.
*/
typedef struct {
    char        id[4];      /* 'FSB1' */
    int32_t     numsamples; /* number of samples in the file */
    int32_t     datasize;   /* size in bytes of compressed sample data */
    int32_t     dunno_null;
} FSOUND_FSB_HEADER_FSB1;

/*
    16 byte header.
*/
typedef struct {
    char        id[4];      /* 'FSB2' */
    int32_t     numsamples; /* number of samples in the file */
    int32_t     shdrsize;   /* size in bytes of all of the sample headers including extended information */
    int32_t     datasize;   /* size in bytes of compressed sample data */
} FSOUND_FSB_HEADER_FSB2;

/*
    24 byte header.
*/
typedef struct {
    char        id[4];      /* 'FSB3' */
    int32_t     numsamples; /* number of samples in the file */
    int32_t     shdrsize;   /* size in bytes of all of the sample headers including extended information */
    int32_t     datasize;   /* size in bytes of compressed sample data */
    uint32_t    version;    /* extended fsb version */
    uint32_t    mode;       /* flags that apply to all samples in the fsb */
} FSOUND_FSB_HEADER_FSB3;

typedef struct {
    char        id[4];      /* 'FSB4' */
    int32_t     numsamples; /* number of samples in the file */
    int32_t     shdrsize;   /* size in bytes of all of the sample headers including extended information */
    int32_t     datasize;   /* size in bytes of compressed sample data */
    uint32_t    version;    /* extended fsb version */
    uint32_t    mode;       /* flags that apply to all samples in the fsb */
    char        zero[8];    /* ??? */
    uint8_t     hash[16];   /* hash??? */
} FSOUND_FSB_HEADER_FSB4;

/*
    64 byte sample header.
*/
typedef struct {
    char        name[32];

    uint32_t    lengthsamples;
    uint32_t    lengthcompressedbytes;
    int32_t     deffreq;
    uint16_t    defpri;
    uint16_t    numchannels;    /* I'm not sure! */
    uint16_t    defvol;
    int16_t     defpan;
    uint32_t    mode;
    uint32_t    loopstart;
    uint32_t    loopend;
   
} FSOUND_FSB_SAMPLE_HEADER_1;

/*
    64 byte sample header.
*/
typedef struct {
    uint16_t    size;
    char        name[FSOUND_FSB_NAMELEN];

    uint32_t    lengthsamples;
    uint32_t    lengthcompressedbytes;
    uint32_t    loopstart;
    uint32_t    loopend;

    uint32_t    mode;
    int32_t     deffreq;
    uint16_t    defvol;
    int16_t     defpan;
    uint16_t    defpri;
    uint16_t    numchannels;
   
} FSOUND_FSB_SAMPLE_HEADER_2;

/*
    80 byte sample header.
*/
typedef struct {
    uint16_t    size;
    char        name[FSOUND_FSB_NAMELEN];

    uint32_t    lengthsamples;
    uint32_t    lengthcompressedbytes;
    uint32_t    loopstart;
    uint32_t    loopend;

    uint32_t    mode;
    int32_t     deffreq;
    uint16_t    defvol;
    int16_t     defpan;
    uint16_t    defpri;
    uint16_t    numchannels;

    F_FLOAT     mindistance;
    F_FLOAT     maxdistance;
    int32_t     varfreq;
    uint16_t    varvol;
    int16_t     varpan;

} FSOUND_FSB_SAMPLE_HEADER_3_1;

/*
    8 byte sample header.
*/
typedef struct {
    uint32_t    lengthsamples;
    uint32_t    lengthcompressedbytes;   
} FSOUND_FSB_SAMPLE_HEADER_BASIC; 



#define FSOUND_LOOP_OFF         0x00000001 /* For non looping samples. */
#define FSOUND_LOOP_NORMAL      0x00000002 /* For forward looping samples. */
#define FSOUND_LOOP_BIDI        0x00000004 /* For bidirectional looping samples. (no effect if in hardware). */
#define FSOUND_8BITS            0x00000008 /* For 8 bit samples. */
#define FSOUND_16BITS           0x00000010 /* For 16 bit samples. */
#define FSOUND_MONO             0x00000020 /* For mono samples. */
#define FSOUND_STEREO           0x00000040 /* For stereo samples. */
#define FSOUND_UNSIGNED         0x00000080 /* For user created source data containing unsigned samples. */
#define FSOUND_SIGNED           0x00000100 /* For user created source data containing signed data. */
#define FSOUND_DELTA            0x00000200 /* For user created source data stored as delta values. */
#define FSOUND_IT214            0x00000400 /* For user created source data stored using IT214 compression. */
#define FSOUND_IT215            0x00000800 /* For user created source data stored using IT215 compression. */
#define FSOUND_HW3D             0x00001000 /* Attempts to make samples use 3d hardware acceleration. (if the card supports it) */
#define FSOUND_2D               0x00002000 /* Tells software (not hardware) based sample not to be included in 3d processing. */
#define FSOUND_STREAMABLE       0x00004000 /* For a streamimg sound where you feed the data to it. */
#define FSOUND_LOADMEMORY       0x00008000 /* "name" will be interpreted as a pointer to data for streaming and samples. */
#define FSOUND_LOADRAW          0x00010000 /* Will ignore file format and treat as raw pcm. */
#define FSOUND_MPEGACCURATE     0x00020000 /* For FSOUND_Stream_Open - for accurate FSOUND_Stream_GetLengthMs/FSOUND_Stream_SetTime. WARNING, see FSOUND_Stream_Open for inital opening time performance issues. */
#define FSOUND_FORCEMONO        0x00040000 /* For forcing stereo streams and samples to be mono - needed if using FSOUND_HW3D and stereo data - incurs a small speed hit for streams */
#define FSOUND_HW2D             0x00080000 /* 2D hardware sounds. allows hardware specific effects */
#define FSOUND_ENABLEFX         0x00100000 /* Allows DX8 FX to be played back on a sound. Requires DirectX 8 - Note these sounds cannot be played more than once, be 8 bit, be less than a certain size, or have a changing frequency */
#define FSOUND_MPEGHALFRATE     0x00200000 /* For FMODCE only - decodes mpeg streams using a lower quality decode, but faster execution */
#define FSOUND_IMAADPCM         0x00400000 /* Contents are stored compressed as IMA ADPCM */
#define FSOUND_VAG              0x00800000 /* For PS2 only - Contents are compressed as Sony VAG format */
//#define FSOUND_NONBLOCKING      0x01000000 /* For FSOUND_Stream_Open/FMUSIC_LoadSong - Causes stream or music to open in the background and not block the foreground app. See FSOUND_Stream_GetOpenState or FMUSIC_GetOpenState to determine when it IS ready. */
#define FSOUND_XMA              0x01000000
#define FSOUND_GCADPCM          0x02000000 /* For Gamecube only - Contents are compressed as Gamecube DSP-ADPCM format */
#define FSOUND_MULTICHANNEL     0x04000000 /* For PS2 and Gamecube only - Contents are interleaved into a multi-channel (more than stereo) format */
#define FSOUND_USECORE0         0x08000000 /* For PS2 only - Sample/Stream is forced to use hardware voices 00-23 */
#define FSOUND_USECORE1         0x10000000 /* For PS2 only - Sample/Stream is forced to use hardware voices 24-47 */
#define FSOUND_LOADMEMORYIOP    0x20000000 /* For PS2 only - "name" will be interpreted as a pointer to data for streaming and samples. The address provided will be an IOP address */
#define FSOUND_IGNORETAGS       0x40000000 /* Skips id3v2 etc tag checks when opening a stream, to reduce seek/read overhead when opening files (helps with CD performance) */
#define FSOUND_STREAM_NET       0x80000000 /* Specifies an internet stream */
#define FSOUND_NORMAL           (FSOUND_16BITS | FSOUND_SIGNED | FSOUND_MONO)

#pragma pack()
