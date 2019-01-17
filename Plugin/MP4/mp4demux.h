/** 20.09.2009 @file
*   ISO MP4 file parsing
*
*   Portability note: this module uses:
*   - Dynamic memory allocation (malloc(), realloc() and free()
*   - Direct file access (fgetc(), fread() & fseek())
*   - File size (fstat())
*
*   This module provide functions to decode mp4 indexes, and retrieve
*   file position and size for each sample in given track.
*
*   Integration scenario example:
*
*   // 1. Parse MP4 structure
*   if (MP4D__open(mp4, file_handle))
*   {
*       // 2. Find tracks, supported by the application
*       for (i = 0; i < mp4->track_count; i++)
*       {
*           if (supported(mp4->track[i].object_type_indication))
*           {
*               // 3. Initialize decoder from transparent 'Decoder Specific Info' data
*               init_decoder(mp4->track[i].dsi);
*
*               // 4. Read track data for each frame
*               for (k = 0; k < mp4->track[i].sample_count; k++)
*               {
*                   fseek(file_handle, MP4D__frame_offset(mp4, i, k, &frame_size, NULL, NULL), SEEK_SET);
*                   fread(buf, frame_size, 1, file_handle);
*                   decode(buf);
*               }
*           }
*       }
*   }
*
*/

#ifndef mp4demux_H_INCLUDED
#define mp4demux_H_INCLUDED

#include <stdio.h>
#include "mp4defs.h"

#ifdef __cplusplus
extern "C" {
#endif  //__cplusplus


/************************************************************************/
/*                  Portable 64-bit type definition                     */
/************************************************************************/

#if (defined(__GNUC__) && __GNUC__ >= 4) || (defined __STDC_VERSION__ && __STDC_VERSION__ >= 199901)
#   include <stdint.h>  // hope that all GCC compilers support this C99 extension
    typedef uint64_t mp4d_size_t;
#else
#   if defined (_MSC_VER)
    typedef unsigned __int64    mp4d_size_t;
#   else
    typedef unsigned long long  mp4d_size_t;
#   endif
    typedef unsigned int        uint32_t;
#endif



typedef struct 
{
    unsigned         first_chunk;
    unsigned         samples_per_chunk;
} MP4D_sample_to_chunk_t;


typedef struct
{
    /************************************************************************/
    /*                 mandatory public data                                */
    /************************************************************************/
    // How many 'samples' in the track
    // The 'sample' is MP4 term, denoting audio or video frame
    unsigned sample_count;

    // Decoder-specific info (DSI) data
    unsigned char * dsi;

    // DSI data size
    unsigned dsi_bytes;

    // MP4 object type code
    // case 0x00: return "Forbidden";
    // case 0x01: return "Systems ISO/IEC 14496-1";
    // case 0x02: return "Systems ISO/IEC 14496-1";
    // case 0x20: return "Visual ISO/IEC 14496-2";
    // case 0x40: return "Audio ISO/IEC 14496-3";
    // case 0x60: return "Visual ISO/IEC 13818-2 Simple Profile";
    // case 0x61: return "Visual ISO/IEC 13818-2 Main Profile";
    // case 0x62: return "Visual ISO/IEC 13818-2 SNR Profile";
    // case 0x63: return "Visual ISO/IEC 13818-2 Spatial Profile";
    // case 0x64: return "Visual ISO/IEC 13818-2 High Profile";
    // case 0x65: return "Visual ISO/IEC 13818-2 422 Profile";
    // case 0x66: return "Audio ISO/IEC 13818-7 Main Profile";
    // case 0x67: return "Audio ISO/IEC 13818-7 LC Profile";
    // case 0x68: return "Audio ISO/IEC 13818-7 SSR Profile";
    // case 0x69: return "Audio ISO/IEC 13818-3";
    // case 0x6A: return "Visual ISO/IEC 11172-2";
    // case 0x6B: return "Audio ISO/IEC 11172-3";
    // case 0x6C: return "Visual ISO/IEC 10918-1";
    unsigned object_type_indication;


    /************************************************************************/
    /*                 informational public data, not needed for decoding   */
    /************************************************************************/
    // handler_type when present in a media box, is an integer containing one of
    // the following values, or a value from a derived specification:
    // 'vide' Video track
    // 'soun' Audio track
    // 'hint' Hint track
    unsigned handler_type;

    // Track duration: 64-bit value split into 2 variables
    unsigned duration_hi;
    unsigned duration_lo;

    // duration scale: duration = timescale*seconds
    unsigned timescale;

    // Average bitrate, bits per second
    unsigned avg_bitrate_bps;

    // Track language: 3-char ISO 639-2T code: "und", "eng", "rus", "jpn" etc...
    unsigned char language[4];

    // MP4 stream type
    // case 0x00: return "Forbidden";
    // case 0x01: return "ObjectDescriptorStream";
    // case 0x02: return "ClockReferenceStream";
    // case 0x03: return "SceneDescriptionStream";
    // case 0x04: return "VisualStream";
    // case 0x05: return "AudioStream";
    // case 0x06: return "MPEG7Stream";
    // case 0x07: return "IPMPStream";
    // case 0x08: return "ObjectContentInfoStream";
    // case 0x09: return "MPEGJStream";
    unsigned stream_type;

    union
    {
        // for handler_type == 'soun' tracks
        struct
        {
            unsigned channelcount;
            unsigned samplerate_hz;
        } audio;

        // for handler_type == 'vide' tracks
        struct
        {
            unsigned width;
            unsigned height;
        } video;
    } SampleDescription;

    /************************************************************************/
    /*                 private data: MP4 indexes                            */
    /************************************************************************/
    unsigned *entry_size;   // [sample_count]
    unsigned *timestamp;    // [sample_count]
    unsigned *duration;     // [sample_count]

    unsigned sample_to_chunk_count;
    MP4D_sample_to_chunk_t * sample_to_chunk;    // [sample_to_chunk_count]

    unsigned chunk_count;
    mp4d_size_t * chunk_offset;  // [chunk_count]

} MP4D_track_t;


typedef struct MP4D_demux_tag
{
    /************************************************************************/
    /*                 mandatory public data                                */
    /************************************************************************/
    // number of tracks in the movie
    unsigned track_count;

    // track data (public/private)
    MP4D_track_t * track;

    /************************************************************************/
    /*                 informational public data, not needed for decoding   */
    /************************************************************************/
    // Movie duration: 64-bit value split into 2 variables
    unsigned duration_hi;
    unsigned duration_lo;

    // duration scale: duration = timescale*seconds
    unsigned timescale;

    // Metadata tag (optional)
    // Tags provided 'as-is', without any re-encoding
    struct
    {
        unsigned char *title;
        unsigned char *artist;
        unsigned char *album;
        unsigned char *year;
        unsigned char *comment;
        unsigned char *genre;
    } tag;

} MP4D_demux_t;


/**
*   Parse given file as MP4 file.  Allocate and store data indexes.
*   return 1 on success, 0 on failure
*   Given file rewind()'ed on return.
*   The MP4 indexes may be stored at the end of file, so this
*   function may parse all file, using fseek().
*   It is guaranteed that function will read/seek the file sequentially,
*   and will never jump back.
*/
int MP4D__open(MP4D_demux_t * mp4, FILE * f);


/**
*   Return position and size for given sample from given track. The 'sample' is a
*   MP4 term for 'frame'
*   
*   frame_bytes [OUT]   - return coded frame size in bytes
*   timestamp [OUT]     - return frame timestamp (in mp4->timescale units)
*   duration [OUT]      - return frame duration (in mp4->timescale units)
*
*   function return file offset for the frame
*/
mp4d_size_t MP4D__frame_offset(const MP4D_demux_t * mp4, unsigned int ntrack, unsigned int nsample, unsigned int * frame_bytes, unsigned * timestamp, unsigned * duration);


/**
*   De-allocated memory
*/
void MP4D__close(MP4D_demux_t * mp4);


/**
*   Helper functions to parse mp4.track[ntrack].dsi for H.264 SPS/PPS
*   Return pointer to internal mp4 memory, it must not be free()-ed
*   
*   Example: process all SPS in MP4 file:
*       while (sps = MP4D__read_sps(mp4, num_of_avc_track, sps_count, &sps_bytes))
*       {
*           process(sps, sps_bytes);
*           sps_count++;
*       }
*/
const unsigned char * MP4D__read_sps(const MP4D_demux_t * mp4, unsigned int ntrack, int nsps, int * sps_bytes);
const unsigned char * MP4D__read_pps(const MP4D_demux_t * mp4, unsigned int ntrack, int npps, int * pps_bytes);



/**
*   Decode MP4D_track_t::stream_type to ASCII string
*/
const char * MP4D__stream_type_to_ascii(int stream_type);


/**
*   Decode MP4D_track_t::object_type_indication to ASCII string
*/
const char * MP4D__object_type_to_ascii(int object_type_indication);


#ifdef __cplusplus
}
#endif //__cplusplus

#endif //mp4demux_H_INCLUDED
