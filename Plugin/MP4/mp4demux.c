/** 20.09.2009 ASP @file
*
*/

#include "mp4demux.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <limits.h>     // LONG_MAX
#include <sys/types.h>  // struct stat
#include <sys/stat.h>   // fstat       - for file size

/************************************************************************/
/*      Build config                                                    */
/************************************************************************/
// Max chunks nesting level
#define MP4D_MAX_CHUNKS_DEPTH            64

// Debug trace
#ifndef MP4D_DEBUG_TRACE
#   define MP4D_DEBUG_TRACE     0
#endif
#if MP4D_DEBUG_TRACE
#   define MP4D_TRACE(x) printf x
#else
#   define MP4D_TRACE(x)
#endif

// Box type: ATOM box, or 'Object Descriptor' box inside the atom.
typedef enum {BOX_ATOM, BOX_OD} mp4d_boxtype_t;


/************************************************************************/
/*      File input (non-portable) stuff                                 */
/************************************************************************/

/**
*   Return 64-bit file size in most portable way
*/
static off_t mp4d_fsize(FILE * f) 
{
    struct stat st;
#ifdef _MSC_VER
    if (fstat(_fileno(f), &st) == 0)
#else
    if (fstat(fileno(f), &st) == 0)
#endif
    {
        return st.st_size;
    }
    return -1;
}

/**
*   Read given number of bytes from the file
*   Used to read box headers
*/
static unsigned mp4d_read(FILE * f, int nb, int * eof_flag)
{
    uint32_t v = 0; int last_byte;
    switch (nb)
    {
    case 4: v = (v << 8) | fgetc(f);
    case 3: v = (v << 8) | fgetc(f);
    case 2: v = (v << 8) | fgetc(f);
    default:
    case 1: v = (v << 8) | (last_byte = fgetc(f));
    }
    if (last_byte == EOF)
    {
        *eof_flag = 1;
    }
    return v;
}

/**
*   Read given number of bytes, but no more than *payload_bytes specifies...
*   Used to read box payload
*/
static uint32_t mp4d_read_payload(FILE * f, unsigned nb, mp4d_size_t * payload_bytes, int * eof_flag)
{
    if (*payload_bytes < nb)
    {
        *eof_flag = 1;
        nb = (int)*payload_bytes;
    }
    *payload_bytes -= nb;

    return mp4d_read(f, nb, eof_flag);
}

/**
*   Skips given number of bytes.
*/
static void mp4d_skip_bytes(FILE * f, mp4d_size_t skip, int * eof_flag)
{
    while (skip > 0)
    {
        long lpos = (long)(skip < (mp4d_size_t)LONG_MAX ? skip : LONG_MAX);
        if (fseek(f, lpos, SEEK_CUR))
        {
            *eof_flag = 1;
            return;
        }
        skip -= lpos;
    }
}


#define READ(n) mp4d_read_payload(f, n, &payload_bytes, &eof_flag)
#define SKIP(n) {mp4d_size_t t = payload_bytes < (n) ? payload_bytes : (n); mp4d_skip_bytes(f, t, &eof_flag); payload_bytes -= t;}
#define MP4D_MALLOC(p, size) p = malloc(size); if (!(p)) {MP4D_ERROR("out of memory");}
#define MP4D_REALLOC(p, size) {void * r = realloc(p, size); if (!(r)) {MP4D_ERROR("out of memory");} else p = r;};

/*
*   On error: release resources, rewind the file.
*/
#define MP4D_RETURN_ERROR(mess) {       \
    MP4D_TRACE(("\nMP4 ERROR: " mess)); \
    fseek(f, 0, SEEK_SET);              \
    MP4D__close(mp4);                   \
    return 0;                           \
}

/*
*   Any errors, occurred on top-level hierarchy is passed to exit check: 'if (!mp4->track_count) ... '
*/
#define MP4D_ERROR(mess)            \
    if (!depth)                     \
        break;                      \
    else                            \
        MP4D_RETURN_ERROR(mess);



/************************************************************************/
/*      Exported API functions                                          */
/************************************************************************/

/**
*   Parse given file as MP4 file.  Allocate and store data indexes.
*/
int MP4D__open(MP4D_demux_t * mp4, FILE * f)
{
    int depth = 0;              // box stack size

    // box stack
    struct
    {
        // remaining bytes for box in the stack
        mp4d_size_t bytes;

        // kind of box children's: OD chunks handled in the same manner as name chunks
        mp4d_boxtype_t format;

    } stack[MP4D_MAX_CHUNKS_DEPTH];

    off_t file_size = mp4d_fsize(f);
    int eof_flag = 0;
    unsigned i;
    MP4D_track_t * tr = NULL;
    int read_hdlr = 0;

#if MP4D_DEBUG_TRACE
    // path of current element: List0/List1/... etc
    uint32_t box_path[MP4D_MAX_CHUNKS_DEPTH];
#endif

    if (!f || !mp4)
    {
        MP4D_TRACE(("\nERROR: invlaid arguments!"));
        return 0;
    }

    if (fseek(f, 0, SEEK_SET))  // some platforms missing rewind()
    {
        return 0;
    }

    memset(mp4, 0, sizeof(MP4D_demux_t));

    stack[0].format = BOX_ATOM;   // start with atom box
    stack[0].bytes = 0;           // never accessed

    do
    {
        // List of boxes, derived from 'FullBox'
        //                ~~~~~~~~~~~~~~~~~~~~~
        // need read version field and check version for these boxes
        static const struct
        {
            uint32_t name;
            unsigned max_version;
            unsigned use_track_flag;
        } g_fullbox[] =
        {
            {BOX_mdhd, 1, 1},
            {BOX_mvhd, 1, 0},
            {BOX_hdlr, 0, 0},
            {BOX_meta, 0, 0},
            {BOX_stts, 0, 0},
            {BOX_ctts, 0, 0},
            {BOX_stz2, 0, 1},
            {BOX_stsz, 0, 1},
            {BOX_stsc, 0, 1},
            {BOX_stco, 0, 1},
            {BOX_co64, 0, 1},
            {BOX_stsd, 0, 0},
            {BOX_esds, 0, 1}    // esds does not use track, but switches to OD mode. Check here, to avoid OD check
        };

        // List of boxes, which contains other boxes ('envelopes')
        // Parser will descend down for boxes in this list, otherwise parsing will proceed to 
        // the next sibling box
        // OD boxes handled in the same way as atom boxes...
        static const struct
        {
            uint32_t name;
            mp4d_boxtype_t type;
        } g_envelope_box[] =
        {
            {BOX_esds, BOX_OD},     // TODO: BOX_esds can be used for both audio and video, but this code supports audio only!
            {OD_ESD,   BOX_OD},
            {OD_DCD,   BOX_OD},
            {OD_DSI,   BOX_OD},
            {BOX_trak, BOX_ATOM},
            {BOX_moov, BOX_ATOM},
            {BOX_mdia, BOX_ATOM},
            {BOX_tref, BOX_ATOM},
            {BOX_minf, BOX_ATOM},
            {BOX_dinf, BOX_ATOM},
            {BOX_stbl, BOX_ATOM},
            {BOX_stsd, BOX_ATOM},
            {BOX_mp4a, BOX_ATOM},
            {BOX_mp4s, BOX_ATOM},
            {BOX_mp4v, BOX_ATOM},
            {BOX_avc1, BOX_ATOM},
            {BOX_udta, BOX_ATOM},
            {BOX_meta, BOX_ATOM},
            {BOX_ilst, BOX_ATOM}
        };

        uint32_t FullAtomVersionAndFlags = 0;
        mp4d_size_t payload_bytes;
        mp4d_size_t box_bytes;
        uint32_t box_name;
        unsigned char ** ptag = NULL;
        int read_bytes = 0;

        // Read header box type and it's length
        if (stack[depth].format == BOX_ATOM)
        {
            box_bytes = mp4d_read(f, 4, &eof_flag);
            if (eof_flag)
            {
                break;  // normal exit
            }

            if (box_bytes >= 2 && box_bytes < 8)
            {
                MP4D_ERROR("invalid box size (broken file?)");
            }

            box_name  = mp4d_read(f, 4, &eof_flag);
            read_bytes = 8;

            // Decode box size
            if (box_bytes == 0 ||                         // standard indication of 'till eof' size
                box_bytes == (mp4d_size_t)0xFFFFFFFFU       // some files uses non-standard 'till eof' signaling
                )
            {
                box_bytes = ~(mp4d_size_t)0;
            }

            payload_bytes = box_bytes - 8;

            if (box_bytes == 1)           // 64-bit sizes
            {
                box_bytes = mp4d_read(f, 4, &eof_flag);
                box_bytes <<= 32;
                box_bytes |= mp4d_read(f, 4, &eof_flag);
                if (box_bytes < 16)
                {
                    MP4D_ERROR("invalid box size (broken file?)");
                }
                payload_bytes = box_bytes - 16;
            }

            // Read and check box version for some boxes
            for (i = 0; i < sizeof(g_fullbox)/sizeof(g_fullbox[0]); i++)
            {
                if (box_name == g_fullbox[i].name)
                {
                    FullAtomVersionAndFlags = READ(4);
                    read_bytes += 4;

                    if ((FullAtomVersionAndFlags >> 24) > g_fullbox[i].max_version)
                    {
                        MP4D_ERROR("unsupported box version!");
                    }
                    if (g_fullbox[i].use_track_flag && !tr)
                    {
                        MP4D_ERROR("broken file structure!");
                    }
                }
            }
        }
        else // stack[depth].format == BOX_OD
        {
            int val;
            box_name = OD_BASE + mp4d_read(f, 1, &eof_flag);     // 1-byte box type
            read_bytes += 1;
            if (eof_flag)
            {
                break;
            }

            payload_bytes = 0;
            box_bytes = 1;
            do
            {
                val = mp4d_read(f, 1, &eof_flag);
                read_bytes += 1;
                if (eof_flag)
                {
                    MP4D_ERROR("premature EOF!");
                }
                payload_bytes = (payload_bytes << 7) | (val & 0x7F);
                box_bytes++;
            } while (val & 0x80);
            box_bytes += payload_bytes;
        }

#if MP4D_DEBUG_TRACE
        box_path[depth] = (box_name >> 24) | (box_name << 24) | ((box_name >> 8) & 0x0000FF00) | ((box_name << 8) & 0x00FF0000);
        MP4D_TRACE(("%2d  %8d %.*s  (%d bytes remains for sibilings) \n", depth, (int)box_bytes, depth*4, (char*)box_path, (int)stack[depth].bytes));
#endif

        // Check that box size <= parent size
        if (depth)
        {
            // Skip box with bad size
            assert(box_bytes > 0);
            if (box_bytes > stack[depth].bytes)
            {
                MP4D_TRACE(("Wrong %c%c%c%c box size: broken file?\n", (box_name >> 24)&255, (box_name >> 16)&255, (box_name >> 8)&255, box_name&255));
                box_bytes = stack[depth].bytes;
                box_name = 0;
                payload_bytes = box_bytes - read_bytes;
            }
            stack[depth].bytes -= box_bytes;
        }

        // Read box header
        switch(box_name)
        {
        case BOX_stz2:  //ISO/IEC 14496-1 Page 38. Section 8.17.2 - Sample Size Box.
        case BOX_stsz:
            {
                int carry_size = 0;
                uint32_t sample_size = READ(4);
                tr->sample_count = READ(4);
                MP4D_MALLOC(tr->entry_size, tr->sample_count*4);
                for (i = 0; i < tr->sample_count; i++)
                {
                    if (box_name == BOX_stsz)
                    {
                       tr->entry_size[i] = (sample_size?sample_size:READ(4));
                    }
                    else
                    {
                        switch (sample_size & 0xFF)
                        {
                        case 16:
                            tr->entry_size[i] = READ(2);
                            break;
                        case  8:
                            tr->entry_size[i] = READ(1);
                            break;
                        case  4:
                            if (i&1)
                            {
                                tr->entry_size[i] = carry_size & 15;
                            }
                            else
                            {
                                carry_size = READ(1);
                                tr->entry_size[i] = (carry_size >> 4);
                            }
                            break;
                        }
                    }
                }
            }
            break;

        case BOX_stsc:  //ISO/IEC 14496-12 Page 38. Section 8.18 - Sample To Chunk Box.
            tr->sample_to_chunk_count = READ(4);
            MP4D_MALLOC(tr->sample_to_chunk, tr->sample_to_chunk_count*sizeof(tr->sample_to_chunk[0]));
            for (i = 0; i < tr->sample_to_chunk_count; i++)
            {
                tr->sample_to_chunk[i].first_chunk = READ(4);
                tr->sample_to_chunk[i].samples_per_chunk = READ(4);
                SKIP(4);    // sample_description_index
            }
            break;

        case BOX_stts:
            {
                unsigned count = READ(4);
                unsigned j, k = 0, ts = 0, ts_count = count;
                MP4D_MALLOC(tr->timestamp, ts_count*4);
                MP4D_MALLOC(tr->duration, ts_count*4);

                for (i = 0; i < count; i++)
                {
                    unsigned sc = READ(4);
                    int d =  READ(4);
                    MP4D_TRACE(("sample %8d count %8d duration %8d\n",i,sc,d));
                    if (k + sc > ts_count)
                    {
                        ts_count = k + sc;
                        MP4D_REALLOC(tr->timestamp, ts_count * sizeof(unsigned));
                        MP4D_REALLOC(tr->duration, ts_count * sizeof(unsigned));
                    }
                    for (j = 0; j < sc; j++)
                    {
                        tr->duration[k] = d;
                        tr->timestamp[k++] = ts;
                        ts += d;
                    }
                }
            }
            break;

        case BOX_ctts:
            {
                unsigned count = READ(4);
                for (i = 0; i < count; i++)
                {
                    int sc = READ(4);
                    int d =  READ(4);
                    (void)sc;
                    (void)d;
                    MP4D_TRACE(("sample %8d count %8d decoding to composition offset %8d\n",i,sc,d));
                }
            }
            break;

        case BOX_stco:  //ISO/IEC 14496-12 Page 39. Section 8.19 - Chunk Offset Box.
        case BOX_co64:
            tr->chunk_count = READ(4);
            MP4D_MALLOC(tr->chunk_offset, tr->chunk_count*sizeof(mp4d_size_t));
            for (i = 0; i < tr->chunk_count; i++)
            {
                tr->chunk_offset[i] = READ(4);
                if (box_name == BOX_co64)
                {
                    // 64-bit chunk_offset 
                    tr->chunk_offset[i] <<= 32;
                    tr->chunk_offset[i] |= READ(4);
                }
            }
            break;

        case BOX_mvhd:
            SKIP(((FullAtomVersionAndFlags >> 24) == 1) ? 8+8 : 4+4);
            mp4->timescale = READ(4);
            mp4->duration_hi = ((FullAtomVersionAndFlags >> 24) == 1) ? READ(4) : 0;
            mp4->duration_lo = READ(4);
            SKIP(4+2+2+4*2+4*9+4*6+4);
            break;

        case BOX_mdhd:
            SKIP(((FullAtomVersionAndFlags >> 24) == 1) ? 8+8 : 4+4);
            tr->timescale = READ(4);
            tr->duration_hi = ((FullAtomVersionAndFlags >> 24) == 1) ? READ(4) : 0;
            tr->duration_lo = READ(4);

            {
                int ISO_639_2_T = READ(2);
                tr->language[2] = (ISO_639_2_T & 31) + 0x60;ISO_639_2_T >>= 5;
                tr->language[1] = (ISO_639_2_T & 31) + 0x60;ISO_639_2_T >>= 5;
                tr->language[0] = (ISO_639_2_T & 31) + 0x60;
            }
            // the rest of this box is skipped by default ...
            break;

        case BOX_mdia:
            read_hdlr = 1;
            break;

        case BOX_minf:
            read_hdlr = 0;
            break;

        case BOX_hdlr:
            if (tr && read_hdlr) // When this box is within 'meta' box, the track may not be avaialable
            {
                SKIP(4);            // pre_defined
                tr->handler_type = READ(4);
            }
            // typically hdlr box does not contain any useful info.
            // the rest of this box is skipped by default ...
            break;

        case BOX_btrt:
            if (!tr)
            {
                MP4D_ERROR("broken file structure!");
            }

            SKIP(4+4);
            tr->avg_bitrate_bps = READ(4);
            break;

            // Set pointer to tag to be read...
        case BOX_calb: ptag = &mp4->tag.album; break;
        case BOX_cART: ptag = &mp4->tag.artist; break;
        case BOX_cnam: ptag = &mp4->tag.title; break;
        case BOX_cday: ptag = &mp4->tag.year; break;
        case BOX_ccmt: ptag = &mp4->tag.comment; break;
        case BOX_cgen: ptag = &mp4->tag.genre; break;

        case BOX_stsd:
            SKIP(4); // entry_count, BOX_mp4a & BOX_mp4v boxes follows immediately
            break;

        case BOX_mp4s:  // private stream
            if (!tr)
            {
                MP4D_ERROR("broken file structure!");
            }
            SKIP(6*1+2/*Base SampleEntry*/);
            break;

        case BOX_mp4a:
            if (!tr)
            {
                MP4D_ERROR("broken file structure!");
            }
            SKIP(6*1+2/*Base SampleEntry*/  + 4*2);
            tr->SampleDescription.audio.channelcount = READ(2);
            SKIP(2/*samplesize*/ + 2 + 2);
            tr->SampleDescription.audio.samplerate_hz = READ(4) >> 16;
            break;

        // Hap subtypes
        case BOX_Hap1:
        case BOX_Hap5:
        case BOX_HapY:
        case BOX_HapM:
        case BOX_HapA:

        // vvvvvvvvvvvvv AVC support vvvvvvvvvvvvv
        case BOX_avc1:  // AVCSampleEntry extends VisualSampleEntry 
//         case BOX_avc2:   - no test
//         case BOX_svc1:   - no test
        case BOX_mp4v:
            if (!tr)
            {
                MP4D_ERROR("broken file structure!");
            }
            SKIP(6*1 + 2/*Base SampleEntry*/ + 2 + 2 + 4*3);
            tr->SampleDescription.video.width = READ(2);
            tr->SampleDescription.video.height = READ(2);
            // frame_count is always 1
            // compressorname is rarely set..
            SKIP(4 + 4 + 4 + 2/*frame_count*/ + 32/*compressorname*/ + 2 + 2);
            // ^^^ end of VisualSampleEntry 
            // now follows for BOX_avc1:
            //      BOX_avcC
            //      BOX_btrt (optional)
            //      BOX_m4ds (optional)
            // for BOX_mp4v:
            //      BOX_esds        
            break;
        
        case BOX_avcC:  // AVCDecoderConfigurationRecord()
            // hack: AAC-specific DSI field reused (for it have same purpose as sps/pps)
            // TODO: check this hack if BOX_esds co-exist with BOX_avcC 
            tr->object_type_indication = MP4_OBJECT_TYPE_AVC;
            tr->dsi = malloc((size_t)box_bytes);
            tr->dsi_bytes = (unsigned)box_bytes;
            {
                int spspps;
                unsigned char * p = tr->dsi;
                unsigned int configurationVersion = READ(1);
                unsigned int AVCProfileIndication = READ(1);
                unsigned int profile_compatibility = READ(1);
                unsigned int AVCLevelIndication = READ(1);
                //bit(6) reserved = ‘111111’b;
                unsigned int lengthSizeMinusOne = READ(1) & 3;
                
                (void)configurationVersion;
                (void)AVCProfileIndication;
                (void)profile_compatibility;
                (void)AVCLevelIndication;
                (void)lengthSizeMinusOne;
                for (spspps = 0; spspps < 2; spspps++)
                {
                    unsigned int numOfSequenceParameterSets= READ(1);
                    if (!spspps)
                    {
                         numOfSequenceParameterSets &= 31;  // clears 3 msb for SPS
                    }
                    *p++ = numOfSequenceParameterSets;
                    for (i=0; i< numOfSequenceParameterSets; i++) {
                        unsigned k, sequenceParameterSetLength  = READ(2);
                        *p++ = sequenceParameterSetLength >> 8;
                        *p++ = sequenceParameterSetLength ;
                        for (k = 0; k < sequenceParameterSetLength ; k++)
                        {
                            *p++ = READ(1);
                        }
                    }
                }
            }
            break;
        // ^^^^^^^^^^^^^ AVC support ^^^^^^^^^^^^^

        case OD_ESD:
            {
                unsigned flags = READ(3);   // ES_ID(2) + flags(1)

                if (flags & 0x80)       // steamdependflag
                {
                    SKIP(2);            // dependsOnESID
                }
                if (flags & 0x40)       // urlflag
                {
                    unsigned bytecount = READ(1);
                    SKIP(bytecount);    // skip URL
                }
                if (flags & 0x20)       // ocrflag (was reserved in MPEG-4 v.1)
                {
                    SKIP(2);            // OCRESID
                }
                break;
            }

        case OD_DCD:        //ISO/IEC 14496-1 Page 28. Section 8.6.5 - DecoderConfigDescriptor.
            assert(tr);     // ensured by g_fullbox[] check
            tr->object_type_indication = READ(1);
            tr->stream_type = READ(1) >> 2;
            SKIP(3/*bufferSizeDB*/ + 4/*maxBitrate*/);
            tr->avg_bitrate_bps = READ(4);
            break;

        case OD_DSI:        //ISO/IEC 14496-1 Page 28. Section 8.6.5 - DecoderConfigDescriptor.
            assert(tr);     // ensured by g_fullbox[] check
            if (!tr->dsi && payload_bytes)
            {
                MP4D_MALLOC(tr->dsi, (int)payload_bytes);
                for (i = 0; i < payload_bytes; i++)
                {
                    tr->dsi[i] = mp4d_read(f, 1, &eof_flag);    // These bytes available due to check above
                }
                tr->dsi_bytes = i;
                payload_bytes -= i;
                break;
            }

        default:
            MP4D_TRACE(("[%c%c%c%c]  %d\n", box_name>>24, box_name>>16, box_name>>8, box_name, (int)payload_bytes));
        }

        // Read tag is tag pointer is set
        if (ptag && !*ptag && payload_bytes > 16)
        {
            SKIP(4+4+4+4);
            MP4D_MALLOC(*ptag, (unsigned)payload_bytes + 1);
            for (i = 0; payload_bytes != 0; i++)
            {
                (*ptag)[i] = READ(1);
            }
            (*ptag)[i] = 0; // zero-terminated string
        }

        if (box_name == BOX_trak)
        {
            // New track found: allocate memory using realloc()
            // Typically there are 1 audio track for AAC audio file,
            // 4 tracks for movie file,
            // 3-5 tracks for scalable audio (CELP+AAC)
            // and up to 50 tracks for BSAC scalable audio
            MP4D_REALLOC(mp4->track, (mp4->track_count + 1)*sizeof(MP4D_track_t));
            tr = mp4->track + mp4->track_count++;
            memset(tr, 0, sizeof(MP4D_track_t));
        }
        else if (box_name == BOX_meta)
        {
            tr = NULL;  // Avoid update of 'hdlr' box, which may contains in the 'meta' box
        }

        // If this box is envelope, save it's size in box stack
        for (i = 0; i < sizeof(g_envelope_box)/sizeof(g_envelope_box[0]); i++)
        {
            if (box_name == g_envelope_box[i].name)
            {
                if (++depth >= MP4D_MAX_CHUNKS_DEPTH)
                {
                    MP4D_ERROR("too deep atoms nesting!");
                }
                stack[depth].bytes = payload_bytes;
                stack[depth].format = g_envelope_box[i].type;
                break;
            }
        }

        // if box is not envelope, just skip it
        if (i == sizeof(g_envelope_box)/sizeof(g_envelope_box[0]))
        {
            if (payload_bytes > file_size)
            {
                eof_flag = 1;
            }
            else
            {
                SKIP(payload_bytes);
            }
        }

        // remove empty boxes from stack
        // don't touch box with index 0 (which indicates whole file)
        while (depth > 0 && !stack[depth].bytes)
        {
            depth--;
        }

    } while(!eof_flag);

    if (!mp4->track_count)
    {
        MP4D_RETURN_ERROR("no tracks found");
    }
    fseek(f, 0, SEEK_SET);
    return 1;
}

/**
*   Find chunk, containing given sample.
*   Returns chunk number, and first sample in this chunk.
*/
static int mp4d_sample_to_chunk(MP4D_track_t * tr, unsigned nsample, unsigned * nfirst_sample_in_chunk)
{
    unsigned chunk_group = 0, nc;
    unsigned sum = 0;
    *nfirst_sample_in_chunk = 0;
    if (tr->chunk_count <= 1)
    {
        return 0;
    }
    for (nc = 0; nc < tr->chunk_count; nc++)
    {
        if (chunk_group+1 < tr->sample_to_chunk_count     // stuck at last entry till EOF
            && nc + 1 ==    // Chunks counted starting with '1'
               tr->sample_to_chunk[chunk_group+1].first_chunk)    // next group?
        {
            chunk_group++;
        }

        sum += tr->sample_to_chunk[chunk_group].samples_per_chunk;
        if (nsample < sum)
        {
            return nc;
        }

        // TODO: this can be calculated once per file
        *nfirst_sample_in_chunk = sum;
    }
    return -1;
}

/**
*   Return position and size for given sample from given track.
*/
mp4d_size_t MP4D__frame_offset(const MP4D_demux_t * mp4, unsigned ntrack, unsigned nsample, unsigned * frame_bytes, unsigned * timestamp, unsigned * duration)
{
    MP4D_track_t * tr = mp4->track + ntrack;
    unsigned ns;
    int nchunk = mp4d_sample_to_chunk(tr, nsample, &ns);
    mp4d_size_t offset;

    if (nchunk < 0)
    {
        *frame_bytes = 0;
        return 0;
    }

    offset = tr->chunk_offset[nchunk];
    for (;ns < nsample; ns++)
    {
        offset += tr->entry_size[ns];
    }

    *frame_bytes = tr->entry_size[ns];

    if (timestamp)
    {
        *timestamp = tr->timestamp[ns];
    }
    if (duration)
    {
        *duration = tr->duration[ns];
    }

    return offset;
}

/**
*   De-allocated memory
*/
void MP4D__close(MP4D_demux_t * mp4)
{
#define FREE(x) if (x) {free(x); x = NULL;}
    while (mp4->track_count)
    {
        MP4D_track_t *tr = mp4->track + --mp4->track_count;
        FREE(tr->entry_size);
        FREE(tr->timestamp);
        FREE(tr->duration);
        FREE(tr->sample_to_chunk);
        FREE(tr->chunk_offset);
        FREE(tr->dsi);
    }
    FREE(mp4->track);
    FREE(mp4->tag.title);
    FREE(mp4->tag.artist);
    FREE(mp4->tag.album);
    FREE(mp4->tag.year);
    FREE(mp4->tag.comment);
    FREE(mp4->tag.genre);
}

/**
*   skip given number of SPS/PPS in the list.
*   return number of bytes skipped
*/
static int mp4d_skip_spspps(const unsigned char * p, int nbytes, int nskip)
{
    int i, k = 0;
    for (i = 0; i < nskip; i++)
    {
        unsigned segmbytes;
        if (k > nbytes - 2)
        {
            return -1;
        }
        segmbytes = p[k]*256 + p[k+1];
        k += 2 + segmbytes;
    }
    return k;
}

/**
*   Read SPS/PPS with given index
*/
static const unsigned char * MP4D__read_spspps(const MP4D_demux_t * mp4, unsigned int ntrack, int pps_flag, int nsps, int * sps_bytes)
{
    int sps_count, skip_bytes;
    int bytepos = 0;
    unsigned char * p = mp4->track[ntrack].dsi;
    if (ntrack >= mp4->track_count)
    {
        return NULL;
    }
    if (mp4->track[ntrack].object_type_indication != MP4_OBJECT_TYPE_AVC)
    {
        return NULL;    // SPS/PPS are specific for AVC format only
    }

    if (pps_flag)
    {
        // Skip all SPS
        sps_count = p[bytepos++];
        skip_bytes = mp4d_skip_spspps(p+bytepos, mp4->track[ntrack].dsi_bytes - bytepos, sps_count);
        if (skip_bytes < 0)
        {
            return NULL;
        }
        bytepos += skip_bytes;
    }

    // Skip sps/pps before the given target
    sps_count = p[bytepos++];
    if (nsps >= sps_count)
    {
        return NULL;
    }
    skip_bytes = mp4d_skip_spspps(p+bytepos, mp4->track[ntrack].dsi_bytes - bytepos, nsps);
    if (skip_bytes < 0)
    {
        return NULL;
    }
    bytepos += skip_bytes;
    *sps_bytes = p[bytepos]*256 + p[bytepos+1];
    return p + bytepos + 2;
}

/**
*   Read SPS with given index
*   Return pointer to internal mp4 memory, it must not be free()-ed
*/
const unsigned char * MP4D__read_sps(const MP4D_demux_t * mp4, unsigned int ntrack, int nsps, int * sps_bytes)
{
    return MP4D__read_spspps(mp4, ntrack, 0, nsps, sps_bytes);
}

/**
*   Read PPS with given index
*   Return pointer to internal mp4 memory, it must not be free()-ed
*/
const unsigned char * MP4D__read_pps(const MP4D_demux_t * mp4, unsigned int ntrack, int npps, int * pps_bytes)
{
    return MP4D__read_spspps(mp4, ntrack, 1, npps, pps_bytes);
}
    

/************************************************************************/
/*  Purely informational part, may be removed for embedded applications */
/************************************************************************/

/**
*   Decodes ISO/IEC 14496 MP4 stream type to ASCII string
*/
const char * MP4D__stream_type_to_ascii(int stream_type)
{
    switch (stream_type)
    {
    case 0x00: return "Forbidden";
    case 0x01: return "ObjectDescriptorStream";
    case 0x02: return "ClockReferenceStream";
    case 0x03: return "SceneDescriptionStream";
    case 0x04: return "VisualStream";
    case 0x05: return "AudioStream";
    case 0x06: return "MPEG7Stream";
    case 0x07: return "IPMPStream";
    case 0x08: return "ObjectContentInfoStream";
    case 0x09: return "MPEGJStream";
    default:
        if (stream_type >= 0x20 && stream_type <= 0x3F)
        {
            return "User private";
        }
        else
        {
            return "Reserved for ISO use";
        }
    }
}

/**
*   Decodes ISO/IEC 14496 MP4 object type to ASCII string
*/
const char * MP4D__object_type_to_ascii(int object_type_indication)
{
    switch (object_type_indication)
    {
    case 0x00: return "Forbidden";
    case 0x01: return "Systems ISO/IEC 14496-1";
    case 0x02: return "Systems ISO/IEC 14496-1";
    case 0x20: return "Visual ISO/IEC 14496-2";
    case 0x21: return "Visual ISO/IEC 14496-10";
    case 0x22: return "Visual ISO/IEC 14496-10 Parameter Sets";
    case 0x40: return "Audio ISO/IEC 14496-3";
    case 0x60: return "Visual ISO/IEC 13818-2 Simple Profile";
    case 0x61: return "Visual ISO/IEC 13818-2 Main Profile";
    case 0x62: return "Visual ISO/IEC 13818-2 SNR Profile";
    case 0x63: return "Visual ISO/IEC 13818-2 Spatial Profile";
    case 0x64: return "Visual ISO/IEC 13818-2 High Profile";
    case 0x65: return "Visual ISO/IEC 13818-2 422 Profile";
    case 0x66: return "Audio ISO/IEC 13818-7 Main Profile";
    case 0x67: return "Audio ISO/IEC 13818-7 LC Profile";
    case 0x68: return "Audio ISO/IEC 13818-7 SSR Profile";
    case 0x69: return "Audio ISO/IEC 13818-3";
    case 0x6A: return "Visual ISO/IEC 11172-2";
    case 0x6B: return "Audio ISO/IEC 11172-3";
    case 0x6C: return "Visual ISO/IEC 10918-1";
    case 0xFF: return "no object type specified";
    default:
        if (object_type_indication >= 0xC0 && object_type_indication <= 0xFE)
        {
            return "User private";
        }
        else
        {
            return "Reserved for ISO use";
        }
    }
}


#ifdef mp4demux_test
/******************************************************************************
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
!!!!                                                                       !!!!
!!!!                 !!!!!!!!  !!!!!!!!   !!!!!!!   !!!!!!!!               !!!!
!!!!                    !!     !!        !!            !!                  !!!!
!!!!                    !!     !!        !!            !!                  !!!!
!!!!                    !!     !!!!!!     !!!!!!!      !!                  !!!!
!!!!                    !!     !!               !!     !!                  !!!!
!!!!                    !!     !!               !!     !!                  !!!!
!!!!                    !!     !!!!!!!!   !!!!!!!      !!                  !!!!
!!!!                                                                       !!!!
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
******************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <math.h>
#include <memory.h>
#include "mp4demux.h"


/**
*   Print MP4 information to stdout.
*/
static void print_mp4_info(const MP4D_demux_t * mp4_demux)
{
    unsigned i;

    printf("\nMP4 FILE: %d tracks found. Movie time %.2f sec\n", mp4_demux->track_count, (4294967296.0*mp4_demux->duration_hi + mp4_demux->duration_lo) / mp4_demux->timescale);
    printf("\nNo|type|lng| duration           | bitrate| %-23s| Object type","Stream type");
    for (i = 0; i < mp4_demux->track_count; i++)
    {
        MP4D_track_t * tr = mp4_demux->track + i;

        printf("\n%2d|%c%c%c%c|%c%c%c|%7.2f s %6d frm| %7d|", i,
            (tr->handler_type>>24), (tr->handler_type>>16), (tr->handler_type>>8), (tr->handler_type>>0),
            tr->language[0],tr->language[1],tr->language[2],
            (65536.0* 65536.0*tr->duration_hi + tr->duration_lo) / tr->timescale,
            tr->sample_count,
            tr->avg_bitrate_bps
            );

        printf(" %-23s|", MP4D__stream_type_to_ascii(tr->stream_type));
        printf(" %-23s", MP4D__object_type_to_ascii(tr->object_type_indication));

        if (tr->handler_type == MP4_HANDLER_TYPE_SOUN)
        {
            printf("  -  %d ch %d hz", tr->SampleDescription.audio.channelcount, tr->SampleDescription.audio.samplerate_hz);
        }
        else if (tr->handler_type == MP4_HANDLER_TYPE_VIDE)
        {
            printf("  -  %dx%d", tr->SampleDescription.video.width, tr->SampleDescription.video.height);
        }
    }
    printf("\n");
}

/**
*   Print MP4 file comment to stdout.
*/
static void print_comment(const MP4D_demux_t * mp4_demux)
{
#define STR_TAG(name) if (mp4_demux->tag.name)  printf("%10s = %s\n", #name, mp4_demux->tag.name)
    STR_TAG(title);
    STR_TAG(artist);
    STR_TAG(album);
    STR_TAG(year);
    STR_TAG(comment);
    STR_TAG(genre);
}

/**
*   Print SPS/PPS/DSI data in hex to stdout.
*/
static void print_dsi_data(const MP4D_demux_t * mp4_demux)
{
    unsigned k, ntrack;
    for (ntrack = 0; ntrack < mp4_demux->track_count; ntrack++)
    {
        MP4D_track_t *tr = mp4_demux->track + ntrack;
        if (tr->dsi_bytes)
        {
            int i, k, sps_bytes, pps_bytes, sps_pps_found = 0;
            for (i = 0; i < 256; i++)
            {
                const unsigned char *sps = MP4D__read_sps(mp4_demux, ntrack, i, &sps_bytes);
                const unsigned char *pps = MP4D__read_pps(mp4_demux, ntrack, i, &pps_bytes);
                if (sps && sps_bytes)
                {
                    printf("%d SPS bytes found for track #%d:\n", sps_bytes, ntrack);
                    for (k = 0; k < sps_bytes; k++)
                    {
                        printf("%02x ", sps[k]);
                    }
                    printf("\n");
                    sps_pps_found = 1;
                }
                if (pps && pps_bytes)
                {
                    printf("%d PPS bytes found for track #%d:\n", pps_bytes, ntrack);
                    for (k = 0; k < pps_bytes; k++)
                    {
                        printf("%02x ", pps[k]);
                    }
                    printf("\n");
                    sps_pps_found = 1;
                }
            }

            if (!sps_pps_found)
            {
                printf("%d DSI bytes found for track #%d:\n", tr->dsi_bytes, ntrack);
                for (k = 0; k < tr->dsi_bytes; k++)
                {
                    printf("%02x ", tr->dsi[k]);
                }
                printf("\n");
            }
        }
    }
}

/**
*   Save AVC & audio tracks data to files
*/
static void save_track_data(const MP4D_demux_t * mp4_demux, FILE * mp4_file, unsigned ntrack)
{
    unsigned i, frame_bytes, timestamp, duration;
    unsigned avc_bytes_to_next_nal = 0;
    MP4D_track_t *tr = mp4_demux->track + ntrack;
    FILE * track_file;
    char name[100];
    const char * ext =  (tr->object_type_indication == MP4_OBJECT_TYPE_AVC) ? "264" : 
        (tr->handler_type == MP4_HANDLER_TYPE_SOUN) ? "audio" :
        (tr->handler_type == MP4_HANDLER_TYPE_VIDE) ? "video" : "data";

    sprintf(name, "track%d.%s", ntrack, ext);
    track_file = fopen(name,"wb");
    for (i = 0; i < tr->sample_count; i++)
    {
        unsigned char * frame_mem;
        mp4d_size_t frame_ofs = MP4D__frame_offset(mp4_demux, ntrack, i, &frame_bytes, &timestamp, &duration);

        // print frame offset
        //printf("%4d %06x %08d %d\n", i, (unsigned)ofs, duration, frame_bytes);
        //printf("%4d %06x %08d %d\n", i, (unsigned)ofs, timestamp, frame_bytes);

        // save payload
        frame_mem = malloc(frame_bytes);
        fseek(mp4_file, (long)frame_ofs, SEEK_SET);
        fread(frame_mem, 1, frame_bytes, mp4_file);

        if (mp4_demux->track[ntrack].object_type_indication == MP4_OBJECT_TYPE_AVC)
        {
            // replace 4-byte length field with start code
            unsigned startcode = 0x01000000;
            unsigned k = avc_bytes_to_next_nal;
            while (k < frame_bytes - 4)
            {
                avc_bytes_to_next_nal = 4 + ((frame_mem[k] * 256 + frame_mem[k+1])*256 + frame_mem[k+2])*256 + frame_mem[k+3];
                *(unsigned *)(frame_mem + k) = startcode;
                k += avc_bytes_to_next_nal;
            }
            avc_bytes_to_next_nal = k - frame_bytes;

            // Write SPS/PPS for 1st frame
            if (!i)
            {
                const void * data;
                int nps, bytes; 
                for (nps = 0; NULL != (data = MP4D__read_sps(mp4_demux, ntrack, nps, &bytes)); nps++)
                {
                    fwrite(&startcode, 1, 4, track_file);
                    fwrite(data, 1, bytes, track_file);
                }
                for (nps = 0; NULL != (data = MP4D__read_pps(mp4_demux, ntrack, nps, &bytes)); nps++)
                {
                    fwrite(&startcode, 1, 4, track_file);
                    fwrite(data, 1, bytes, track_file);
                }
            }
        }

        fwrite(frame_mem, 1, frame_bytes, track_file);
        free(frame_mem);
    }
    fclose(track_file);
}

int main(int argc, char* argv[])
{
    unsigned ntrack = 0;
    MP4D_demux_t mp4_demux = {0,};
    char* file_name = (argc>1)?argv[1]:"default_input.mp4";
    FILE * mp4_file = fopen(file_name, "rb");

    if (!mp4_file)
    {
        printf("\nERROR: can't open file %s for reading\n", file_name);
        return 0;
    }
    if (!MP4D__open(&mp4_demux, mp4_file))
    {
        printf("\nERROR: can't parse %s \n", file_name);
        return 0;
    }

    print_mp4_info(&mp4_demux);
    
    for (ntrack = 0; ntrack < mp4_demux.track_count; ntrack++)
    {
        save_track_data(&mp4_demux, mp4_file, ntrack);
    }

    print_comment(&mp4_demux);
    print_dsi_data(&mp4_demux);
    MP4D__close(&mp4_demux);
    fclose(mp4_file);

    return 0;
}

// dmc mp4demux.c -Dmp4demux_test && del *.obj *.map 
#endif  // mp4demux_test

