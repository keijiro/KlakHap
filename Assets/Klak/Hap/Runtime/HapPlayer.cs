using UnityEngine;

namespace Klak.Hap
{
    [AddComponentMenu("Klak/HAP/HAP Player")]
    public sealed class HapPlayer : MonoBehaviour
    {
        #region Editable attributes

        public enum PathMode { StreamingAssets, LocalFileSystem }

        [SerializeField] PathMode _pathMode = PathMode.StreamingAssets;
        [SerializeField] string _filePath = "";

        [SerializeField] float _time = 0;
        [SerializeField, Range(-10, 10)] float _speed = 1;

        [SerializeField] RenderTexture _targetTexture = null;
        [SerializeField] Renderer _targetRenderer = null;
        [SerializeField] string _targetMaterialProperty = null;

        #endregion

        #region Public properties

        public float time {
            get { return _time; }
            set { _time = value; }
        }

        public float speed {
            get { return _speed; }
            set { _speed = value; }
        }

        public RenderTexture targetTexture {
            get { return _targetTexture; }
            set { _targetTexture = value; }
        }

        public Renderer targetRenderer {
            get { return _targetRenderer; }
            set { _targetRenderer = value; }
        }

        public string targetMaterialProperty {
            get { return _targetMaterialProperty; }
            set { _targetMaterialProperty = value; }
        }

        #endregion

        #region Private members

        Demuxer _demuxer;
        StreamReader _stream;
        Decoder _decoder;

        Texture2D _texture;
        TextureUpdater _updater;

        float _playbackTime;
        float _appliedSpeed;

        #endregion

        #region External object updaters

        Material _blitMaterial;
        MaterialPropertyBlock _propertyBlock;

        void UpdateTargetTexture()
        {
            if (_targetTexture == null) return;

            // Material lazy initialization
            if (_blitMaterial == null)
                _blitMaterial = new Material(Utility.DetermineShader(_demuxer.VideoType));

            // Blit
            Graphics.Blit(_texture, _targetTexture, _blitMaterial, 0);
        }

        void UpdateTargetRenderer()
        {
            if (_targetRenderer == null) return;

            // Material property block lazy initialization
            if (_propertyBlock == null)
                _propertyBlock = new MaterialPropertyBlock();

            // Read-modify-write
            _targetRenderer.GetPropertyBlock(_propertyBlock);
            _propertyBlock.SetTexture(_targetMaterialProperty, _texture);
            _targetRenderer.SetPropertyBlock(_propertyBlock);
        }

        #endregion

        #region MonoBehaviour implementation

        void Start()
        {
            // File path
            var path = _filePath;
            if (_pathMode == PathMode.StreamingAssets)
                path = System.IO.Path.Combine(Application.streamingAssetsPath, path);

            // Demuxer instantiation
            _demuxer = new Demuxer(path);

            if (!_demuxer.IsValid)
            {
                _demuxer.Dispose();
                _demuxer = null;
                return;
            }

            // Stream reader instantiation
            _stream = new StreamReader(_demuxer, _time, _speed / 60);
            _playbackTime = _time;
            _appliedSpeed = _speed;

            // Decoder instantiation
            _decoder = new Decoder(
                _stream, _demuxer.Width, _demuxer.Height, _demuxer.VideoType
            );

            // Texture initialization
            _texture = new Texture2D(
                _demuxer.Width, _demuxer.Height,
                Utility.DetermineTextureFormat(_demuxer.VideoType), false
            );
            _updater = new TextureUpdater(_texture, _decoder.CallbackID);
        }

        void OnDestroy()
        {
            if (_updater != null)
            {
                _updater.Dispose();
                _updater = null;
            }

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

            if (_texture != null)
            {
                Destroy(_texture);
                _texture = null;
            }

            if (_blitMaterial != null)
            {
                Destroy(_blitMaterial);
                _blitMaterial = null;
            }
        }

        void LateUpdate()
        {
            if (_demuxer == null) return;

            // Restart the stream reader when the time/speed were changed.
            if (_time != _playbackTime || _speed != _appliedSpeed)
            {
                _stream.Restart(_time, _speed / 60);
                _playbackTime = _time;
                _appliedSpeed = _speed;
            }

            // Decode and update
            _decoder.UpdateTime(_time);
            _updater.RequestUpdate();

            // Time advance
            _time += Time.deltaTime * _appliedSpeed;
            _playbackTime = _time;

            // External object updates
            UpdateTargetRenderer();
            UpdateTargetTexture();
        }

        #endregion
    }
}
