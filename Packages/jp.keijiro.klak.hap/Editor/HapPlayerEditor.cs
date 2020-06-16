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
        SerializedProperty _loop;

        SerializedProperty _targetTexture;
        SerializedProperty _targetRenderer;
        SerializedProperty _targetMaterialProperty;

        static class Labels
        {
            public static readonly GUIContent Property = new GUIContent("Property");
            public static readonly GUIContent Select = new GUIContent("Select");
        }

        string _sourceInfo;

        void ShowSourceInfo(HapPlayer player)
        {
            if (!player.enabled || !player.gameObject.activeInHierarchy) return;

            if (!player.isValid)
            {
                EditorGUILayout.HelpBox(
                    "Failed to open file. " +
                    "Please specify a valid HAP-encoded .mov file.",
                    MessageType.Warning
                );
                return;
            }

            if (_sourceInfo == null)
                _sourceInfo = string.Format(
                    "Codec: {0}\n" +
                    "Frame dimensions: {1} x {2}\n" +
                    "Stream duration: {3:0.00}\n" +
                    "Frame rate: {4:0.00}",
                    player.codecType,
                    player.frameWidth, player.frameHeight,
                    player.streamDuration,
                    player.frameCount / player.streamDuration
                );

            EditorGUILayout.HelpBox(_sourceInfo, MessageType.None);
        }

        void OnEnable()
        {
            _filePath = serializedObject.FindProperty("_filePath");
            _pathMode = serializedObject.FindProperty("_pathMode");

            _time = serializedObject.FindProperty("_time");
            _speed = serializedObject.FindProperty("_speed");
            _loop = serializedObject.FindProperty("_loop");

            _targetTexture = serializedObject.FindProperty("_targetTexture");
            _targetRenderer = serializedObject.FindProperty("_targetRenderer");
            _targetMaterialProperty = serializedObject.FindProperty("_targetMaterialProperty");
        }

        public override void OnInspectorGUI()
        {
            var reload = false;

            serializedObject.Update();

            // Source infomation
            if (!_filePath.hasMultipleDifferentValues &&
                !_pathMode.hasMultipleDifferentValues &&
                !string.IsNullOrEmpty(_filePath.stringValue))
            {
                ShowSourceInfo((HapPlayer)target);
            }

            // Source file
            EditorGUI.BeginChangeCheck();
            EditorGUILayout.DelayedTextField(_filePath);
            EditorGUILayout.PropertyField(_pathMode);
            reload = EditorGUI.EndChangeCheck();

            // Playback control
            EditorGUILayout.PropertyField(_time);
            EditorGUILayout.PropertyField(_speed);
            EditorGUILayout.PropertyField(_loop);

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

            if (reload)
            {
                // This is a little bit scary hack but we can force a HapPlayer
                // to reload a given video file by invoking OnDestroy.
                foreach (HapPlayer hp in targets)
                {
                    hp.SendMessage("OnDestroy");
                    hp.SendMessage("LateUpdate");
                }

                // Also the source information string should be refreshed.
                _sourceInfo = null;
            }
        }
    }
}
