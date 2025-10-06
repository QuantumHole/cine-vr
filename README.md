<!--
SPDX-FileCopyrightText: 2025 QuantumHole <QuantumHole@github.com>

SPDX-License-Identifier: GPL-3.0-or-later
-->

# Cine-VR
This is a small test project to check out some mechanisms in OpenGL and OpenVR.

It renders a scene into HMD (left/right). It also blits left eye to GLFW window for debugging.
It demonstrates handling of a menu structure. Users can configure some display options via the
OpenVR controller. Some debug information is printed to the console.

# Dependencies

## Runtime Dependencies
* OpenGL
* GLFW
* GLEW
* OpenVR
* libpng
* libjpeg

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
