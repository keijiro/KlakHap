using System;
using System.Collections.Generic;
using System.IO;
using UnityEditor;
using UnityEngine;
using UnityEngine.Networking;

sealed class TestDataDownloaderWindow : EditorWindow
{
    [SerializeField] TestDataList _testDataList = null;

    readonly HashSet<string> _activeDownloads = new HashSet<string>();

    public static void TryShowWindow()
    {
        var window = GetWindow<TestDataDownloaderWindow>(true, "Missing Test Files");
        window.minSize = new Vector2(400, 200);
    }

    void OnGUI()
    {
        var missingUrls = _testDataList.MissingFileUrls;
        if (missingUrls.Length == 0)
        {
            Close();
            return;
        }

        EditorGUILayout.Space(12);
        EditorGUILayout.LabelField("Some test files are missing.");

        EditorGUILayout.Space(8);
        DrawFileList(missingUrls);
    }

    void DrawFileList(string[] urls)
    {
        foreach (var url in urls)
        {
            var isActive = _activeDownloads.Contains(url);
            var buttonLabel = isActive ? "Downloading..." : "Download";
            var buttonWidth = GUILayout.Width(120);

            EditorGUILayout.BeginHorizontal();

            // Filename label
            EditorGUILayout.LabelField(TestDataUtils.UrlToFilename(url));

            // Download button
            EditorGUI.BeginDisabledGroup(isActive);
            if (GUILayout.Button(buttonLabel, buttonWidth)) DownloadAsync(url);
            EditorGUI.EndDisabledGroup();

            EditorGUILayout.EndHorizontal();
        }
    }

    async void DownloadAsync(string url)
    {
        if (!_activeDownloads.Add(url)) return;

        var filename = TestDataUtils.UrlToFilename(url);
        var path = TestDataUtils.GetDestinationPath(filename);
        var tempPath = TestDataUtils.GetTemporaryPath(filename);

        var success = false;
        using (var request = UnityWebRequest.Get(url))
        {
            request.downloadHandler = new DownloadHandlerFile(tempPath);
            await Awaitable.FromAsyncOperation(request.SendWebRequest());
            success = (request.result == UnityWebRequest.Result.Success);
        }

        if (success)
        {
            File.Move(tempPath, path);
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
