<!--
SPDX-FileCopyrightText: 2025 QuantumHole <QuantumHole@github.com>

SPDX-License-Identifier: GPL-3.0-or-later
-->

# Cine-VR
This is a small test project to check out some mechanisms in OpenGL and OpenVR.

It renders a single image into HMD stereoscopically.
It also shows the left eye view in a companion window on the desktop.
It demonstrates handling of a menu structure.
Users can configure some display options via the OpenVR controller.
Some debug information is printed to the console.

# Dependencies

## Runtime Dependencies
* [OpenGL](https://www.opengl.org)
* [GLFW](https://www.glfw.org/)
* [GLEW](https://www.opengl.org/sdk/libs/GLEW/)
* [OpenVR](https://github.com/ValveSoftware/openvr)
* [libpng](https://www.libpng.org/pub/png/libpng.html)
* [libjpeg](https://jpegclub.org/reference/reference-sources/)
* [FreeType2](https://freetype.org/index.html)

## Build Dependencies

* gcc or clang
* make
* image magick
* pkg-config
* bc

## Lint Dependencies

* uncrustify
* reuse

# Building

`make run`

# Contribution

Before committing, please take care of source code format and proper license information.

`make lint`

# Background Information

Some useful resources for learning the topics covered in this project.
* OpenGL Video tutorials by [Victor Gordan](https://www.youtube.com/@VictorGordan/videos)
