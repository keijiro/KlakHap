using UnityEngine;
using Klak.Hap;

public sealed class ScriptingTest : MonoBehaviour
{
    [SerializeField] string _filename = "Test.mov";
    [SerializeField] RenderTexture _destination = null;
    [SerializeField] float _interval = 1;
    [SerializeField] int _iteration = 10;

    async void Start()
    {
        // Create a HAP player instance.
        var player = gameObject.AddComponent<HapPlayer>();

        // Open the specified HAP video file.
        player.Open(_filename);

        // Route playback to the destination render texture.
        player.targetTexture = _destination;

        // Step through playback changes.
        for (var i = 0; i < _iteration; i++)
        {
            // Jump to a random timestamp after each interval.
            await Awaitable.WaitForSecondsAsync(_interval);
            player.time = Random.Range(0, (float)player.streamDuration - _interval);
        }

        // Clean up the player component.
        Destroy(player);
    }
}
