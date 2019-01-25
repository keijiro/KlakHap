using System;
using System.Runtime.InteropServices;
using UnityEngine;
using UnityEngine.Rendering;

namespace Klak.Hap
{
    internal sealed class TextureUpdater : IDisposable
    {
        #region Public methods

        public TextureUpdater(Texture texture, uint callbackID)
        {
            _command = new CommandBuffer();
            _command.name = "Klak HAP";
            _command.IssuePluginCustomTextureUpdateV2(
                KlakHap_GetTextureUpdateCallback(), texture, callbackID
            );
        }

        public void Dispose()
        {
            if (_command != null)
            {
                _command.Dispose();
                _command = null;
            }
        }

        public void RequestUpdate()
        {
            Graphics.ExecuteCommandBuffer(_command);
        }

        #endregion

        #region Private fields

        CommandBuffer _command;

        #endregion

        #region Native plugin entry points

        [DllImport("KlakHap")]
        internal static extern IntPtr KlakHap_GetTextureUpdateCallback();

        #endregion
    }
}
