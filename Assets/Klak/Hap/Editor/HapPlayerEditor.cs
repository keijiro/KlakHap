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

        SerializedProperty _targetTexture;
        SerializedProperty _targetRenderer;
        SerializedProperty _targetMaterialProperty;

        static class Labels
        {
            public static readonly GUIContent Property = new GUIContent("Property");
            public static readonly GUIContent Select = new GUIContent("Select");
        }

        void OnEnable()
        {
            _fileName = serializedObject.FindProperty("_fileName");

            _time = serializedObject.FindProperty("_time");
            _speed = serializedObject.FindProperty("_speed");

            _targetTexture = serializedObject.FindProperty("_targetTexture");
            _targetRenderer = serializedObject.FindProperty("_targetRenderer");
            _targetMaterialProperty = serializedObject.FindProperty("_targetMaterialProperty");
        }

        public override void OnInspectorGUI()
        {
            serializedObject.Update();

            EditorGUILayout.PropertyField(_fileName);
            EditorGUILayout.PropertyField(_time);
            EditorGUILayout.PropertyField(_speed);

            // Target texture/renderer
            EditorGUILayout.PropertyField(_targetTexture);
            EditorGUILayout.PropertyField(_targetRenderer);

            EditorGUI.indentLevel++;

            if (_targetRenderer.hasMultipleDifferentValues)
            {
                // Multiple renderers selected: Show a simple text field.
                EditorGUILayout.PropertyField(_targetMaterialProperty, Labels.Property);
            }
            else if (_targetRenderer.objectReferenceValue != null)
            {
                // Single renderer: Show the material property selection dropdown.
                MaterialPropertySelector.DropdownList(_targetRenderer, _targetMaterialProperty);
            }

            EditorGUI.indentLevel--;

            serializedObject.ApplyModifiedProperties();
        }
    }
}
