using UnityEditor;
using UnityEngine;

[InitializeOnLoad]
static class TestDataDownloader
{
    static TestDataDownloader()
      => EditorApplication.delayCall += OnDelayedSetup;

    static void OnDelayedSetup()
    {
        // Check session state.
        const string sessionKey = "KlakHap.TestDataDownloaderShown";
        if (SessionState.GetBool(sessionKey, false)) return;
        SessionState.SetBool(sessionKey, true);

        // Try to show the downloader window:
        // It will be shown only when there are missing files.
        TestDataDownloaderWindow.TryShowWindow();
    }
}
