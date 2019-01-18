#include <stdint.h>
#include <map>
#include <vector>
#include "mp4demux.h"
#include "hap.h"
#include "IUnityRenderingExtensions.h"

namespace
{
    class Decoder
    {
    public:

        Decoder(uint32_t id) : id_(id)
        {
            readBuffer_.resize(4 * 1024 * 1024);
            decodeBuffer_.resize(4 * 1024 * 1024);
        }

        uint32_t GetID() const
        {
            return id_;
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
                decodeBuffer_.data(),
                static_cast<unsigned long>(decodeBuffer_.size()),
                &used, &format
            );

            frameDataSize_ = used;
        }

        const MP4D_track_t& GetVideoTrack() const
        {
            return demux_.track[0];
        }

        const void* GetDataPointer() const
        {
            return decodeBuffer_.data();
        }

    private:

        uint32_t id_;
        FILE* file_ = nullptr;
        MP4D_demux_t demux_;
        std::vector<uint8_t> readBuffer_;
        std::vector<uint8_t> decodeBuffer_;
        size_t frameDataSize_ = 0;

        static void hap_callback(
            HapDecodeWorkFunction work, void* p,
            unsigned int count, void* info
        )
        {
            fputs("Threading callback is not implemented.", stderr);
            exit(-1);
        }
    };

    std::map<uint32_t, Decoder> decoderMap_;
    uint32_t decoderCount_;

    // Callback for texture update events
    void TextureUpdateCallback(int eventID, void* data)
    {
        auto event = static_cast<UnityRenderingExtEventType>(eventID);

        if (event == kUnityRenderingExtEventUpdateTextureBeginV2)
        {
            // UpdateTextureBegin: Return texture image data.
            auto params = reinterpret_cast<UnityRenderingExtTextureUpdateParamsV2*>(data);
            auto it = decoderMap_.find(params->userData);
            if (it != decoderMap_.end())
            {
                // Note: It requires a pitch instead of actual texture width. Not sure if it's by design.
                params->width = ((it->second.GetVideoTrack().SampleDescription.video.width + 3) / 4) * 8;
                params->height = it->second.GetVideoTrack().SampleDescription.video.height;
                params->bpp = 1;
                params->texData = const_cast<void*>(it->second.GetDataPointer());
            }
        }
        else if (event == kUnityRenderingExtEventUpdateTextureEndV2)
        {
            // UpdateTextureEnd:
        }
    }
}

extern "C" UnityRenderingEventAndData UNITY_INTERFACE_EXPORT HapGetTextureUpdateCallback()
{
    return TextureUpdateCallback;
}

extern "C" Decoder UNITY_INTERFACE_EXPORT * HapOpen(const char* filepath)
{
    auto id = ++decoderCount_;
    auto it = decoderMap_.emplace(id, id).first;
    if (!it->second.Open(filepath))
    {
        decoderMap_.erase(it);
        return nullptr;
    }
    return &it->second;
}

extern "C" void UNITY_INTERFACE_EXPORT HapClose(Decoder* decoder)
{
    if (decoder == nullptr) return;
    decoder->Close();
    decoderMap_.erase(decoderMap_.find(decoder->GetID()));
}

extern "C" uint32_t UNITY_INTERFACE_EXPORT HapGetID(Decoder* decoder)
{
    if (decoder == nullptr) return 0;
    return decoder->GetID();
}

extern "C" int32_t UNITY_INTERFACE_EXPORT HapCountFrames(Decoder* decoder)
{
    if (decoder == nullptr) return 0;
    return decoder->GetVideoTrack().sample_count;
}

extern "C" int32_t UNITY_INTERFACE_EXPORT HapGetVideoWidth(Decoder* decoder)
{
    if (decoder == nullptr) return 0;
    return decoder->GetVideoTrack().SampleDescription.video.width;
}

extern "C" int32_t UNITY_INTERFACE_EXPORT HapGetVideoHeight(Decoder* decoder)
{
    if (decoder == nullptr) return 0;
    return decoder->GetVideoTrack().SampleDescription.video.height;
}

extern "C" void UNITY_INTERFACE_EXPORT HapDecodeFrame(Decoder* decoder, int32_t index)
{
    if (decoder == nullptr) return;
    decoder->DecodeFrame(index);
}
