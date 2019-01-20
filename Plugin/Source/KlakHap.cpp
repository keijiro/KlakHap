#include <map>
#include "Decoder.h"
#include "IUnityRenderingExtensions.h"

namespace
{
    using namespace KlakHap;

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

extern "C" double UNITY_INTERFACE_EXPORT HapGetDuration(Decoder* decoder)
{
    if (decoder == nullptr) return 0;
    auto track = decoder->GetVideoTrack();
    auto dur = static_cast<double>(track.duration_hi);
    dur = dur * 0x100000000L + track.duration_lo;
    return dur / track.timescale;
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
