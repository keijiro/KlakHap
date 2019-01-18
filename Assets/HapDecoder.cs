using System;
using System.Runtime.InteropServices;
using UnityEngine;
using UnityEngine.Rendering;

namespace Klak.Hap
{
    class HapDecoder : MonoBehaviour
    {
        IntPtr _hap;
        Texture2D _texture;
        CommandBuffer _command;

        void Start()
        {
            _hap = HapOpen("Assets\\StreamingAssets\\Test.mov");

            _texture = new Texture2D(
                HapGetVideoWidth(_hap),
                HapGetVideoHeight(_hap),
                TextureFormat.DXT1,
                false
            );

            _command = new CommandBuffer();
        }

        void OnDestroy()
        {
            if (_hap != IntPtr.Zero)
            {
                HapClose(_hap);
                _hap = IntPtr.Zero;
            }

            Destroy(_texture);

            _command.Dispose();
        }

        void Update()
        {
            var time = 1 - Mathf.Abs(1 - Time.time / 3 % 1 * 2);
            var index = (int)(time * HapCountFrames(_hap));
            
            HapDecodeFrame(_hap, index);

            _command.IssuePluginCustomTextureUpdateV2(
                HapGetTextureUpdateCallback(), _texture, HapGetID(_hap)
            );
            Graphics.ExecuteCommandBuffer(_command);
            _command.Clear();

            GetComponent<Renderer>().material.mainTexture = _texture;
        }

        [DllImport("KlakHap")]
        internal static extern IntPtr HapGetTextureUpdateCallback();

        [DllImport("KlakHap")]
        internal static extern IntPtr HapOpen(string filepath);

        [DllImport("KlakHap")]
        internal static extern void HapClose(IntPtr context);

        [DllImport("KlakHap")]
        internal static extern uint HapGetID(IntPtr context);

        [DllImport("KlakHap")]
        internal static extern int HapCountFrames(IntPtr context);

        [DllImport("KlakHap")]
        internal static extern int HapGetVideoWidth(IntPtr context);

        [DllImport("KlakHap")]
        internal static extern int HapGetVideoHeight(IntPtr context);

        [DllImport("KlakHap")]
        internal static extern void HapDecodeFrame(IntPtr context, int index);
    }
}
