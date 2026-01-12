using System;
using System.IO;
using UnityEngine;

namespace TestDataDownloader {

static class FileUtils
{
    public static string UrlToFilename(string url)
    {
        if (Uri.TryCreate(url, UriKind.Absolute, out var uri))
            return Path.GetFileName(uri.LocalPath);
        return null;
    }

    public static string GetDestinationPath(string filename)
      => Path.Combine(Application.streamingAssetsPath, filename);

    public static string GetTemporaryPath(string filename)
      => Path.Combine(Application.temporaryCachePath, filename);
}

} // namespace TestDataDownloader
