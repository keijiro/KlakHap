using Unity.Properties;
using UnityEngine;
using UnityEngine.UIElements;
using Klak.Hap;

public sealed class SeekBarBind : MonoBehaviour
{
    [SerializeField] UIDocument _ui = null;

    HapPlayer _player;

    [CreateProperty]
    public float SeekPoint
      { get => _player.time;
        set => _player.time = value; }

    void Start()
    {
        _player = GetComponent<HapPlayer>();

        var slider = _ui.rootVisualElement.Q<Slider>("seek-bar");
        slider.dataSource = this;
        slider.highValue = (float)_player.streamDuration;
    }
}
