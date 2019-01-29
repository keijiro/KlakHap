#include <unordered_map>
#include "Decoder.h"
#include "Demuxer.h"
#include "ReadBuffer.h"
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
                params->texData = const_cast<void*>(it->second->LockBuffer());
            }
        }
        else if (event == kUnityRenderingExtEventUpdateTextureEndV2)
        {
            // UpdateTextureEnd:
            auto params = reinterpret_cast<UnityRenderingExtTextureUpdateParamsV2*>(data);
            auto it = decoderMap_.find(params->userData);
            if (it != decoderMap_.end()) it->second->UnlockBuffer();
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

#pragma region Read buffer functions

extern "C" ReadBuffer UNITY_INTERFACE_EXPORT * KlakHap_CreateReadBuffer()
{
    return new ReadBuffer;
}

extern "C" void UNITY_INTERFACE_EXPORT KlakHap_DestroyReadBuffer(ReadBuffer* buffer)
{
    if (buffer != nullptr) delete buffer;
}

#pragma endregion

#pragma region Demuxer functions

extern "C" Demuxer UNITY_INTERFACE_EXPORT * KlakHap_OpenDemuxer(const char* filepath)
{
    return new Demuxer(filepath);
}

extern "C" void UNITY_INTERFACE_EXPORT KlakHap_CloseDemuxer(Demuxer* demuxer)
{
    if (demuxer != nullptr) delete demuxer;
}

extern "C" int32_t UNITY_INTERFACE_EXPORT KlakHap_DemuxerIsValid(Demuxer* demuxer)
{
    if (demuxer == nullptr) return 0;
    return demuxer->IsValid() ? 1 : 0;
}

extern "C" int32_t UNITY_INTERFACE_EXPORT KlakHap_CountFrames(Demuxer* demuxer)
{
    if (demuxer == nullptr) return 0;
    return demuxer->GetVideoTrack().sample_count;
}

extern "C" double UNITY_INTERFACE_EXPORT KlakHap_GetDuration(Demuxer* demuxer)
{
    if (demuxer == nullptr) return 0;
    auto track = demuxer->GetVideoTrack();
    auto dur = static_cast<double>(track.duration_hi);
    dur = dur * 0x100000000L + track.duration_lo;
    return dur / track.timescale;
}

extern "C" int32_t UNITY_INTERFACE_EXPORT KlakHap_GetVideoWidth(Demuxer* demuxer)
{
    if (demuxer == nullptr) return 0;
    return demuxer->GetVideoTrack().SampleDescription.video.width;
}

extern "C" int32_t UNITY_INTERFACE_EXPORT KlakHap_GetVideoHeight(Demuxer* demuxer)
{
    if (demuxer == nullptr) return 0;
    return demuxer->GetVideoTrack().SampleDescription.video.height;
}

extern "C" int32_t UNITY_INTERFACE_EXPORT KlakHap_AnalyzeVideoType(Demuxer* demuxer)
{
    if (demuxer == nullptr) return 0;
    return demuxer->ReadVideoTypeField();
}

extern "C" void UNITY_INTERFACE_EXPORT KlakHap_ReadFrame(Demuxer* demuxer, int frameNumber, ReadBuffer* buffer)
{
    if (demuxer == nullptr || buffer == nullptr) return;
    return demuxer->ReadFrame(frameNumber, *buffer);
}

#pragma endregion

#pragma region Decoder functions

extern "C" Decoder UNITY_INTERFACE_EXPORT *KlakHap_CreateDecoder(int width, int height, int typeID)
{
    return new Decoder(width, height, typeID);
}

extern "C" void UNITY_INTERFACE_EXPORT KlakHap_DestroyDecoder(Decoder* decoder)
{
    delete decoder;
}

extern "C" void UNITY_INTERFACE_EXPORT KlakHap_AssignDecoder(uint32_t id, Decoder* decoder)
{
    if (decoder != nullptr)
        decoderMap_[id] = decoder;
    else
        decoderMap_.erase(decoderMap_.find(id));
}

extern "C" void UNITY_INTERFACE_EXPORT KlakHap_DecodeFrame(Decoder* decoder, const ReadBuffer* input)
{
    if (decoder == nullptr) return;
    decoder->DecodeFrame(*input);
}

extern "C" const void UNITY_INTERFACE_EXPORT *KlakHap_LockDecoderBuffer(Decoder* decoder)
{
    if (decoder == nullptr) return nullptr;
    return decoder->LockBuffer();
}

extern "C" void UNITY_INTERFACE_EXPORT KlakHap_UnlockDecoderBuffer(Decoder* decoder)
{
    if (decoder == nullptr) return;
    decoder->UnlockBuffer();
}

extern "C" int32_t UNITY_INTERFACE_EXPORT KlakHap_GetDecoderBufferSize(Decoder* decoder)
{
    if (decoder == nullptr) return 0;
    return static_cast<int32_t>(decoder->GetBufferSize());
}

#pragma endregion
