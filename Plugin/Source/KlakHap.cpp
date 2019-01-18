#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "mp4demux.h"
#include "hap.h"
#include "IUnityInterface.h"

static void hap_callback
    (HapDecodeWorkFunction work, void *p, unsigned int count, void *info)
{
    fputs("Threading callback is not implemented.", stderr);
    exit(-1);
}

typedef struct
{
    FILE *file;
    MP4D_demux_t demux;
    void *read_buffer;
    void *decode_buffer;
    int64_t decode_size;
}
DecoderContext;

extern "C" DecoderContext UNITY_INTERFACE_EXPORT *HapOpen(const char *filepath)
{
    DecoderContext *context = reinterpret_cast<DecoderContext*>(calloc(1, sizeof(DecoderContext)));

    context->read_buffer = malloc(4 * 1024 * 1024);
    context->decode_buffer = malloc(4 * 1024 * 1024);

    fopen_s(&context->file, filepath, "rb");
    MP4D__open(&context->demux, context->file);

    return context;
}

extern "C" void UNITY_INTERFACE_EXPORT HapClose(DecoderContext *context)
{
    fclose(context->file);
    free(context->read_buffer);
    free(context->decode_buffer);
    free(context);
}

extern "C" int64_t UNITY_INTERFACE_EXPORT HapCountFrames(DecoderContext *context)
{
    return context->demux.track[0].sample_count;
}

extern "C" int32_t UNITY_INTERFACE_EXPORT HapGetVideoWidth(DecoderContext *context)
{
    return context->demux.track[0].SampleDescription.video.width;
}

extern "C" int32_t UNITY_INTERFACE_EXPORT HapGetVideoHeight(DecoderContext *context)
{
    return context->demux.track[0].SampleDescription.video.height;
}

extern "C" void UNITY_INTERFACE_EXPORT HapDecodeFrame(DecoderContext *context, int32_t index)
{
    unsigned int in_size, timestamp, duration;
    mp4d_size_t in_offs = MP4D__frame_offset
        (&context->demux, 0, index, &in_size, &timestamp, &duration);

    fseek(context->file, (long)in_offs, SEEK_SET);
    fread(context->read_buffer, in_size, 1, context->file);

    unsigned long used;
    unsigned int format;
    HapDecode(
        context->read_buffer, in_size, 0,
        hap_callback, NULL,
        context->decode_buffer, 4 * 1024 * 1024,
        &used, &format
    );

    context->decode_size = used;
}

extern "C" void UNITY_INTERFACE_EXPORT *HapGetBufferPointer(DecoderContext *context)
{
    return context->decode_buffer;
}

extern "C" int64_t UNITY_INTERFACE_EXPORT HapGetBufferSize(DecoderContext *context)
{
    return context->decode_size;
}
