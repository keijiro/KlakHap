using UnityEngine;
using Klak.Hap;
using System.Collections;

class ScriptingTest : MonoBehaviour
{
    enum RetrievalMethod { TargetRenderer, TextureReference }

    [SerializeField] string _fileName = "Test.mov";
    [SerializeField] RetrievalMethod _method = RetrievalMethod.TargetRenderer;
    [SerializeField] float _switchInterval = 1;
    [SerializeField] int _iteration = 10;

    IEnumerator Start()
    {
        // Create a player and open the video source file.
        var player = gameObject.AddComponent<HapPlayer>();
        player.Open(_fileName);

        // Assign to the renderer.
        var renderer = GetComponent<Renderer>();
        if (_method == RetrievalMethod.TargetRenderer)
            player.targetRenderer = renderer;
        else
            renderer.material.mainTexture = player.texture;

        // Main iteration
        for (var i = 0; i < _iteration; i++)
        {
            // Randomly change the playback time with the given interval.
            yield return new WaitForSeconds(_switchInterval);
            player.time = Random.Range(0, (float)player.streamDuration - _switchInterval);
        }

        // Destroy and hide the player
        Destroy(player);
        renderer.enabled = false;
    }
}
