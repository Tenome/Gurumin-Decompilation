// ogg_vorbis_types.h : the libogg / libvorbis structures embedded in sound3d.dll's OggStream, and the library entry
// points the game code calls.
//
// The library is "Xiphophorus libVorbis I 20011217" (0x10034670-0x100482d0). Only the sizes of the state structures
// matter to the game code, so structures it never reads are opaque blocks of the right size; the ones it does read
// (ogg_packet, vorbis_info, vorbis_comment) are spelled out.
#ifndef OGG_VORBIS_TYPES_H
#define OGG_VORBIS_TYPES_H

typedef __int64 ogg_int64_t;

// ogg_sync_state (7 ints): raw bytes the page layer is fed with.
struct OggSyncState {
    unsigned char *data;
    int storage;
    int fill;
    int returned;
    int unsynced;
    int headerbytes;
    int bodybytes;
};

// ogg_page: one page found in the sync buffer.
struct OggPage {
    unsigned char *header;
    long headerLength;
    unsigned char *body;
    long bodyLength;
};

// ogg_stream_state: reassembles packets from pages of one logical stream (0x168 bytes).
struct OggStreamState {
    char opaque[0x168];
};

// ogg_packet: one Vorbis packet.
struct OggPacket {
    unsigned char *packet;
    long bytes;
    long beginningOfStream;
    long endOfStream;
    ogg_int64_t granulePosition;
    ogg_int64_t packetNumber;
};

// vorbis_info: stream parameters filled from the header packets.
struct VorbisInfo {
    int version;
    int channels;
    long rate;
    long bitrateUpper;
    long bitrateNominal;
    long bitrateLower;
    long bitrateWindow;
    void *codecSetup;
};

// vorbis_comment: tag list; userComments is a null-terminated array of strings.
struct VorbisComment {
    char **userComments;
    int *commentLengths;
    int comments;
    char *vendor;
};

// vorbis_dsp_state and vorbis_block (0x70 bytes each).
struct VorbisDspState {
    char opaque[0x70];
};
struct VorbisBlock {
    char opaque[0x70];
};

extern "C" {
// libogg
int ogg_sync_init(OggSyncState *sync);
int ogg_sync_clear(OggSyncState *sync);
char *ogg_sync_buffer(OggSyncState *sync, long size);
int ogg_sync_wrote(OggSyncState *sync, long bytes);
int ogg_sync_pageout(OggSyncState *sync, OggPage *page);
int ogg_page_serialno(OggPage *page);
int ogg_page_eos(OggPage *page);
int ogg_stream_init(OggStreamState *stream, int serialNumber);
int ogg_stream_clear(OggStreamState *stream);
int ogg_stream_pagein(OggStreamState *stream, OggPage *page);
int ogg_stream_packetout(OggStreamState *stream, OggPacket *packet);
// libvorbis
void vorbis_info_init(VorbisInfo *info);
void vorbis_info_clear(VorbisInfo *info);
void vorbis_comment_init(VorbisComment *comment);
void vorbis_comment_clear(VorbisComment *comment);
int vorbis_synthesis_headerin(VorbisInfo *info, VorbisComment *comment, OggPacket *packet);
int vorbis_synthesis_init(VorbisDspState *dsp, VorbisInfo *info);
int vorbis_block_init(VorbisDspState *dsp, VorbisBlock *block);
int vorbis_block_clear(VorbisBlock *block);
void vorbis_dsp_clear(VorbisDspState *dsp);
int vorbis_synthesis(VorbisBlock *block, OggPacket *packet);
int vorbis_synthesis_blockin(VorbisDspState *dsp, VorbisBlock *block);
int vorbis_synthesis_pcmout(VorbisDspState *dsp, float ***pcm);
int vorbis_synthesis_read(VorbisDspState *dsp, int samples);
}

#endif
