using System.Collections.Generic;
using System.IO;
using UnityEngine;

namespace TestDataDownloader {

[CreateAssetMenu(fileName = "Dataset", menuName = "Klak/Hap/Test Dataset")]
public sealed class Dataset : ScriptableObject
{
    [field:SerializeField] public string[] SourceUrls { get; private set; }

    public string[] MissingFileUrls => GetMissingFileUrls();

    string[] GetMissingFileUrls()
    {
        var missingList = new List<string>();
        foreach (var url in SourceUrls)
        {
            var filename = FileUtils.UrlToFilename(url);
            if (filename == null) continue;
            var destPath = FileUtils.GetDestinationPath(filename);
            if (!File.Exists(destPath)) missingList.Add(url);
        }
        return missingList.ToArray();
    }
}

} // namespace TestDataDownloader
