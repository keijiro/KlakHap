using System.Collections.Generic;
using System.IO;
using UnityEngine;

[CreateAssetMenu(fileName = "TestDataList", menuName = "Klak/Hap/Test Data List")]
public sealed class TestDataList : ScriptableObject
{
    [field:SerializeField] public string[] SourceUrls { get; private set; }

    public string[] MissingFileUrls => GetMissingFileUrls();

    string[] GetMissingFileUrls()
    {
        var missingList = new List<string>();
        foreach (var url in SourceUrls)
        {
            var filename = TestDataUtils.UrlToFilename(url);
            if (filename == null) continue;
            var destPath = TestDataUtils.GetDestinationPath(filename);
            if (!File.Exists(destPath)) missingList.Add(url);
        }
        return missingList.ToArray();
    }
}
