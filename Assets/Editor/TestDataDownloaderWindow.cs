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
        var window = GetWindow<TestDataDownloaderWindow>(true, "Test Video Missing");
        window.minSize = new Vector2(420, 200);
    }

    void OnGUI()
    {
        var missingUrls = GetMissingUrls(_testDataList);
        if (missingUrls.Length == 0)
        {
            Close();
            return;
        }

        EditorGUILayout.Space(12);
        EditorGUILayout.LabelField(
            "Some test videos are missing. Please download the files.",
            EditorStyles.wordWrappedLabel);

        EditorGUILayout.Space(8);
        DrawFileList(missingUrls);

        GUILayout.FlexibleSpace();
        using (new EditorGUILayout.HorizontalScope())
        {
            GUILayout.FlexibleSpace();
            if (GUILayout.Button("Close", GUILayout.Height(28))) Close();
        }
    }

    void DrawFileList(string[] urls)
    {
        foreach (var url in urls)
        {
            using (new EditorGUILayout.HorizontalScope())
            {
                var fileName = GetFileName(url);
                EditorGUILayout.LabelField(fileName, GUILayout.MinWidth(120));
                GUILayout.FlexibleSpace();
                using (new EditorGUI.DisabledScope(_activeDownloads.Contains(url)))
                    if (GUILayout.Button("Download", GUILayout.Width(120)))
                        DownloadAsync(url);
            }
        }
    }

    async void DownloadAsync(string url)
    {
        if (!_activeDownloads.Add(url)) return;

        var fileName = GetFileName(url);
        if (string.IsNullOrEmpty(fileName))
        {
            _activeDownloads.Remove(url);
            return;
        }

        var path = GetDestinationPath(fileName);
        var tempPath = Path.Combine(
            Application.temporaryCachePath,
            $"{fileName}.{Guid.NewGuid():N}.tmp");

        var success = false;

        using (var request = UnityWebRequest.Get(url))
        {
            request.downloadHandler = new DownloadHandlerFile(tempPath);

            try
            {
                await Awaitable.FromAsyncOperation(request.SendWebRequest());
                success = request.result == UnityWebRequest.Result.Success;
                if (success)
                    success = MoveToDestination(tempPath, path);
                else if (File.Exists(tempPath))
                    File.Delete(tempPath);
            }
            finally
            {
                _activeDownloads.Remove(url);
            }
        }

        if (success)
            AssetDatabase.Refresh();
        Repaint();
    }

    static TestDataList FindTestDataList()
    {
        var guids = AssetDatabase.FindAssets("t:TestDataList");
        if (guids.Length == 0) return null;
        var path = AssetDatabase.GUIDToAssetPath(guids[0]);
        return AssetDatabase.LoadAssetAtPath<TestDataList>(path);
    }

    static string[] GetMissingUrls(TestDataList list)
    {
        if (list.SourceUrls == null || list.SourceUrls.Length == 0)
            return Array.Empty<string>();

        var missing = new List<string>();
        foreach (var url in list.SourceUrls)
        {
            if (string.IsNullOrEmpty(url)) continue;
            var fileName = GetFileName(url);
            if (string.IsNullOrEmpty(fileName)) continue;
            if (!File.Exists(GetDestinationPath(fileName)))
                missing.Add(url);
        }

        return missing.ToArray();
    }

    static string GetFileName(string url)
    {
        if (Uri.TryCreate(url, UriKind.Absolute, out var uri))
            return Path.GetFileName(uri.LocalPath);
        return Path.GetFileName(url);
    }

    static string GetDestinationPath(string fileName)
        => Path.Combine(Application.dataPath, "StreamingAssets", fileName);

    static bool MoveToDestination(string tempPath, string destinationPath)
    {
        try
        {
            Directory.CreateDirectory(Path.GetDirectoryName(destinationPath));
            if (File.Exists(destinationPath))
                File.Delete(destinationPath);
            File.Move(tempPath, destinationPath);
            return true;
        }
        catch
        {
            if (File.Exists(tempPath))
                File.Delete(tempPath);
            return false;
        }
    }
}
