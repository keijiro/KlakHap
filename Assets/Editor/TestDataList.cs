using UnityEngine;

[CreateAssetMenu(fileName = "TestDataList", menuName = "Klak/Hap/Test Data List")]
public sealed class TestDataList : ScriptableObject
{
    [field:SerializeField] public string[] SourceUrls { get; private set; }
}
