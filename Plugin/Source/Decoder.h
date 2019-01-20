#pragma once

#include <stdint.h>
#include <vector>
#include <mutex>
#include "mp4demux.h"
#include "hap.h"

namespace KlakHap
{
    class Decoder
    {
    public:

        #pragma region Constructor/destructor

        Decoder(uint32_t id) : id_(id)
        {
            std::memset(&demux_, 0, sizeof(MP4D_demux_t));
        }

        #pragma endregion

        #pragma region Basic file operations

        bool Open(const char* path)
        {
            if (fopen_s(&file_, path, "rb") != 0) return false;
            return MP4D__open(&demux_, file_);
        }

        void Close()
        {
            MP4D__close(&demux_);
            if (file_ != nullptr) fclose(file_);
        }

        #pragma endregion

        #pragma region Public accessors

        uint32_t GetID() const
        {
            return id_;
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

        #pragma endregion

        #pragma region Decoding methods

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

        #pragma endregion

    private:

        #pragma region Internal-use members

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

        #pragma endregion

        #pragma region HAP callback implementation

        static void hap_callback(
            HapDecodeWorkFunction work, void* p,
            unsigned int count, void* info
        )
        {
            fputs("Threading callback is not implemented.", stderr);
            exit(-1);
        }

        #pragma endregion
    };
}
