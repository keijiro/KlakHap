using UnityEngine;
using Klak.Hap;
using System.Collections;

class ScriptingTest : MonoBehaviour
{
    [SerializeField] string _fileName = "Test.mov";

    IEnumerator Start()
    {
        var player = gameObject.AddComponent<HapPlayer>();
        player.targetRenderer = GetComponent<Renderer>();
        player.Open(_fileName);

        for (var i = 0; i < 20; i++)
        {
            yield return new WaitForSeconds(0.5f);
            player.time = Random.Range(0, (float)player.streamDuration - 0.5f);
        }

        Destroy(player);
        GetComponent<Renderer>().enabled = false;
    }
}
