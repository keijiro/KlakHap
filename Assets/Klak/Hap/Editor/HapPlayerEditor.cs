using UnityEngine;
using UnityEditor;

namespace Klak.Hap
{
    [CanEditMultipleObjects]
    [CustomEditor(typeof(HapPlayer))]
    sealed class HapPlayerEditor : Editor
    {
        SerializedProperty _fileName;
        SerializedProperty _time;
        SerializedProperty _speed;

        void OnEnable()
        {
            _fileName = serializedObject.FindProperty("_fileName");
            _time = serializedObject.FindProperty("_time");
            _speed = serializedObject.FindProperty("_speed");
        }

        public override void OnInspectorGUI()
        {
            serializedObject.Update();

            EditorGUILayout.PropertyField(_fileName);
            EditorGUILayout.PropertyField(_time);
            EditorGUILayout.PropertyField(_speed);

            serializedObject.ApplyModifiedProperties();
        }
    }
}
