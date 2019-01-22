using System;
using System.IO;
using System.Runtime.InteropServices;
using UnityEngine;
using UnityEngine.Rendering;

namespace Klak.Hap
{
    sealed class HapDecoder : MonoBehaviour
    {
        #region Editable attributes

        [SerializeField] string _fileName = "Test.mov";
        [SerializeField, Range(-10, 10)] float _speed = 1;

        #endregion

        #region Private members

        Demuxer _demuxer;
        StreamReader _stream;
        Decoder _decoder;

        Texture2D _texture;
        CommandBuffer _updateCommand;

        float _time;
        float _appliedSpeed;

        #endregion

        #region MonoBehaviour implementation

        void Start()
        {
            // Demuxer instantiation
            var path = Path.Combine(Application.streamingAssetsPath, _fileName);
            _demuxer = new Demuxer(path);

            if (!_demuxer.IsValid)
            {
                _demuxer.Dispose();
                _demuxer = null;
                return;
            }

            // Stream reader instantiation
            _stream = new StreamReader(_demuxer, _time, _speed / 60);
            _appliedSpeed = _speed;

            // Decoder instantiation
            _decoder = new Decoder(
                _stream, _demuxer.Width, _demuxer.Height, _demuxer.VideoType
            );

            // Texture initialization
            _texture = new Texture2D(
                _demuxer.Width, _demuxer.Height, _demuxer.TextureFormat, false
            );

            // Replace a renderer texture with our one.
            GetComponent<Renderer>().material.mainTexture = _texture;

            // Command buffer initialization
            _updateCommand = new CommandBuffer();
            _updateCommand.name = "Klak HAP";
            _updateCommand.IssuePluginCustomTextureUpdateV2(
                KlakHap_GetTextureUpdateCallback(),
                _texture, _decoder.CallbackID
            );
        }

        void OnDestroy()
        {
            if (_decoder != null)
            {
                _decoder.Dispose();
                _decoder = null;
            }

            if (_stream != null)
            {
                _stream.Dispose();
                _stream = null;
            }

            if (_demuxer != null)
            {
                _demuxer.Dispose();
                _demuxer = null;
            }

            if (_texture != null) Destroy(_texture);
            if (_updateCommand != null) _updateCommand.Dispose();
        }

        void Update()
        {
            if (_demuxer == null) return;

            // Restart the stream reader when the speed value was changed.
            if (_speed != _appliedSpeed)
            {
                _stream.Restart(_time, _speed / 60);
                _appliedSpeed = _speed;
            }

            _decoder.UpdateTime(_time);
            Graphics.ExecuteCommandBuffer(_updateCommand);

            _time += Time.deltaTime * _appliedSpeed;
        }

        #endregion

        #region Native plugin entry points

        [DllImport("KlakHap")]
        internal static extern IntPtr KlakHap_GetTextureUpdateCallback();

        #endregion
    }
}
