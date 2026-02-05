# KlakHap

![GIF](https://i.imgur.com/exuJAIA.gif)

**KlakHap** is a Unity plugin for playing back video streams encoded with the
[HAP video codecs].

HAP is a fast and high-quality video codec often used in real-time interactive
applications. From the HAP Codecs website:

> The HAP codecs are designed to fit the needs of a variety of real-time video
> workflows where ultra high resolution video is needed such as live event
> production, set design, 360 video for gaming, projection mapping and creative
> coding.

KlakHap provides decoded frames as textures that you can use anywhere in
Unity's rendering pipeline: apply them to a material, present full-screen
video, animate a UI element, and more. Thanks to the efficient design and
implementation of the HAP codecs, it can adjust playback time and speed
dynamically without hiccups.

[HAP video codecs]: https://hap.video/

# System requirements

- Unity 2022.3 or later

Currently, KlakHap supports only 64-bit desktop platforms (Windows, macOS, and
Linux).

# Supported formats

KlakHap supports **HAP**, **HAP Alpha**, and **HAP Q**. **HAP Q Alpha** is not
supported.

KlakHap only supports the QuickTime File Format as a container, i.e., `.mov`
files.

# How to install

Install the KlakHap package (`jp.keijiro.klak.hap`) from the "Keijiro" scoped
registry in Package Manager. Follow [these instructions] to add the registry to
your project.

[these instructions]:
  https://gist.github.com/keijiro/f8c7e8ff29bfe63d86b888901b82644c

# How to specify a video file

There are two ways to specify a video file in the plugin:

- **Streaming Assets Mode**: Put a video file in the [Streaming Assets]
  directory and specify its file name.
- **Local File System Mode**: Put a video file somewhere in local drive and
  specify its full path.

The former method is recommended when the video file ships with the
application. The latter method is useful when you need to play external
content.

[Streaming Assets]: https://docs.unity3d.com/Manual/StreamingAssets.html

# Hap Player component

![Inspector](https://i.imgur.com/pIACL4W.png)

**File Path** and **Path Mode** specify the source video file. See the previous
section for details.

**Time**, **Speed** and **Loop** are used to set the initial playback state.
You can also change these values during playback.

**Target Texture** stores decoded frames in a render texture. Note that this
allocates a small amount of GPU time for data transfer.

**Target Renderer** applies the decoded texture to a specific material
property. Although this is the most performant way to render video frames, it
requires a few extra steps to render correctly. Keep the following points in
mind:

- UV coordinate incompatibility: Decoded textures are upside down due to
  differences in UV coordinate conventions between Unity and HAP. You can fix
  this using a vertically inverted texture scale/offset. You can also use the
  `Klak/Hap` shader for this purpose.
- Color space conversion for HAP Q: [YCoCg conversion] must be added to a
  shader when using HAP Q. You can also use the `Klak/HAP Q` shader for this
  purpose.

[YCoCg conversion]:
  https://gist.github.com/dlublin/90f879cfe027ebf5792bdadf2c911bb5

# How to control playback

`HapPlayer` provides only a few properties and methods for controlling
playback. This is an intentional design choice; I avoid ambiguous methods like
`Play`, `Stop`, and `Pause`. Use the basic properties and methods instead.

- To jump to a specific point: Assign a time in seconds to `time`.
- To jump to a specific frame: Calculate the time in seconds using `frameCount`
  and `streamDuration`, then assign it to `time`.
- To reverse the playback direction: Assign a negative value to `speed`.
- To pause: Assign `0` to `speed`.
- To resume: Assign `1` to `speed`.
- To stop: Assign `false` to `enabled`.
- To close the video file: Destroy the `HapPlayer` component.
- To open another video file: Call `AddComponent<HapPlayer>`, then call `Open`.

# Timeline support

![GIF](https://i.imgur.com/efrvvye.gif)

The HAP Player component implements the [ITimeControl] interface, which allows
it to control playback time from a Control Track in a [Timeline]. You can
create a control track by dragging and dropping a HAP Player game object into
the Timeline Editor, or manually create a Control Track/Clip and set the
source game object.

[ITimeControl]: https://docs.unity3d.com/ScriptReference/Timeline.ITimeControl.html
[Timeline]: https://docs.unity3d.com/Manual/TimelineSection.html

# Platform differences (internal latency)

On Windows, KlakHap uses the [Custom Texture Update] feature to hide the
synchronization point in the background thread. It guarantees exact-frame
playback with minimal load on the main thread.

On macOS and Linux, the Custom Texture Update feature is unavailable for this
purpose[^1]. Instead, KlakHap delays synchronization to the next frame to avoid
main thread stalls. In other words, it guarantees exact-frame playback but adds
a single-frame latency.

You can turn off this behavior by adding `HAP_NO_DELAY` to the [Scripting
Define Symbols] in the project settings. This stalls the main thread for every
frame decoding. It significantly slows down the application but is useful when
exact frame matching is essential (e.g., [volumetric video playback] with
Alembic animation).

[Custom Texture Update]:
  https://github.com/keijiro/TextureUpdateExample

[Scripting Define Symbols]:
  https://docs.unity3d.com/Manual/CustomScriptingSymbols.html

[volumetric video playback]:
  https://github.com/keijiro/Abcvfx

[^1]: The Custom Texture Update feature is available even on macOS/Linux but
      doesn't support compressed texture formats, which are essential for HAP
      decoding.
