#pragma once

#include <stdint.h>
#include <cstring>
#include "mp4demux.h"
#include "ReadBuffer.h"

#ifdef _WIN32
#include <windows.h>
#endif

namespace KlakHap
{
    class Demuxer
    {
    public:

        #pragma region Constructor/destructor

        Demuxer(const char* path)
        {
            std::memset(&demux_, 0, sizeof(MP4D_demux_t));

        #ifdef _WIN32
            // Convert UTF-8 to wide character for Windows
            int wlen = MultiByteToWideChar(CP_UTF8, 0, path, -1, nullptr, 0);
            if (wlen <= 0) return;
            
            wchar_t* wpath = new wchar_t[wlen];
            MultiByteToWideChar(CP_UTF8, 0, path, -1, wpath, wlen);
            
            if (_wfopen_s(&file_, wpath, L"rb") != 0) {
                delete[] wpath;
                return;
            }
            delete[] wpath;
        #else
            file_ = fopen(path, "rb");
            if (file_ == nullptr) return;
        #endif

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
        #if defined(_WIN32)
            _fseeki64(file_, inOffs, SEEK_SET);
        #else
            fseek(file_, inOffs, SEEK_SET);
        #endif
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
