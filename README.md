<!--
SPDX-FileCopyrightText: 2025 QuantumHole <QuantumHole@github.com>

SPDX-License-Identifier: GPL-3.0-or-later
-->

# Cine-VR
This is a small test project to check out some mechanisms in OpenGL and OpenVR.

It renders a scene into HMD (left/right). It also blits left eye to GLFW window for debugging.
It shows some floating rectangles, controller rays, computes ray-rectangle intersection,
and prints clicked coordinate on trigger button activation.

# Dependencies

* OpenGL
* GLFW
* GLEW
* OpenVR
* libpng
* libjpeg

# Building

`make run`

# Contribution

Before committing, please take care of source code format and proper license information.

`make lint`
