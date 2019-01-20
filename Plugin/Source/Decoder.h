#pragma once

#include <stdint.h>
#include <algorithm>
#include <list>
#include <memory>
#include <mutex>
#include <vector>
#include "mp4demux.h"
#include "hap.h"

namespace KlakHap
{

    class Decoder
    {
    public:

        #pragma region Constructor/destructor

        Decoder(const char* path)
        {
            std::memset(&demux_, 0, sizeof(MP4D_demux_t));

            if (fopen_s(&file_, path, "rb") != 0) return;

            if (MP4D__open(&demux_, file_) == 0)
            {
                fclose(file_);
                file_ = nullptr;
            }

            // Initial entries for read buffer list
            for (auto i = 0; i < 3; i++)
                readBufferList_.push_back(std::make_shared<ReadBuffer>());
        }

        ~Decoder()
        {
            MP4D__close(&demux_);
            if (file_ != nullptr) fclose(file_);
        }

        #pragma endregion

        #pragma region Public accessors

        bool IsValid() const
        {
            return file_ != nullptr;
        }

        const MP4D_track_t& GetVideoTrack() const
        {
            return demux_.track[0];
        }

        const void* LockDecodeBuffer()
        {
            decodeBufferLock_.lock();
            return decodeBuffer_.data();
        }

        void UnlockDecodeBuffer()
        {
            decodeBufferLock_.unlock();
        }

        #pragma endregion

        #pragma region Analysis method

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

        #pragma endregion

        #pragma region Read buffer operations

        void ReadFrame(int32_t index)
        {
            // Do nothing if the specified frame has been already read.
            if (FindReadBufferEntry(index)) return;

            std::lock_guard<std::mutex> lock(readBufferLock_);

            // Pop out the oldest entry.
            auto buffer = readBufferList_.front();
            readBufferList_.pop_front();

            // Frame data offset
            unsigned int inSize, timestamp, duration;
            auto inOffs = MP4D__frame_offset(&demux_, 0, index, &inSize, &timestamp, &duration);

            // Frame data read
            fseek(file_, (long)inOffs, SEEK_SET);
            buffer->index_ = index;
            buffer->storage_.resize(inSize);
            fread(buffer->storage_.data(), inSize, 1, file_);

            // Return the buffer to the buffer list.
            readBufferList_.push_back(buffer);
        }

        #pragma endregion

        #pragma region Decoding operations

        void DecodeFrame(int32_t index)
        {
            std::lock_guard<std::mutex> readLock(readBufferLock_);

            // Do nothing if the specified frame is not ready.
            auto rb = FindReadBufferEntry(index);
            if (!rb) return;

            // Decoded data size calculation
            auto bpp = GetBppFromTypeField(rb->storage_[3]);
            auto sd = GetVideoTrack().SampleDescription.video;
            auto dataSize = sd.width * sd.height * bpp / 8;
            if (decodeBuffer_.size() < dataSize) decodeBuffer_.resize(dataSize);

            std::lock_guard<std::mutex> decodeLock(decodeBufferLock_);

            // Hap decoding
            unsigned int format;
            HapDecode(
                rb->storage_.data(),
                static_cast<unsigned long>(rb->storage_.size()),
                0, hap_callback, nullptr,
                decodeBuffer_.data(),
                static_cast<unsigned long>(dataSize),
                nullptr, &format
            );
        }

        #pragma endregion

    private:

        #pragma region Internal-use members

        FILE* file_ = nullptr;
        MP4D_demux_t demux_;

        std::vector<uint8_t> decodeBuffer_;
        std::mutex decodeBufferLock_;

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

        #pragma region Read buffer implementation

        struct ReadBuffer
        {
            int32_t index_ = -1;
            std::vector<uint8_t> storage_;
        };

        std::list<std::shared_ptr<ReadBuffer>> readBufferList_;
        std::mutex readBufferLock_;

        std::shared_ptr<ReadBuffer> FindReadBufferEntry(int index)
        {
            auto it = std::find_if(
                readBufferList_.begin(), readBufferList_.end(),
                [=] (const auto pb) { return pb->index_ == index; }
            );
            return it != readBufferList_.end() ? *it : nullptr;
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
