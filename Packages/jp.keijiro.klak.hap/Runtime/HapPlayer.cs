using System.Collections.Generic;
using UnityEngine;
using UnityEngine.Playables;

#if KLAKHAP_HAS_TIMELINE
using UnityEngine.Timeline;
#endif

namespace Klak.Hap
{
    [ExecuteInEditMode, AddComponentMenu("Klak/HAP/HAP Player")]
    #if KLAKHAP_HAS_TIMELINE
    public sealed class HapPlayer : MonoBehaviour , ITimeControl, IPropertyPreview
    #else
    public sealed class HapPlayer : MonoBehaviour
    #endif
    {
        #region Editable attributes

        public enum PathMode { StreamingAssets, LocalFileSystem }

        [SerializeField] PathMode _pathMode = PathMode.StreamingAssets;
        [SerializeField] string _filePath = "";

        [SerializeField] float _time = 0;
        [SerializeField, Range(-10, 10)] float _speed = 1;
        [SerializeField] bool _loop = true;

        [SerializeField] RenderTexture _targetTexture = null;
        [SerializeField] Renderer _targetRenderer = null;
        [SerializeField] string _targetMaterialProperty = "_MainTex";

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

        public bool loop {
            get { return _loop; }
            set { _loop = value; }
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

        #region Read-only properties

        // True after a successful open for the current path. Stays true while
        // the demuxer is temporarily released in the editor (idle file unlock).
        public bool isValid { get { return _hasSource; } }
        public bool isStreamOpen { get { return _demuxer != null; } }

        public int frameWidth { get { return _width; } }
        public int frameHeight { get { return _height; } }
        public int frameCount { get { return _frameCount; } }
        public double streamDuration { get { return _duration; } }

        public CodecType codecType {
            get { return Utility.DetermineCodecType(_videoType); }
        }

        public string resolvedFilePath { get {
            if (_pathMode == PathMode.StreamingAssets)
                return System.IO.Path.Combine(Application.streamingAssetsPath, _filePath);
            else
                return _filePath;
        } }

        public Texture2D texture { get { return _texture; } }

        #endregion

        #region Public methods

        public void Open(string filePath, PathMode pathMode = PathMode.StreamingAssets)
        {
            if (_demuxer != null)
            {
                Debug.LogError("Stream has already been opened.");
                return;
            }

            _filePath = filePath;
            _pathMode = pathMode;
            _suppressEditModeAutoOpen = false;

            OpenInternal();
        }

        public void UpdateNow()
        {
            // Scrubbing / forced preview must be allowed to reopen after idle unlock.
            _suppressEditModeAutoOpen = false;
            LateUpdate();
        }

        // Closes the native demuxer (and reader/decoder) so Windows can
        // overwrite the movie file, while keeping the last decoded preview.
        public void ReleaseFileLock()
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

            // In edit mode, stay closed until time is scrubbed again.
            if (!Application.isPlaying)
                _suppressEditModeAutoOpen = true;
        }

        #endregion

        #region Internal editor support

        static readonly HashSet<HapPlayer> _instances = new HashSet<HapPlayer>();

        public static IReadOnlyCollection<HapPlayer> Instances => _instances;

        #endregion

        #region Private members

        Demuxer _demuxer;
        StreamReader _stream;
        Decoder _decoder;

        Texture2D _texture;
        TextureUpdater _updater;

        float _storedTime;
        float _storedSpeed;

        bool _hasSource;
        int _width;
        int _height;
        int _frameCount;
        int _videoType;
        double _duration;

        // After an idle unlock in edit mode, don't reopen every LateUpdate.
        bool _suppressEditModeAutoOpen;

        void OpenInternal()
        {
            // Demuxer instantiation
            _demuxer = new Demuxer(resolvedFilePath);

            if (!_demuxer.IsValid)
            {
                if (Application.isPlaying)
                {
                    // In play mode, show an error message, then disable itself
                    // to prevent spamming the console.
                    Debug.LogError("Failed to open stream (" + resolvedFilePath + ").");
                    enabled = false;
                }
                _demuxer.Dispose();
                _demuxer = null;
                _hasSource = false;
                _width = _height = _frameCount = _videoType = 0;
                _duration = 0;
                return;
            }

            _width = _demuxer.Width;
            _height = _demuxer.Height;
            _frameCount = _demuxer.FrameCount;
            _duration = _demuxer.Duration;
            _videoType = _demuxer.VideoType;
            _hasSource = true;

            // Stream reader instantiation
            _stream = new StreamReader(_demuxer, _time, _speed / 60);
            (_storedTime, _storedSpeed) = (_time, _speed);

            // Decoder instantiation
            _decoder = new Decoder(
                _stream, _width, _height, _videoType
            );

            // Texture initialization (reuse when scrub-reopening the same size)
            var format = Utility.DetermineTextureFormat(_videoType);
            if (_texture != null &&
                (_texture.width != _width ||
                 _texture.height != _height ||
                 _texture.format != format))
            {
                Utility.Destroy(_texture);
                _texture = null;
            }

            if (_texture == null)
            {
                _texture = new Texture2D(_width, _height, format, false);
                _texture.wrapMode = TextureWrapMode.Clamp;
                _texture.hideFlags = HideFlags.DontSave;
            }

            _updater = new TextureUpdater(_texture, _decoder);
            _suppressEditModeAutoOpen = false;
        }

        #endregion

        #region External object updaters

        Material _blitMaterial;
        MaterialPropertyBlock _propertyBlock;

        void UpdateTargetTexture()
        {
            if (_targetTexture == null || _texture == null) return;

            // Material lazy initialization
            if (_blitMaterial == null)
            {
                _blitMaterial = new Material(Utility.DetermineBlitShader(_videoType));
                _blitMaterial.hideFlags = HideFlags.DontSave;
            }

            // Blit
            Graphics.Blit(_texture, _targetTexture, _blitMaterial, 0);
        }

        void UpdateTargetRenderer()
        {
            if (_targetRenderer == null || _texture == null) return;

            // Material property block lazy initialization
            if (_propertyBlock == null)
                _propertyBlock = new MaterialPropertyBlock();

            // Read-modify-write
            _targetRenderer.GetPropertyBlock(_propertyBlock);
            _propertyBlock.SetTexture(_targetMaterialProperty, _texture);
            _targetRenderer.SetPropertyBlock(_propertyBlock);
        }

        #endregion

        #region ITimeControl implementation

        bool _externalTime;

        public void OnControlTimeStart()
        {
            _externalTime = true;

            // In the external time mode, we can't know the actual playback
            // speed but sure that it's positive (Control Track doesn't support
            // reverse playback), so we assume that the speed is 1.0.
            // Cons: Resync could happen every frame for high speed play back.
            _speed = 1;
        }

        public void OnControlTimeStop()
        {
            _externalTime = false;
        }

        public void SetTime(double time)
        {
            _time = (float)time;
            _speed = 1;
            _suppressEditModeAutoOpen = false;
        }

        #endregion

        #region IPropertyPreview implementation

        #if KLAKHAP_HAS_TIMELINE
        public void GatherProperties(PlayableDirector director, IPropertyCollector driver)
        {
            driver.AddFromName<HapPlayer>(gameObject, "_time");
        }
        #endif

        #endregion

        #region MonoBehaviour implementation

        void OnEnable()
        {
            _instances.Add(this);
        }

        void OnDisable()
        {
            _instances.Remove(this);

            // Drop the native file handle when leaving play mode / disabling
            // in the editor. Play mode entry also hits this; LateUpdate reopens.
            if (!Application.isPlaying)
                ReleaseFileLock();
        }

        void OnDestroy()
        {
            ReleaseFileLock();

            // Allow LateUpdate to reopen after inspector path reloads, which
            // force-invoke OnDestroy then LateUpdate.
            _suppressEditModeAutoOpen = false;

            _hasSource = false;
            _width = _height = _frameCount = _videoType = 0;
            _duration = 0;

            Utility.Destroy(_texture);
            Utility.Destroy(_blitMaterial);
            _texture = null;
            _blitMaterial = null;
        }

        int _lastUpdateFrameCount = -1;

        void LateUpdate()
        {
            // Double update check
            if (Time.frameCount == _lastUpdateFrameCount) return;
            _lastUpdateFrameCount = Time.frameCount;

            // Lazy initialization of demuxer
            if (_demuxer == null && !string.IsNullOrEmpty(_filePath))
            {
                if (Application.isPlaying || !_suppressEditModeAutoOpen)
                    OpenInternal();
            }

            // Do nothing if the demuxer hasn't been instantiated.
            if (_demuxer == null) return;

            var duration = (float)_duration;

            // Check if _time is still in the same frame of _storedTime.
            // Resync is needed when it went out of the frame.
            var dt = duration / _frameCount;
            var resync = _time < _storedTime || _time > _storedTime + dt;

            // Check if the speed was externally modified.
            if (_speed != _storedSpeed)
            {
                resync = true; // Resync to adapt to the new speed.
                _storedSpeed = _speed;
            }

            // Time clamping
            var t = _loop ? _time : Mathf.Clamp(_time, 0, duration - 1e-4f);

            // Determine if background decoding is available.
            // Resync shouldn't happen. Not preferable in edit mode.
            var bgdec = !resync && Application.isPlaying;

            // Restart the stream reader on resync.
            if (resync) _stream.Restart(t, _speed / 60);

            if (TextureUpdater.AsyncSupport)
            {
                // Asynchronous texture update supported:
                // Decode a frame and request a texture update.
                if (bgdec) _decoder.UpdateAsync(t); else _decoder.UpdateSync(t);
                _updater.RequestAsyncUpdate();
            }
            #if !HAP_NO_DELAY
            else if (bgdec)
            {
                // Synchronous texture update with background decoding:
                // Update first, then start background decoding. This
                // introduces a single frame delay but makes it possible to
                // offload decoding load to a background thread.
                _updater.UpdateNow();
                _decoder.UpdateAsync(t);
            }
            #endif
            else
            {
                // Synchronous decoding and texture update.
                _decoder.UpdateSync(t);
                _updater.UpdateNow();
            }

            // Update the stored time.
            if (Application.isPlaying && !_externalTime)
                _time += Time.deltaTime * _speed;
            _storedTime = _time;

            // External object updates
            UpdateTargetRenderer();
            UpdateTargetTexture();
        }

        #endregion
    }
}
