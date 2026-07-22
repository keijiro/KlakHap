using System.Collections.Generic;
using System.Linq;
using UnityEditor;
using UnityEngine;

namespace Klak.Hap
{
    // Edit-mode file-lock policy:
    // - Keep the demuxer open while _time is being scrubbed (normal preview).
    // - After a short idle period, close the native FILE* so Windows can
    //   overwrite the movie; the last decoded texture stays on screen.
    // - Always release on leaving play mode.
    [InitializeOnLoad]
    static class HapPlayerFileLockWatchdog
    {
        const double IdleCloseSeconds = 1.0;

        static readonly Dictionary<int, float> _lastTime =
            new Dictionary<int, float>();
        static readonly Dictionary<int, double> _lastActivity =
            new Dictionary<int, double>();

        static HapPlayerFileLockWatchdog()
        {
            EditorApplication.update += Update;
            EditorApplication.playModeStateChanged += OnPlayModeStateChanged;
        }

        static void OnPlayModeStateChanged(PlayModeStateChange state)
        {
            if (state != PlayModeStateChange.EnteredEditMode) return;

            foreach (var player in HapPlayer.Instances.ToArray())
            {
                if (player != null)
                    player.ReleaseFileLock();
            }

            _lastTime.Clear();
            _lastActivity.Clear();
        }

        static void Update()
        {
            if (EditorApplication.isPlayingOrWillChangePlaymode) return;

            var now = EditorApplication.timeSinceStartup;

            foreach (var player in HapPlayer.Instances.ToArray())
            {
                if (player == null) continue;
                if (EditorUtility.IsPersistent(player)) continue;
                if (string.IsNullOrEmpty(player.resolvedFilePath)) continue;

                var id = player.GetInstanceID();
                var time = player.time;

                if (!_lastTime.TryGetValue(id, out var previous) ||
                    !Mathf.Approximately(previous, time))
                {
                    _lastTime[id] = time;
                    _lastActivity[id] = now;

                    // Reopen (if idle-closed) and decode the scrubbed frame.
                    player.UpdateNow();
                    continue;
                }

                if (!player.isStreamOpen) continue;

                if (!_lastActivity.TryGetValue(id, out var activity))
                {
                    _lastActivity[id] = now;
                    continue;
                }

                if (now - activity >= IdleCloseSeconds)
                    player.ReleaseFileLock();
            }
        }
    }
}
