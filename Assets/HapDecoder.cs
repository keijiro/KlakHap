using System;
using System.IO;
using System.Collections.Concurrent;
using System.Runtime.InteropServices;
using System.Threading;
using UnityEngine;
using UnityEngine.Rendering;

namespace Klak.Hap
{
    class HapDecoder : MonoBehaviour
    {
        #region Editable attributes

        [SerializeField] string _fileName = "Test.mov";

        #endregion

        #region Private members

        static uint _instantiationCount;

        IntPtr _demuxer;
        IntPtr _decoder;
        uint _decoderID;

        double _duration;
        int _totalFrames;

        Texture2D _texture;
        CommandBuffer _updateCommand;

        #endregion

        #region Internal use functions

        static TextureFormat VideoTypeToTextureFormat(int type)
        {
            switch (type & 0xf)
            {
                case 0xb: return TextureFormat.DXT1;
                case 0xe: return TextureFormat.DXT5;
                case 0xf: return TextureFormat.DXT5;
                case 0xc: return TextureFormat.BC7;
                case 0x1: return TextureFormat.BC4;
            }
            return TextureFormat.DXT1;
        }

        #endregion

        #region Decoder thread

        Thread _thread;
        AutoResetEvent _decodeRequest;
        int _decodeFrame;

        void StartDecoderThread()
        {
            _decodeRequest = new AutoResetEvent(false);
            _decodeFrame = 0;

            _thread = new Thread(DecoderThread);
            _thread.Start();
        }

        void DecoderThread()
        {
            var buffer = KlakHap_CreateReadBuffer();

            while (true)
            {
                _decodeRequest.WaitOne();
                if (_decodeFrame < 0) break;
                KlakHap_ReadFrame(_demuxer, _decodeFrame, buffer);
                KlakHap_DecodeFrame(_decoder, buffer);
            }

            KlakHap_DestroyReadBuffer(buffer);
        }

        #endregion

        #region MonoBehaviour implementation

        void Start()
        {
            // Demuxer instantiation
            var path = Path.Combine(Application.streamingAssetsPath, _fileName);
            _demuxer = KlakHap_OpenDemuxer(path);

            if (KlakHap_DemuxerIsValid(_demuxer) == 0)
            {
                // Instantiation failed; Close and stop.
                KlakHap_CloseDemuxer(_demuxer);
                _demuxer = IntPtr.Zero;
                return;
            }

            // Video properties
            var width = KlakHap_GetVideoWidth(_demuxer);
            var height = KlakHap_GetVideoHeight(_demuxer);
            var typeID = KlakHap_AnalyzeVideoType(_demuxer);
            _duration = KlakHap_GetDuration(_demuxer);
            _totalFrames = KlakHap_CountFrames(_demuxer);

            // Decoder instantiation
            _decoder = KlakHap_CreateDecoder(width, height, typeID);
            _decoderID = ++_instantiationCount;
            KlakHap_AssignDecoder(_decoderID, _decoder);

            // Texture initialization
            var format = VideoTypeToTextureFormat(typeID);
            _texture = new Texture2D(width, height, format, false);

            // Replace a renderer texture with our one.
            GetComponent<Renderer>().material.mainTexture = _texture;

            // Command buffer initialization
            _updateCommand = new CommandBuffer();
            _updateCommand.name = "Klak HAP";
            _updateCommand.IssuePluginCustomTextureUpdateV2(
                KlakHap_GetTextureUpdateCallback(), _texture, _decoderID
            );

            // Start a decoder thead; Immediately decodes the first frame.
            StartDecoderThread();
        }

        void OnDestroy()
        {
            // Decoder thread termination
            if (_thread != null)
            {
                _decodeFrame = -1;
                _decodeRequest.Set();
                _thread.Join();
            }

            // Decoder destruction
            if (_decoder != IntPtr.Zero)
            {
                KlakHap_AssignDecoder(_decoderID, IntPtr.Zero);
                KlakHap_DestroyDecoder(_decoder);
            }

            // Demuxer destruction
            if (_demuxer != IntPtr.Zero) KlakHap_CloseDemuxer(_demuxer);

            // Misc resources
            if (_texture != null) Destroy(_texture);
            if (_updateCommand != null) _updateCommand.Dispose();
        }

        void Update()
        {
            if (_demuxer == IntPtr.Zero || _decoder == IntPtr.Zero) return;

            // Frame index calculation
            var index = (int)(Time.time * _totalFrames / _duration);
            index %= _totalFrames;

            // Do nothing if it's same to the previous frame.
            if (index == _decodeFrame) return;

            // Decoding request
            _decodeFrame = index;
            _decodeRequest.Set();

            // Texture update command execution
            Graphics.ExecuteCommandBuffer(_updateCommand);
        }

        #endregion

        #region Native plugin entry points

        [DllImport("KlakHap")]
        internal static extern IntPtr KlakHap_GetTextureUpdateCallback();

        [DllImport("KlakHap")]
        internal static extern IntPtr KlakHap_CreateReadBuffer();

        [DllImport("KlakHap")]
        internal static extern void KlakHap_DestroyReadBuffer(IntPtr buffer);

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

        [DllImport("KlakHap")]
        internal static extern IntPtr KlakHap_CreateDecoder(int width, int height, int typeID);

        [DllImport("KlakHap")]
        internal static extern void KlakHap_DestroyDecoder(IntPtr decoder);

        [DllImport("KlakHap")]
        internal static extern void KlakHap_AssignDecoder(uint id, IntPtr decoder);

        [DllImport("KlakHap")]
        internal static extern void KlakHap_DecodeFrame(IntPtr decoder, IntPtr input);

        #endregion
    }
}
