#pragma once

#include <stdint.h>
#include "mp4demux.h"
#include "ReadBuffer.h"

namespace KlakHap
{
    class Demuxer
    {
    public:

        #pragma region Constructor/destructor

        Demuxer(const char* path)
        {
            std::memset(&demux_, 0, sizeof(MP4D_demux_t));

            if (fopen_s(&file_, path, "rb") != 0) return;

            if (MP4D__open(&demux_, file_) == 0)
            {
                fclose(file_);
                file_ = nullptr;
            }
        }

        ~Demuxer()
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

        #pragma endregion

        #pragma region Read methods

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

        void ReadFrame(int index, ReadBuffer& buffer)
        {
            // Frame data offset
            unsigned int inSize, timestamp, duration;
            auto inOffs = MP4D__frame_offset(&demux_, 0, index, &inSize, &timestamp, &duration);

            // Frame data read
            fseek(file_, (long)inOffs, SEEK_SET);
            buffer.storage.resize(inSize);
            fread(buffer.storage.data(), inSize, 1, file_);
        }

        #pragma endregion

    private:

        #pragma region Private members

        FILE* file_ = nullptr;
        MP4D_demux_t demux_;

        #pragma endregion
    };
}
