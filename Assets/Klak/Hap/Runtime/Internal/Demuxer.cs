using System;
using System.Runtime.InteropServices;

namespace Klak.Hap
{
    internal sealed class Demuxer : IDisposable
    {
        #region Public properties

        public bool IsValid { get { return _plugin != IntPtr.Zero; } }
        public int Width { get { return _width; } }
        public int Height { get { return _height; } }
        public int VideoType { get { return _videoType; } }
        public double Duration { get { return _duration; } }
        public int FrameCount { get { return _frameCount; } }

        #endregion

        #region Initialization/finalization

        public Demuxer(string filePath)
        {
            _plugin = KlakHap_OpenDemuxer(filePath);

            if (KlakHap_DemuxerIsValid(_plugin) == 0)
            {
                // Instantiation failed; Close and stop.
                KlakHap_CloseDemuxer(_plugin);
                _plugin = IntPtr.Zero;
                return;
            }

            // Video properties
            _width = KlakHap_GetVideoWidth(_plugin);
            _height = KlakHap_GetVideoHeight(_plugin);
            _videoType = KlakHap_AnalyzeVideoType(_plugin);
            _duration = KlakHap_GetDuration(_plugin);
            _frameCount = KlakHap_CountFrames(_plugin);
        }

        public void Dispose()
        {
            if (_plugin != IntPtr.Zero)
            {
                KlakHap_CloseDemuxer(_plugin);
                _plugin = IntPtr.Zero;
            }
        }

        #endregion

        #region Public methods

        public void ReadFrame(ReadBuffer buffer, int index, float time)
        {
            KlakHap_ReadFrame(_plugin, index, buffer.PluginPointer);
            buffer.Index = index;
            buffer.Time = time;
        }

        #endregion

        #region Private members

        IntPtr _plugin;
        int _width, _height, _videoType;
        double _duration;
        int _frameCount;

        #endregion

        #region Native plugin entry points

        [DllImport("KlakHap")]
        internal static extern IntPtr KlakHap_OpenDemuxer(string filepath);

        [DllImport("KlakHap")]
        internal static extern void KlakHap_CloseDemuxer(IntPtr demuxer);

        [DllImport("KlakHap")]
        internal static extern int KlakHap_DemuxerIsValid(IntPtr demuxer);

        [DllImport("KlakHap")]
        internal static extern int KlakHap_CountFrames(IntPtr demuxer);

        [DllImport("KlakHap")]
        internal static extern double KlakHap_GetDuration(IntPtr demuxer);

        [DllImport("KlakHap")]
        internal static extern int KlakHap_GetVideoWidth(IntPtr demuxer);

        [DllImport("KlakHap")]
        internal static extern int KlakHap_GetVideoHeight(IntPtr demuxer);

        [DllImport("KlakHap")]
        internal static extern int KlakHap_AnalyzeVideoType(IntPtr demuxer);

        [DllImport("KlakHap")]
        internal static extern void KlakHap_ReadFrame(IntPtr demuxer, int frameNumber, IntPtr buffer);

        #endregion
    }
}
