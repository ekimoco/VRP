using System.IO;

using UnityEditor;
using UnityEngine;

namespace ekimoco.WorldProfiler
{
  public class WorldProfilerSetup : EditorWindow
  {
    private string[] languages = new string[] { "日本語", "English", "한국어" };
    private int selectedLanguageIndex = 0;
    public const string VERSION = "0.1.0";

    [MenuItem("EKMIOCO/World Profiler/Setup")]
    static void init()
    {
      WorldProfilerSetup window = (WorldProfilerSetup)EditorWindow.GetWindow(typeof(WorldProfilerSetup));
      window.titleContent = new GUIContent("World Profiler Setup");
      window.Show();
    }
  }
}