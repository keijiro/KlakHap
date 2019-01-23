using UnityEngine;
using UnityEditor;

namespace Klak.Hap
{
    [CanEditMultipleObjects]
    [CustomEditor(typeof(HapPlayer))]
    sealed class HapPlayerEditor : Editor
    {
        SerializedProperty _filePath;
        SerializedProperty _pathMode;

        SerializedProperty _time;
        SerializedProperty _speed;
        SerializedProperty _looping;

        SerializedProperty _targetTexture;
        SerializedProperty _targetRenderer;
        SerializedProperty _targetMaterialProperty;

        string _sourceInfo;

        public string SourceInfo { get {
            if (_sourceInfo != null) return _sourceInfo;

            var player = (HapPlayer)target;
            _sourceInfo = string.Format(
                "{0}\n" +
                "Codec: {1}\n" +
                "Frame dimensions: {2} x {3}\n" +
                "Stream duration: {4:0.00}\n" +
                "Frame rate: {5:0.00}",
                player.resolvedFilePath, player.codecType,
                player.frameWidth, player.frameHeight,
                player.streamDuration,
                player.frameCount / player.streamDuration
            );

            return _sourceInfo;
        } }
        
        static class Labels
        {
            public static readonly GUIContent Property = new GUIContent("Property");
            public static readonly GUIContent Select = new GUIContent("Select");
        }

        void OnEnable()
        {
            _filePath = serializedObject.FindProperty("_filePath");
            _pathMode = serializedObject.FindProperty("_pathMode");

            _time = serializedObject.FindProperty("_time");
            _speed = serializedObject.FindProperty("_speed");
            _looping = serializedObject.FindProperty("_looping");

            _targetTexture = serializedObject.FindProperty("_targetTexture");
            _targetRenderer = serializedObject.FindProperty("_targetRenderer");
            _targetMaterialProperty = serializedObject.FindProperty("_targetMaterialProperty");
        }

        public override void OnInspectorGUI()
        {
            serializedObject.Update();

            if (!Application.isPlaying)
            {
                // Source file
                EditorGUILayout.PropertyField(_filePath);
                EditorGUILayout.PropertyField(_pathMode);
            }
            else if (targets.Length == 1)
            {
                EditorGUILayout.HelpBox(SourceInfo, MessageType.None);
            }

            // Playback control
            EditorGUILayout.PropertyField(_time);
            EditorGUILayout.PropertyField(_speed);
            EditorGUILayout.PropertyField(_looping);

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
