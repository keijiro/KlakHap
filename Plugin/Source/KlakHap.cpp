#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <vector>
#include "mp4demux.h"
#include "hap.h"
#include "IUnityInterface.h"

namespace
{
    class Decoder
    {
    public:
        FILE* file_;
        MP4D_demux_t demux_;
        std::vector<uint8_t> readBuffer_;
        std::vector<uint8_t> decodeBuffer_;
        size_t frameDataSize_;

        Decoder()
        {
            readBuffer_.resize(4 * 1024 * 1024);
            decodeBuffer_.resize(4 * 1024 * 1024);
        }

        bool Open(const char* path)
        {
            if (fopen_s(&file_, path, "rb") != 0) return false;
            return MP4D__open(&demux_, file_);
        }

        void Close()
        {
            MP4D__close(&demux_);
            fclose(file_);
        }

        void DecodeFrame(uint32_t index)
        {
            unsigned int in_size, timestamp, duration;
            auto in_offs = MP4D__frame_offset(&demux_, 0, index, &in_size, &timestamp, &duration);

            fseek(file_, (long)in_offs, SEEK_SET);
            fread(readBuffer_.data(), in_size, 1, file_);

            unsigned long used;
            unsigned int format;
            HapDecode(
                readBuffer_.data(), in_size, 0,
                hap_callback, NULL,
                decodeBuffer_.data(), decodeBuffer_.size(),
                &used, &format
            );

            frameDataSize_ = used;
        }

        static void hap_callback(
            HapDecodeWorkFunction work, void* p,
            unsigned int count, void* info
        )
        {
            fputs("Threading callback is not implemented.", stderr);
            exit(-1);
        }
    };
}

extern "C" Decoder UNITY_INTERFACE_EXPORT * HapOpen(const char* filepath)
{
    auto decoder = new Decoder();
    if (!decoder->Open(filepath))
    {
        delete decoder;
        return nullptr;
    }
    return decoder;
}

extern "C" void UNITY_INTERFACE_EXPORT HapClose(Decoder* decoder)
{
    if (decoder == nullptr) return;
    decoder->Close();
    delete decoder;
}

extern "C" int32_t UNITY_INTERFACE_EXPORT HapCountFrames(Decoder* decoder)
{
    if (decoder == nullptr) return 0;
    return decoder->demux_.track[0].sample_count;
}

extern "C" int32_t UNITY_INTERFACE_EXPORT HapGetVideoWidth(Decoder* decoder)
{
    if (decoder == nullptr) return 0;
    return decoder->demux_.track[0].SampleDescription.video.width;
}

extern "C" int32_t UNITY_INTERFACE_EXPORT HapGetVideoHeight(Decoder* decoder)
{
    if (decoder == nullptr) return 0;
    return decoder->demux_.track[0].SampleDescription.video.height;
}

extern "C" void UNITY_INTERFACE_EXPORT HapDecodeFrame(Decoder* decoder, int32_t index)
{
    if (decoder == nullptr) return;
    decoder->DecodeFrame(index);
}

extern "C" const void UNITY_INTERFACE_EXPORT *HapGetBufferPointer(Decoder* decoder)
{
    if (decoder == nullptr) return nullptr;
    return decoder->decodeBuffer_.data();
}

extern "C" int32_t UNITY_INTERFACE_EXPORT HapGetFrameDataSize(Decoder* decoder)
{
    if (decoder == nullptr) return 0;
    return static_cast<int32_t>(decoder->frameDataSize_);
}
