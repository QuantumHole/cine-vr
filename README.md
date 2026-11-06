<!--
SPDX-FileCopyrightText: 2025, 2026 QuantumHole <QuantumHole@github.com>

SPDX-License-Identifier: GPL-3.0-or-later
-->

# <img src="images/logo-cinevr.svg" alt="Cine-VR" width="20%" />
Cine-VR displays media in 3D using OpenVR and OpenGL.

It renders a media file into HMD stereoscopically.
It also shows the left eye view in a companion window on the desktop.
It demonstrates handling of a menu structure.
To achieve this, the most basic graphical elements for user interactions had to be implemented from scratch.
These elements contain simple and toggle buttons, slideable gauges as well as scrollable list views.
Users can configure some display options via the OpenVR controller.

# Dependencies

## Runtime Dependencies
* [OpenGL](https://www.opengl.org) rendering of display elements
* [GLFW](https://www.glfw.org) 3.4 or newer, built with Wayland support for native sway sessions
* [OpenVR](https://github.com/ValveSoftware/openvr) VR interface and display
* [libpng](https://www.libpng.org/pub/png/libpng.html) PNG image display
* [libjpeg](https://jpegclub.org/reference/reference-sources) JPEG image display
* [FreeType2](https://freetype.org) text rendering
* [libmpv](https://mpv.io) video playback
* [libavformat libavcodec libavutil libswscale](https://ffmpeg.org) thumbnail generation
* [libsqlite3](https://sqlite.org) thumbnail and configuration storage
* [libcrypto](https://docs.openssl.org/master/man7/ossl-guide-libcrypto-introduction/) calculation of hash values

## Build Dependencies

* [gcc](https://gcc.gnu.org) or [clang](https://clang.llvm.org)
* [GNU make](https://www.gnu.org/software/make)
* [pkg-config](https://www.freedesktop.org/wiki/Software/pkg-config)
* [SDL3](https://wiki.libsdl.org/SDL3/FrontPage)

## Lint Dependencies

* [uncrustify](https://github.com/uncrustify/uncrustify)
* [reuse](https://reuse.software)

# Building

`make all`, or when selecting a compiler explicitly
`make gnu` or `make llvm`

# Runtime environment

In a Wayland session, Cine-VR requests a native Wayland backend by default.
GLFW must be built with Wayland support.
To choose a backend explicitly, set `CINE_VR_PLATFORM` to `wayland`, `x11`, or `auto`:

```sh
CINE_VR_PLATFORM=wayland ./cine-vr
CINE_VR_PLATFORM=x11 ./cine-vr
```

`x11` uses XWayland when launched from a wayland environment.
`auto` lets it select an available platform.
Native window support does not guarantee that the installed SteamVR/OpenVR runtime supports running its compositor
in the same wayland or X11 session; that must be verified separately.

# Contribution

Before committing, please take care of source code format and proper license information.

`make lint`

# Background Information

Some useful resources for learning the topics covered in this project.
* OpenGL Video tutorials by [Victor Gordan](https://www.youtube.com/@VictorGordan/videos)
