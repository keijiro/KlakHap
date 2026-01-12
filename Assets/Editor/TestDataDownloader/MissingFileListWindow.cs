using System;
using System.Collections.Generic;
using System.IO;
using UnityEditor;
using UnityEngine;
using UnityEngine.Networking;

namespace TestDataDownloader {

sealed class MissingFileListWindow : EditorWindow
{
    [SerializeField] Dataset _dataset = null;

    public static void TryShowWindow()
    {
        var window = GetWindow<MissingFileListWindow>(true, "Missing Test Files");
        window.minSize = window.maxSize = new Vector2(400, 200);
    }

    void OnGUI()
    {
        var missingUrls = _dataset.MissingFileUrls;

        // Close immediately if there are no missing files.
        if (missingUrls.Length == 0)
        {
            Close();
            return;
        }

        // Message label
        EditorGUILayout.Space(12);
        EditorGUILayout.LabelField("Some test files are missing.");

        // Missing file list
        EditorGUILayout.Space(8);
        DrawFileList(missingUrls);
    }

    // HashSet to track active downloads
    readonly HashSet<string> _activeDownloads = new HashSet<string>();

    // Missing file list along with download buttons
    void DrawFileList(string[] urls)
    {
        foreach (var url in urls)
        {
            var isActive = _activeDownloads.Contains(url);
            var buttonLabel = isActive ? "Downloading..." : "Download";
            var buttonWidth = GUILayout.Width(120);

            EditorGUILayout.BeginHorizontal();

            // Filename label
            EditorGUILayout.LabelField(FileUtils.UrlToFilename(url));

            // Download button
            EditorGUI.BeginDisabledGroup(isActive);
            if (GUILayout.Button(buttonLabel, buttonWidth)) DownloadAsync(url);
            EditorGUI.EndDisabledGroup();

            EditorGUILayout.EndHorizontal();
        }
    }

    // Asynchronous file download method
    async void DownloadAsync(string url)
    {
        if (!_activeDownloads.Add(url)) return;

        var filename = FileUtils.UrlToFilename(url);
        var destPath = FileUtils.GetDestinationPath(filename);
        var tempPath = FileUtils.GetTemporaryPath(filename);

        var success = false;

        using (var request = UnityWebRequest.Get(url))
        {
            request.downloadHandler = new DownloadHandlerFile(tempPath);
            await Awaitable.FromAsyncOperation(request.SendWebRequest());
            success = (request.result == UnityWebRequest.Result.Success);
        }

        if (success)
        {
            File.Move(tempPath, destPath);
            AssetDatabase.Refresh();
        }
        else
        {
            Debug.LogError($"Failed to download test data file: {url}");
            _activeDownloads.Remove(url);
        }

        Repaint();
    }
}

} // namespace TestDataDownloader
