using System;
using System.Runtime.InteropServices;
using UnityEngine;
using UnityEngine.Rendering;

namespace Klak.Hap
{
    internal sealed class TextureUpdater : IDisposable
    {
        #region Public properties

        public static bool AsyncSupport { get {
            return SystemInfo.graphicsDeviceType == GraphicsDeviceType.Direct3D11;
        } }

        #endregion

        #region Public methods

        public TextureUpdater(Texture2D texture, Decoder decoder)
        {
            _texture = texture;
            _decoder = decoder;

            if (AsyncSupport)
            {
                _command = new CommandBuffer();
                _command.name = "Klak HAP";
                _command.IssuePluginCustomTextureUpdateV2(
                    KlakHap_GetTextureUpdateCallback(),
                    texture, decoder.CallbackID
                );
            }
        }

        public void Dispose()
        {
            if (_command != null)
            {
                _command.Dispose();
                _command = null;
            }
        }

        public void UpdateNow()
        {
            _texture.LoadRawTextureData(
                _decoder.LockBuffer(),
                _decoder.BufferSize
            );
            _texture.Apply();
            _decoder.UnlockBuffer();
        }

        public void RequestAsyncUpdate()
        {
            if (_command != null) Graphics.ExecuteCommandBuffer(_command);
        }

        #endregion

        #region Private fields

        Texture2D _texture;
        Decoder _decoder;
        CommandBuffer _command;

        #endregion

        #region Native plugin entry points

        [DllImport("KlakHap")]
        internal static extern IntPtr KlakHap_GetTextureUpdateCallback();

        #endregion
    }
}
