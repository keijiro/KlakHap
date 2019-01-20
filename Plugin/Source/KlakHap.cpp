#include <unordered_map>
#include "Decoder.h"
#include "IUnityRenderingExtensions.h"

using namespace KlakHap;

namespace
{
    #pragma region ID to decorder instance map

    std::unordered_map<uint32_t, Decoder*> decoderMap_;

    #pragma endregion

    #pragma region Texture update callback implementation

    //
    // Get a "fake" byte-per-pixel value for a given compression format
    //
    // This is a workaround for a problem that Unity incorrectly uses a BPP
    // value to calculate a texture pitch. We have to give a mutiplier for
    // it instead of an actual BPP value.
    //
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
                params->bpp = GetFakeBpp(params->format);
                params->texData = const_cast<void*>(it->second->LockDecodeBuffer());
            }
        }
        else if (event == kUnityRenderingExtEventUpdateTextureEndV2)
        {
            // UpdateTextureEnd:
            auto params = reinterpret_cast<UnityRenderingExtTextureUpdateParamsV2*>(data);
            auto it = decoderMap_.find(params->userData);
            if (it != decoderMap_.end()) it->second->UnlockDecodeBuffer();
        }
    }

    #pragma endregion
}

#pragma region Plugin common function

extern "C" UnityRenderingEventAndData UNITY_INTERFACE_EXPORT KlakHap_GetTextureUpdateCallback()
{
    return TextureUpdateCallback;
}

#pragma endregion

#pragma region ID-instance map functions

extern "C" void UNITY_INTERFACE_EXPORT KlakHap_MapInstance(uint32_t id, Decoder* decoder)
{
    if (decoder != nullptr)
        decoderMap_[id] = decoder;
    else
        decoderMap_.erase(decoderMap_.find(id));
}

#pragma endregion

#pragma region Allocation/deallocation functions

extern "C" Decoder UNITY_INTERFACE_EXPORT * KlakHap_Open(const char* filepath)
{
    return new Decoder(filepath);
}

extern "C" void UNITY_INTERFACE_EXPORT KlakHap_Close(Decoder* decoder)
{
    if (decoder != nullptr) delete decoder;
}

#pragma endregion

#pragma region Accessor functions

extern "C" int32_t UNITY_INTERFACE_EXPORT KlakHap_IsValid(Decoder* decoder)
{
    if (decoder == nullptr) return 0;
    return decoder->IsValid() ? 1 : 0;
}

extern "C" int32_t UNITY_INTERFACE_EXPORT KlakHap_CountFrames(Decoder* decoder)
{
    if (decoder == nullptr) return 0;
    return decoder->GetVideoTrack().sample_count;
}

extern "C" double UNITY_INTERFACE_EXPORT KlakHap_GetDuration(Decoder* decoder)
{
    if (decoder == nullptr) return 0;
    auto track = decoder->GetVideoTrack();
    auto dur = static_cast<double>(track.duration_hi);
    dur = dur * 0x100000000L + track.duration_lo;
    return dur / track.timescale;
}

extern "C" int32_t UNITY_INTERFACE_EXPORT KlakHap_GetVideoWidth(Decoder* decoder)
{
    if (decoder == nullptr) return 0;
    return decoder->GetVideoTrack().SampleDescription.video.width;
}

extern "C" int32_t UNITY_INTERFACE_EXPORT KlakHap_GetVideoHeight(Decoder* decoder)
{
    if (decoder == nullptr) return 0;
    return decoder->GetVideoTrack().SampleDescription.video.height;
}

#pragma endregion

#pragma region Decoding functions

extern "C" int32_t UNITY_INTERFACE_EXPORT KlakHap_AnalyzeVideoType(Decoder* decoder)
{
    if (decoder == nullptr) return 0;
    return decoder->ReadVideoTypeField();
}

extern "C" void UNITY_INTERFACE_EXPORT KlakHap_DecodeFrame(Decoder* decoder, int32_t index)
{
    if (decoder == nullptr) return;
    decoder->DecodeFrame(index);
}

#pragma endregion
