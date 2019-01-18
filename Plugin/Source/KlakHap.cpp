#include <stdint.h>
#include <map>
#include <vector>
#include <mutex>
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

        uint8_t ReadVideoTypeField()
        {
            // Data offset for the first frame
            unsigned int size;
            auto offs = MP4D__frame_offset(&demux_, 0, 0, &size, nullptr, nullptr);

            // Read to a temporary buffer.
            uint8_t temp;
            fseek(file_, (long)offs + 3, SEEK_SET);
            fread(&temp, 1, 1, file_);

            return temp;
        }

        void DecodeFrame(uint32_t index)
        {
            // Frame data offset
            unsigned int in_size, timestamp, duration;
            auto in_offs = MP4D__frame_offset(&demux_, 0, index, &in_size, &timestamp, &duration);

            // Read buffer lazy allocation
            if (readBuffer_.size() < in_size) readBuffer_.resize(in_size);

            // Frame data read
            fseek(file_, (long)in_offs, SEEK_SET);
            fread(readBuffer_.data(), in_size, 1, file_);

            // Decoded data size calculation
            auto bpp = GetBppFromTypeField(readBuffer_[3]);
            auto sd = GetVideoTrack().SampleDescription.video;
            auto data_size = sd.width * sd.height * bpp / 8;
            if (decodeBuffer_.size() < data_size) decodeBuffer_.resize(data_size);

            mutex_.lock();

            // Hap decoding
            unsigned int format;
            HapDecode(
                readBuffer_.data(), in_size, 0,
                hap_callback, nullptr,
                decodeBuffer_.data(), static_cast<unsigned long>(data_size),
                nullptr, &format
            );

            mutex_.unlock();
        }

        const MP4D_track_t& GetVideoTrack() const
        {
            return demux_.track[0];
        }

        const void* LockDecodeBuffer()
        {
            mutex_.lock();
            return decodeBuffer_.data();
        }

        void UnlockDecodeBuffer()
        {
            mutex_.unlock();
        }

    private:

        uint32_t id_;
        FILE* file_ = nullptr;
        MP4D_demux_t demux_;
        std::vector<uint8_t> readBuffer_;
        std::vector<uint8_t> decodeBuffer_;
        std::mutex mutex_;

        static size_t GetBppFromTypeField(uint8_t type)
        {
            switch (type & 0xf)
            {
            case 0xb: return 4; // DXT1
            case 0xe: return 8; // DXT5
            case 0xf: return 8; // DXT5
            case 0xc: return 8; // BC7
            case 0x1: return 4; // BC4
            }
            return 0;
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

    std::map<uint32_t, Decoder> decoderMap_;
    uint32_t decoderCount_;

    uint32_t GetFakeBpp(UnityRenderingExtTextureFormat format)
    {
        switch (format)
        {
        case kUnityRenderingExtFormatRGBA_DXT1_SRGB:
        case kUnityRenderingExtFormatRGBA_DXT1_UNorm:
        case kUnityRenderingExtFormatR_BC4_UNorm:
        case kUnityRenderingExtFormatR_BC4_SNorm:
            return 2;
        case kUnityRenderingExtFormatRGBA_DXT5_SRGB:
        case kUnityRenderingExtFormatRGBA_DXT5_UNorm:
        case kUnityRenderingExtFormatRGBA_BC7_SRGB:
        case kUnityRenderingExtFormatRGBA_BC7_UNorm:
            return 4;
        }
        return 0;
    }

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
                // WORKAROUND: Unity uses a BPP value to calculate a texture
                // pitch, so this is not an actual BPP -- just a multiplier
                // for pitch calculation.
                params->bpp = GetFakeBpp(params->format);
                params->texData = const_cast<void*>(it->second.LockDecodeBuffer());
            }
        }
        else if (event == kUnityRenderingExtEventUpdateTextureEndV2)
        {
            // UpdateTextureEnd:
            auto params = reinterpret_cast<UnityRenderingExtTextureUpdateParamsV2*>(data);
            auto it = decoderMap_.find(params->userData);
            if (it != decoderMap_.end()) it->second.UnlockDecodeBuffer();
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

extern "C" int32_t UNITY_INTERFACE_EXPORT HapGetVideoType(Decoder* decoder)
{
    if (decoder == nullptr) return 0;
    return decoder->ReadVideoTypeField();
}

extern "C" void UNITY_INTERFACE_EXPORT HapDecodeFrame(Decoder* decoder, int32_t index)
{
    if (decoder == nullptr) return;
    decoder->DecodeFrame(index);
}
