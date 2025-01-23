# OtogiFrontierPlayer
某ゲーの某場面再生用。

## Runtime requirement
- Windows OS later than Windows 10
- MSVC 2015-2022 (x64)

## How to play
Select a folder containing spine resources such as the following.
<pre>
221491
├ odin_S1.atlas.txt // Atlas file
├ odin_S1.png
├ odin_S1.txt // Skeleton file (in JSON format)
├ odin_S2.atlas.txt
├ odin_S2.png
├ odin_S2.txt
└ ...
</pre>

After selecting spine resources folder, the audio files will be searched assuming the following relative path, and loaded if found.
<pre>
assetbundleresources
├ chara
│   └ still
│     ├ 221491
│     │   ├ odin_S1.atlas.txt
│     │   └ ...
│     └ ...
└ sound
   └ voice
     └ still
       ├ voice_still_221491
       │   ├ c_2149_1_020_2149.m4a
       │   └ ...
       └ ...
</pre>

## Mouse functions
| Input  | Action  |
| --- | --- |
| Mouse wheel | Scale up/down the window. |
| Left button + mouse wheel | Speed up/down the animation. |
| Left button click | Switch to next animation. |
| Left button drag | Move view-point. |
| Middle button | Reset scaling, animation speed, and view point. |
| Right button + mouse wheel | Play next/previous audio file. |
| Right button + left button | Move window. |

## Keyboard functions
| Input  | Action  |
| --- | --- |
| Esc | Close the application. |
| Up | Move on to the next folder. |
| Down | Move on to the previous folder. |
| PageUp | Speed up the audio playback rate. |
| PageDown | Speed down the audio playback rate. |
| Home | Reset the audio playback rate.|
| D | Enable/disable initial offset.|
| S | Enable/disable premultiplied alpha to sensitive slots.|
| T | Hide/show audio track number.|

- It is better to disable the initial offset for newer scenes, and to enable for older scenes.

## External Libraries
- [SFML-2.6.2](https://www.sfml-dev.org/download/sfml/2.6.2/)
- [spine-c-3.6](https://github.com/EsotericSoftware/spine-runtimes/tree/3.6)

## Build

1. Run `OtogiFrontierPlayer/deps/CMakeLists.txt` to obtain external libraries.
2. Open `OtogiFrontierPlayer.sln` with Visual Studio.
3. Select `Build Solution` on menu item.

`OtogiFrontierPlayer/deps` folder will be as follows:
<pre>
deps
├ SFML-2.6.2 //  SFML for VC17 x64
│   ├ include
│   │   └ SFML
│   │       └ ...
│   └ lib
│       └ ...
└ spine-c-3.6 // Spine generic runtime for version 3.6.xx
    ├ include
    │   └ spine
    │       └ ...
    └ src
        └ spine
            └ ...
</pre>

## Feature limitation
- Spine outline shader, an extension by Unity, cannot be brought forth by SFML.
