/** 25.09.2018 @file
*
*   Common MP4 definitions
*
*   Acronyms:
*       AVC = Advanced Video Coding (AKA H.264)
*       AAC = Advanced Audio Coding
*       OD  = Object descriptor
*       DSI = Decoder Specific Info (AAC element)
*       LC  = Low Complexity (AAC profile)
*       SPS = Sequence Parameter Set (H.264 element)
*       PPS = Picture Parameter Set (H.264 element)
*
*   The MP4 file has several tracks. Each track contains a number of 'samples'
*   (audio or video frames). Position and size of each sample in the track
*   encoded in the track index.
*
*/

#ifndef mp4defs_H_INCLUDED
#define mp4defs_H_INCLUDED

/************************************************************************/
/*          Some values of MP4X_track_t::object_type_indication         */
/************************************************************************/
enum
{
    // MPEG-4 AAC (all profiles)
    MP4_OBJECT_TYPE_AUDIO_ISO_IEC_14496_3 = 0x40,

    // MPEG-2 AAC, Main profile
    MP4_OBJECT_TYPE_AUDIO_ISO_IEC_13818_7_MAIN_PROFILE = 0x66,

    // MPEG-2 AAC, LC profile
    MP4_OBJECT_TYPE_AUDIO_ISO_IEC_13818_7_LC_PROFILE = 0x67,

    // MPEG-2 AAC, SSR profile
    MP4_OBJECT_TYPE_AUDIO_ISO_IEC_13818_7_SSR_PROFILE = 0x68,

    // H.264 (AVC) video 
    MP4_OBJECT_TYPE_AVC = 0x21,

    // http://www.mp4ra.org/object.html 0xC0-E0  && 0xE2 - 0xFE are specified as "user private"
    MP4_OBJECT_TYPE_USER_PRIVATE = 0xC0
};


/************************************************************************/
/*          Some values of MP4X_track_t::handler_type                   */
/************************************************************************/
enum
{
    // Video track : 'vide'
    MP4_HANDLER_TYPE_VIDE = 0x76696465,
    // Audio track : 'soun'
    MP4_HANDLER_TYPE_SOUN = 0x736F756E,
    // General MPEG-4 systems streams (without specific handler). 
    // Used for private stream, as suggested in http://www.mp4ra.org/handler.html
    MP4_HANDLER_TYPE_GESM = 0x6765736D,
    // Text comment track
    MP4_HANDLER_TYPE_MDIR = 0x6d646972
};

/************************************************************************/
/*      Complete box list (most of them not used here)                  */
/************************************************************************/
#define FOUR_CHAR_INT( a, b, c, d ) (((((((unsigned)(a)*256)|(b))*256)|(c))*256)|(d))
enum
{
    BOX_co64    = FOUR_CHAR_INT( 'c', 'o', '6', '4' ),//ChunkLargeOffsetAtomType
    BOX_stco    = FOUR_CHAR_INT( 's', 't', 'c', 'o' ),//ChunkOffsetAtomType
    BOX_crhd    = FOUR_CHAR_INT( 'c', 'r', 'h', 'd' ),//ClockReferenceMediaHeaderAtomType
    BOX_ctts    = FOUR_CHAR_INT( 'c', 't', 't', 's' ),//CompositionOffsetAtomType
    BOX_cprt    = FOUR_CHAR_INT( 'c', 'p', 'r', 't' ),//CopyrightAtomType
    BOX_url_    = FOUR_CHAR_INT( 'u', 'r', 'l', ' ' ),//DataEntryURLAtomType
    BOX_urn_    = FOUR_CHAR_INT( 'u', 'r', 'n', ' ' ),//DataEntryURNAtomType
    BOX_dinf    = FOUR_CHAR_INT( 'd', 'i', 'n', 'f' ),//DataInformationAtomType
    BOX_dref    = FOUR_CHAR_INT( 'd', 'r', 'e', 'f' ),//DataReferenceAtomType
    BOX_stdp    = FOUR_CHAR_INT( 's', 't', 'd', 'p' ),//DegradationPriorityAtomType
    BOX_edts    = FOUR_CHAR_INT( 'e', 'd', 't', 's' ),//EditAtomType
    BOX_elst    = FOUR_CHAR_INT( 'e', 'l', 's', 't' ),//EditListAtomType
    BOX_uuid    = FOUR_CHAR_INT( 'u', 'u', 'i', 'd' ),//ExtendedAtomType
    BOX_free    = FOUR_CHAR_INT( 'f', 'r', 'e', 'e' ),//FreeSpaceAtomType
    BOX_hdlr    = FOUR_CHAR_INT( 'h', 'd', 'l', 'r' ),//HandlerAtomType
    BOX_hmhd    = FOUR_CHAR_INT( 'h', 'm', 'h', 'd' ),//HintMediaHeaderAtomType
    BOX_hint    = FOUR_CHAR_INT( 'h', 'i', 'n', 't' ),//HintTrackReferenceAtomType
    BOX_mdia    = FOUR_CHAR_INT( 'm', 'd', 'i', 'a' ),//MediaAtomType
    BOX_mdat    = FOUR_CHAR_INT( 'm', 'd', 'a', 't' ),//MediaDataAtomType
    BOX_mdhd    = FOUR_CHAR_INT( 'm', 'd', 'h', 'd' ),//MediaHeaderAtomType
    BOX_minf    = FOUR_CHAR_INT( 'm', 'i', 'n', 'f' ),//MediaInformationAtomType
    BOX_moov    = FOUR_CHAR_INT( 'm', 'o', 'o', 'v' ),//MovieAtomType
    BOX_mvhd    = FOUR_CHAR_INT( 'm', 'v', 'h', 'd' ),//MovieHeaderAtomType
    BOX_stsd    = FOUR_CHAR_INT( 's', 't', 's', 'd' ),//SampleDescriptionAtomType
    BOX_stsz    = FOUR_CHAR_INT( 's', 't', 's', 'z' ),//SampleSizeAtomType
    BOX_stz2    = FOUR_CHAR_INT( 's', 't', 'z', '2' ),//CompactSampleSizeAtomType
    BOX_stbl    = FOUR_CHAR_INT( 's', 't', 'b', 'l' ),//SampleTableAtomType
    BOX_stsc    = FOUR_CHAR_INT( 's', 't', 's', 'c' ),//SampleToChunkAtomType
    BOX_stsh    = FOUR_CHAR_INT( 's', 't', 's', 'h' ),//ShadowSyncAtomType
    BOX_skip    = FOUR_CHAR_INT( 's', 'k', 'i', 'p' ),//SkipAtomType
    BOX_smhd    = FOUR_CHAR_INT( 's', 'm', 'h', 'd' ),//SoundMediaHeaderAtomType
    BOX_stss    = FOUR_CHAR_INT( 's', 't', 's', 's' ),//SyncSampleAtomType
    BOX_stts    = FOUR_CHAR_INT( 's', 't', 't', 's' ),//TimeToSampleAtomType
    BOX_trak    = FOUR_CHAR_INT( 't', 'r', 'a', 'k' ),//TrackAtomType
    BOX_tkhd    = FOUR_CHAR_INT( 't', 'k', 'h', 'd' ),//TrackHeaderAtomType
    BOX_tref    = FOUR_CHAR_INT( 't', 'r', 'e', 'f' ),//TrackReferenceAtomType
    BOX_udta    = FOUR_CHAR_INT( 'u', 'd', 't', 'a' ),//UserDataAtomType
    BOX_vmhd    = FOUR_CHAR_INT( 'v', 'm', 'h', 'd' ),//VideoMediaHeaderAtomType
    BOX_url     = FOUR_CHAR_INT( 'u', 'r', 'l', ' ' ),
    BOX_urn     = FOUR_CHAR_INT( 'u', 'r', 'n', ' ' ),

    BOX_gnrv    = FOUR_CHAR_INT( 'g', 'n', 'r', 'v' ),//GenericVisualSampleEntryAtomType
    BOX_gnra    = FOUR_CHAR_INT( 'g', 'n', 'r', 'a' ),//GenericAudioSampleEntryAtomType

    //V2 atoms
    BOX_ftyp    = FOUR_CHAR_INT( 'f', 't', 'y', 'p' ),//FileTypeAtomType
    BOX_padb    = FOUR_CHAR_INT( 'p', 'a', 'd', 'b' ),//PaddingBitsAtomType

    //MP4 Atoms
    BOX_sdhd    = FOUR_CHAR_INT( 's', 'd', 'h', 'd' ),//SceneDescriptionMediaHeaderAtomType
    BOX_dpnd    = FOUR_CHAR_INT( 'd', 'p', 'n', 'd' ),//StreamDependenceAtomType
    BOX_iods    = FOUR_CHAR_INT( 'i', 'o', 'd', 's' ),//ObjectDescriptorAtomType
    BOX_odhd    = FOUR_CHAR_INT( 'o', 'd', 'h', 'd' ),//ObjectDescriptorMediaHeaderAtomType
    BOX_mpod    = FOUR_CHAR_INT( 'm', 'p', 'o', 'd' ),//ODTrackReferenceAtomType
    BOX_nmhd    = FOUR_CHAR_INT( 'n', 'm', 'h', 'd' ),//MPEGMediaHeaderAtomType
    BOX_esds    = FOUR_CHAR_INT( 'e', 's', 'd', 's' ),//ESDAtomType
    BOX_sync    = FOUR_CHAR_INT( 's', 'y', 'n', 'c' ),//OCRReferenceAtomType
    BOX_ipir    = FOUR_CHAR_INT( 'i', 'p', 'i', 'r' ),//IPIReferenceAtomType
    BOX_mp4s    = FOUR_CHAR_INT( 'm', 'p', '4', 's' ),//MPEGSampleEntryAtomType
    BOX_mp4a    = FOUR_CHAR_INT( 'm', 'p', '4', 'a' ),//MPEGAudioSampleEntryAtomType
    BOX_mp4v    = FOUR_CHAR_INT( 'm', 'p', '4', 'v' ),//MPEGVisualSampleEntryAtomType

    // http://www.itscj.ipsj.or.jp/sc29/open/29view/29n7644t.doc
    BOX_avc1    = FOUR_CHAR_INT( 'a', 'v', 'c', '1' ),
    BOX_avc2    = FOUR_CHAR_INT( 'a', 'v', 'c', '2' ),
    BOX_svc1    = FOUR_CHAR_INT( 's', 'v', 'c', '1' ),
    BOX_avcC    = FOUR_CHAR_INT( 'a', 'v', 'c', 'C' ),
    BOX_svcC    = FOUR_CHAR_INT( 's', 'v', 'c', 'C' ),
    BOX_btrt    = FOUR_CHAR_INT( 'b', 't', 'r', 't' ),
    BOX_m4ds    = FOUR_CHAR_INT( 'm', '4', 'd', 's' ),
    BOX_seib    = FOUR_CHAR_INT( 's', 'e', 'i', 'b' ),

    //3GPP atoms
    BOX_samr    = FOUR_CHAR_INT( 's', 'a', 'm', 'r' ),//AMRSampleEntryAtomType
    BOX_sawb    = FOUR_CHAR_INT( 's', 'a', 'w', 'b' ),//WB_AMRSampleEntryAtomType
    BOX_damr    = FOUR_CHAR_INT( 'd', 'a', 'm', 'r' ),//AMRConfigAtomType
    BOX_s263    = FOUR_CHAR_INT( 's', '2', '6', '3' ),//H263SampleEntryAtomType
    BOX_d263    = FOUR_CHAR_INT( 'd', '2', '6', '3' ),//H263ConfigAtomType

    //V2 atoms - Movie Fragments
    BOX_mvex    = FOUR_CHAR_INT( 'm', 'v', 'e', 'x' ),//MovieExtendsAtomType
    BOX_trex    = FOUR_CHAR_INT( 't', 'r', 'e', 'x' ),//TrackExtendsAtomType
    BOX_moof    = FOUR_CHAR_INT( 'm', 'o', 'o', 'f' ),//MovieFragmentAtomType
    BOX_mfhd    = FOUR_CHAR_INT( 'm', 'f', 'h', 'd' ),//MovieFragmentHeaderAtomType
    BOX_traf    = FOUR_CHAR_INT( 't', 'r', 'a', 'f' ),//TrackFragmentAtomType
    BOX_tfhd    = FOUR_CHAR_INT( 't', 'f', 'h', 'd' ),//TrackFragmentHeaderAtomType
    BOX_trun    = FOUR_CHAR_INT( 't', 'r', 'u', 'n' ),//TrackFragmentRunAtomType

    // Object Descriptors (OD) data coding
    // These takes only 1 byte; this implementation translate <od_tag> to
    // <od_tag> + OD_BASE to keep API uniform and safe for string functions
    OD_BASE    = FOUR_CHAR_INT( '$', '$', '$', '0' ),//
    OD_ESD     = FOUR_CHAR_INT( '$', '$', '$', '3' ),//SDescriptor_Tag
    OD_DCD     = FOUR_CHAR_INT( '$', '$', '$', '4' ),//DecoderConfigDescriptor_Tag
    OD_DSI     = FOUR_CHAR_INT( '$', '$', '$', '5' ),//DecoderSpecificInfo_Tag
    OD_SLC     = FOUR_CHAR_INT( '$', '$', '$', '6' ),//SLConfigDescriptor_Tag

    BOX_meta   = FOUR_CHAR_INT( 'm', 'e', 't', 'a' ),
    BOX_ilst   = FOUR_CHAR_INT( 'i', 'l', 's', 't' ),

    // Metagata tags, see http://atomicparsley.sourceforge.net/mpeg-4files.html
    BOX_calb    = FOUR_CHAR_INT( '\xa9', 'a', 'l', 'b'),    // album
    BOX_cart    = FOUR_CHAR_INT( '\xa9', 'a', 'r', 't'),    // artist
    BOX_aART    = FOUR_CHAR_INT( 'a', 'A', 'R', 'T' ),      // album artist
    BOX_ccmt    = FOUR_CHAR_INT( '\xa9', 'c', 'm', 't'),    // comment
    BOX_cday    = FOUR_CHAR_INT( '\xa9', 'd', 'a', 'y'),    // year (as string)
    BOX_cnam    = FOUR_CHAR_INT( '\xa9', 'n', 'a', 'm'),    // title
    BOX_cgen    = FOUR_CHAR_INT( '\xa9', 'g', 'e', 'n'),    // custom genre (as string or as byte!)
    BOX_trkn    = FOUR_CHAR_INT( 't', 'r', 'k', 'n'),       // track number (byte)
    BOX_disk    = FOUR_CHAR_INT( 'd', 'i', 's', 'k'),       // disk number (byte)
    BOX_cwrt    = FOUR_CHAR_INT( '\xa9', 'w', 'r', 't'),    // composer
    BOX_ctoo    = FOUR_CHAR_INT( '\xa9', 't', 'o', 'o'),    // encoder
    BOX_tmpo    = FOUR_CHAR_INT( 't', 'm', 'p', 'o'),       // bpm (byte)
    BOX_cpil    = FOUR_CHAR_INT( 'c', 'p', 'i', 'l'),       // compilation (byte)
    BOX_covr    = FOUR_CHAR_INT( 'c', 'o', 'v', 'r'),       // cover art (JPEG/PNG)
    BOX_rtng    = FOUR_CHAR_INT( 'r', 't', 'n', 'g'),       // rating/advisory (byte)
    BOX_cgrp    = FOUR_CHAR_INT( '\xa9', 'g', 'r', 'p'),    // grouping
    BOX_stik    = FOUR_CHAR_INT( 's', 't', 'i', 'k'),       // stik (byte)  0 = Movie   1 = Normal  2 = Audiobook  5 = Whacked Bookmark  6 = Music Video  9 = Short Film  10 = TV Show  11 = Booklet  14 = Ringtone
    BOX_pcst    = FOUR_CHAR_INT( 'p', 'c', 's', 't'),       // podcast (byte)
    BOX_catg    = FOUR_CHAR_INT( 'c', 'a', 't', 'g'),       // category
    BOX_keyw    = FOUR_CHAR_INT( 'k', 'e', 'y', 'w'),       // keyword
    BOX_purl    = FOUR_CHAR_INT( 'p', 'u', 'r', 'l'),       // podcast URL (byte)
    BOX_egid    = FOUR_CHAR_INT( 'e', 'g', 'i', 'd'),       // episode global unique ID (byte)
    BOX_desc    = FOUR_CHAR_INT( 'd', 'e', 's', 'c'),       // description
    BOX_clyr    = FOUR_CHAR_INT( '\xa9', 'l', 'y', 'r'),    // lyrics (may be > 255 bytes)
    BOX_tven    = FOUR_CHAR_INT( 't', 'v', 'e', 'n'),       // tv episode number
    BOX_tves    = FOUR_CHAR_INT( 't', 'v', 'e', 's'),       // tv episode (byte)
    BOX_tvnn    = FOUR_CHAR_INT( 't', 'v', 'n', 'n'),       // tv network name
    BOX_tvsh    = FOUR_CHAR_INT( 't', 'v', 's', 'h'),       // tv show name
    BOX_tvsn    = FOUR_CHAR_INT( 't', 'v', 's', 'n'),       // tv season (byte)
    BOX_purd    = FOUR_CHAR_INT( 'p', 'u', 'r', 'd'),       // purchase date
    BOX_pgap    = FOUR_CHAR_INT( 'p', 'g', 'a', 'p'),       // Gapless Playback (byte)

    //BOX_aart   = FOUR_CHAR_INT( 'a', 'a', 'r', 't' ),     // Album artist
    BOX_cART    = FOUR_CHAR_INT( '\xa9', 'A', 'R', 'T'),    // artist
    BOX_gnre    = FOUR_CHAR_INT( 'g', 'n', 'r', 'e'),

    // 3GPP metatags  (http://cpansearch.perl.org/src/JHAR/MP4-Info-1.12/Info.pm)
    BOX_auth    = FOUR_CHAR_INT( 'a', 'u', 't', 'h'),       // author
    BOX_titl    = FOUR_CHAR_INT( 't', 'i', 't', 'l'),       // title
    BOX_dscp    = FOUR_CHAR_INT( 'd', 's', 'c', 'p'),       // description
    BOX_perf    = FOUR_CHAR_INT( 'p', 'e', 'r', 'f'),       // performer
    BOX_mean    = FOUR_CHAR_INT( 'm', 'e', 'a', 'n'),       //
    BOX_name    = FOUR_CHAR_INT( 'n', 'a', 'm', 'e'),       //
    BOX_data    = FOUR_CHAR_INT( 'd', 'a', 't', 'a'),       //

    // these from http://lists.mplayerhq.hu/pipermail/ffmpeg-devel/2008-September/053151.html
    BOX_albm    = FOUR_CHAR_INT( 'a', 'l', 'b', 'm'),      // album
    BOX_yrrc    = FOUR_CHAR_INT( 'y', 'r', 'r', 'c'),      // album

    // Hap subtypes
    BOX_Hap1    = FOUR_CHAR_INT( 'H', 'a', 'p', '1' ),
    BOX_Hap5    = FOUR_CHAR_INT( 'H', 'a', 'p', '5' ),
    BOX_HapY    = FOUR_CHAR_INT( 'H', 'a', 'p', 'Y' ),
    BOX_HapM    = FOUR_CHAR_INT( 'H', 'a', 'p', 'M' ),
    BOX_HapA    = FOUR_CHAR_INT( 'H', 'a', 'p', 'A' )
};

#endif //mp4defs_H_INCLUDED
