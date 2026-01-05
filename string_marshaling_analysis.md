# KlakHap File Path Handling Analysis

## Summary of File Path Flow

1. **C# Side (HapPlayer.cs)**:
   - File paths are stored as C# strings (UTF-16 internally)
   - `resolvedFilePath` property combines `Application.streamingAssetsPath` with `_filePath`
   - Path is passed to `Demuxer` constructor

2. **C# to Native Boundary (Demuxer.cs)**:
   - `KlakHap_OpenDemuxer(string filepath)` is declared with `[DllImport("KlakHap")]`
   - No explicit `CharSet` is specified in the DllImport attribute
   - Unity's default marshaling for strings without CharSet specification is ANSI on Windows

3. **Native Side (KlakHap.cpp & Demuxer.h)**:
   - `KlakHap_OpenDemuxer(const char* filepath)` receives the path as `const char*`
   - The path is passed directly to `Demuxer` constructor
   - In Demuxer.h, the path is used with:
     - Windows: `fopen_s(&file_, path, "rb")`
     - Other platforms: `fopen(path, "rb")`

## Identified Issues

### 1. **String Marshaling Issue on Windows**
- Without explicit `CharSet` specification, Unity marshals strings as ANSI on Windows
- This causes issues with multibyte characters (Japanese, Chinese, etc.)
- The ANSI code page may not support all Unicode characters

### 2. **No Wide Character API Usage**
- On Windows, the code uses `fopen_s` instead of `_wfopen` 
- `fopen_s` expects ANSI/UTF-8 paths, not Unicode
- This limits support for paths with characters outside the current code page

### 3. **Project Configuration**
- The Windows project file (KlakHap.vcxproj) has `CharacterSet=Unicode`
- However, this doesn't affect the file opening functions used

## Potential Solutions

### Solution 1: Fix P/Invoke Marshaling (Minimal Change)
```csharp
[DllImport("KlakHap", CharSet = CharSet.Ansi)]
internal static extern IntPtr KlakHap_OpenDemuxer([MarshalAs(UnmanagedType.LPUTF8Str)] string filepath);
```
Note: `UnmanagedType.LPUTF8Str` requires .NET Framework 4.7+ or .NET Core

### Solution 2: Use Wide Character APIs on Windows
```cpp
#ifdef _WIN32
    FILE* file = nullptr;
    // Convert UTF-8 to wide char
    int wlen = MultiByteToWideChar(CP_UTF8, 0, path, -1, nullptr, 0);
    if (wlen > 0) {
        wchar_t* wpath = new wchar_t[wlen];
        MultiByteToWideChar(CP_UTF8, 0, path, -1, wpath, wlen);
        _wfopen_s(&file, wpath, L"rb");
        delete[] wpath;
    }
#else
    file_ = fopen(path, "rb");
#endif
```

### Solution 3: Explicit UTF-8 Marshaling
```csharp
// C# side - manually convert to UTF-8
byte[] utf8Bytes = System.Text.Encoding.UTF8.GetBytes(filePath + "\0");
IntPtr utf8Ptr = Marshal.AllocHGlobal(utf8Bytes.Length);
Marshal.Copy(utf8Bytes, 0, utf8Ptr, utf8Bytes.Length);
_plugin = KlakHap_OpenDemuxer(utf8Ptr);
Marshal.FreeHGlobal(utf8Ptr);
```

## Recommendation

The most robust solution would be to:
1. Update the P/Invoke declaration to explicitly specify UTF-8 marshaling
2. On Windows, convert UTF-8 to wide characters and use `_wfopen` instead of `fopen_s`

This ensures proper handling of all Unicode characters in file paths across all platforms.