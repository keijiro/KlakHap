using UnityEngine;
using UnityEngine.UI;
using Klak.Hap;

class Seeker : MonoBehaviour
{
    [SerializeField] HapPlayer _player = null;

    bool _isUpdating;

    public float time {
        set { if (!_isUpdating) _player.time = value; }
    }

    public void Update()
    {
        _isUpdating = true;
        GetComponent<Slider>().value = _player.time;
        _isUpdating = false;
    }
}
