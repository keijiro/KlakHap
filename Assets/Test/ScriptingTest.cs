using UnityEngine;
using Klak.Hap;
using System.Collections;

class ScriptingTest : MonoBehaviour
{
    enum RetrievalMethod { TargetRenderer, TextureReference }

    [SerializeField] string _fileName = "Test.mov";
    [SerializeField] RetrievalMethod _method = RetrievalMethod.TargetRenderer;

    IEnumerator Start()
    {
        var player = gameObject.AddComponent<HapPlayer>();
        player.Open(_fileName);

        var renderer = GetComponent<Renderer>();

        if (_method == RetrievalMethod.TargetRenderer)
            player.targetRenderer = renderer;
        else
            renderer.material.mainTexture = player.texture;

        for (var i = 0; i < 20; i++)
        {
            yield return new WaitForSeconds(0.5f);
            player.time = Random.Range(0, (float)player.streamDuration - 0.5f);
        }

        Destroy(player);
        renderer.enabled = false;
    }
}
