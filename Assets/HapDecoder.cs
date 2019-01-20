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

        uint _id;
        IntPtr _decoder;

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
            while (true)
            {
                _decodeRequest.WaitOne();
                if (_decodeFrame < 0) break;
                KlakHap_DecodeFrame(_decoder, _decodeFrame);
            }
        }

        #endregion

        #region MonoBehaviour implementation

        void Start()
        {
            // Unique ID
            _id = ++_instantiationCount;

            // Decoder instantiation
            var path = Path.Combine(Application.streamingAssetsPath, _fileName);
            _decoder = KlakHap_Open(path);

            if (KlakHap_IsValid(_decoder) == 0)
            {
                // Instantiation failed; Close and stop.
                KlakHap_Close(_decoder);
                _decoder = IntPtr.Zero;
                return;
            }

            // ID to decoder instance mapping
            KlakHap_MapInstance(_id, _decoder);

            // Video properties
            _duration = KlakHap_GetDuration(_decoder);
            _totalFrames = KlakHap_CountFrames(_decoder);

            // Texture initialization
            _texture = new Texture2D(
                KlakHap_GetVideoWidth(_decoder),
                KlakHap_GetVideoHeight(_decoder),
                VideoTypeToTextureFormat(KlakHap_AnalyzeVideoType(_decoder)),
                false
            );

            // Replace a renderer texture with our one.
            GetComponent<Renderer>().material.mainTexture = _texture;

            // Command buffer initialization
            _updateCommand = new CommandBuffer();
            _updateCommand.name = "Klak HAP";
            _updateCommand.IssuePluginCustomTextureUpdateV2(
                KlakHap_GetTextureUpdateCallback(), _texture, _id
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

            // Decoder instance destruction
            if (_decoder != IntPtr.Zero)
            {
                KlakHap_MapInstance(_id, IntPtr.Zero);
                KlakHap_Close(_decoder);
            }

            // Misc resources
            if (_texture != null) Destroy(_texture);
            if (_updateCommand != null) _updateCommand.Dispose();
        }

        void Update()
        {
            if (_decoder == IntPtr.Zero) return;

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
        internal static extern void KlakHap_MapInstance(uint id, IntPtr decoder);

        [DllImport("KlakHap")]
        internal static extern IntPtr KlakHap_Open(string filepath);

        [DllImport("KlakHap")]
        internal static extern void KlakHap_Close(IntPtr decoder);

        [DllImport("KlakHap")]
        internal static extern int KlakHap_IsValid(IntPtr decoder);

        [DllImport("KlakHap")]
        internal static extern int KlakHap_CountFrames(IntPtr decoder);

        [DllImport("KlakHap")]
        internal static extern double KlakHap_GetDuration(IntPtr decoder);

        [DllImport("KlakHap")]
        internal static extern int KlakHap_GetVideoWidth(IntPtr decoder);

        [DllImport("KlakHap")]
        internal static extern int KlakHap_GetVideoHeight(IntPtr decoder);

        [DllImport("KlakHap")]
        internal static extern int KlakHap_AnalyzeVideoType(IntPtr decoder);

        [DllImport("KlakHap")]
        internal static extern void KlakHap_DecodeFrame(IntPtr decoder, int index);

        #endregion
    }
}
