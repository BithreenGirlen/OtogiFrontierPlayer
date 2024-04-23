# OtogiFrontierPlayer
某ゲーの某場面再生用。

## Runtime requirement
- Windows OS later than Windows 10
- MSVC 2015-2022 (x64)

## How to play
Select a folder containing spine resources such as the following.
<pre>
  221491
  ├ odin_S1.atlas.txt
  ├ odin_S1.png
  ├ odin_S1.txt
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
  │       ├ 221491
  │       │   ├ odin_S1.atlas.txt
  │       │   └ ...
  │       └ ...
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
| Mouse wheel | Scale up/down the window |
| Left button + mouse wheel | Speed up/down the animation. |
| Left button click | Switch to the next animation. |
| Left button drag | Move View point |
| Middle button | Reset scaling, animation speed, and view point. |
| Right button + mouse wheel | Play the next/previous audio file. |
| Right button + left button | Move Window |

## Key functions
| Input  | Action  |
| --- | --- |
| Esc | Close the application. |
| Up | Move on to the next folder. |
| Down | Move on to the previous folder. |
| PageUp | Speed up the audio playback rate. |
| PageDown | Speed down the audio playback rate. |
| Home | Reset the audio playback rate.|  
| A | Enable/disable premultiplied alpha to the sensitive slots.|  

## Libraries
- [SFML-2.6.1](https://www.sfml-dev.org/download/sfml/2.6.1/)
- [spine-c-3.6](https://github.com/EsotericSoftware/spine-runtimes/tree/3.6)

The visual studio project assumes the following paths in the project folder.
<pre>
  deps
  ├ SFML-2.6.1
  │   ├ include
  │   │   └ SFML
  │   │       └ ...
  │   └ lib
  │       └ ...
  └ spine-c-3.6
      ├ include
      │   └ spine
      │       └ ...
      └ src
          └ spine
              └ ...
</pre>

## Feature limitation
- The spine outline shader, an extension by Unity, cannot be brought forth by SFML.
- Scaling skeleton had yet to be implemented in spine-c 3.6; it was added in 3.7-c.

  I know that in older scenes the initial view point is set away from the center.  
  But their generally grotesque physique are so unattractive that I am not inclined to adapt to the situation.
