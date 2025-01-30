# TerminalVideoPlayer

A video player that runs completely in your terminal!

# Demo

Click on the image to see the video

[![Demo of the terminal video player](https://img.youtube.com/vi/XUbJASvvWxE/0.jpg)](https://www.youtube.com/watch?v=XUbJASvvWxE)

# How does it work?

You see this unicode character `▄`? It's a half block character, and it's the building block of this program.

If you just printed a bunch of them to the terminal, you would get a bunch of white and black rows, which is not very useful.

```
▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄
▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄
▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄
▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄
▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄
```

However, if you strategically change the background and foreground color of each half-block,
you can create an image where each half-block is 2 pixels!

How do you change the foreground and background color? You can use ANSI escape codes!
```
\033[38;2;255;255;255m  # Set the foreground color to white
\033[48;2;0;0;0m        # Set the background color to black
```

It uses ffmpeg to decode raw frames and resize them to the terminal's size,
and then it uses the above technique to draw the frames to the terminal.

It also uses a technique called "frame differencing" to only update the pixels that have changed between frames,
this way it can update the terminal screen faster.

It also uses ffmpeg in the commandline to extract audio from the video
before using [miniaudio](https://miniaud.io/index.html) to play the audio.

# Dependencies

- [ffmpeg](https://ffmpeg.org/) - for video decoding, resizing, and extracting audio
- [miniaudio](https://miniaud.io/index.html) - for audio playback
- [fmt](https://fmt.dev/latest/index.html) - to display unicode characters (such as the half block earlier) properly on Windows

You also need to have ffmpeg installed on your system as a program that is on your PATH.
To download ffmpeg, you can use the [ffmpeg download page](https://ffmpeg.org/download.html)

# Usage

Taken from help command:
```
Usage: TerminalVideoPlayer.exe [options] <video_file>
Options:
  -h, --help                    Show this help message and exit
  -o, --optimization-level      Optimization level
                                (higher values lead to more artifacts but better performance
                                can be a decimal value, default is 25
  -r, --redraw                  Force redraw of the entire frame instead of optimizing and only updating pixels that need updating
                                Default is false, should only be used if you hate artifacts, love slow powerpoint presentations,
                                or if your terminal font size is somewhat big

Video Controls:
  q                     Quit
  r                     Redraw the entire frame, use if you want to get rid of artifacts
  space or k            Pause/Play the video
  j                     Skip back 5 seconds
  k                     Skip forward 5 seconds
```

# Building and contributing

On Windows, you can just open the `TerminalVideoPlayer.sln` file in Visual Studio 2022 and it should just work. I beleive vcpkg is installed with Visual Studio so you don't have to worry about that.

On other platforms, you will need to get a C\+\+17 compiler and the ffmpeg and {fmt} C\+\+ libraries, which are both cross platform.
Miniaudio is a single header dependency and is included in the repository, so you don't have to worry about it.
I have not tested this on other platforms, but the code should hopefully work without too many changes.

# Summary

This was a fun project to work on, and I learned a lot about how video players work under the hood
along with how to optimize code and profile it. I hope you enjoy this program as much as I enjoyed making it!
