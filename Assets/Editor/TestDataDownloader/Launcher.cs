using UnityEditor;
using UnityEngine;

namespace TestDataDownloader {

[InitializeOnLoad]
static class Launcher
{
    static Launcher()
      => EditorApplication.delayCall += OnDelayedSetup;

    static void OnDelayedSetup()
    {
        // Check session state.
        const string sessionKey = "TestDataDownloader.Shown";
        if (SessionState.GetBool(sessionKey, false)) return;
        SessionState.SetBool(sessionKey, true);

        // Try to show the missing file list window:
        // It will be shown only when there are missing files.
        MissingFileListWindow.TryShowWindow();
    }
}

} // namespace TestDataDownloader
