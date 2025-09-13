<!--
SPDX-FileCopyrightText: 2025 QuantumHole <QuantumHole@github.com>

SPDX-License-Identifier: GPL-3.0-or-later
-->

# Cine-VR
This is a small test project to check out some mechanisms in OpenGL and OpenVR.

It renders a scene into HMD (left/right). It also blits left eye to GLFW window for debugging.
It shows a floating rectangle, controller rays, computes ray-rectangle intersection,
and prints clicked coordinate on trigger rising edge.

# Dependencies

* OpenGL
* GLFW
* GLEW
* OpenVR

# Building

`make clean && make -j all && ./cine-vr`

# Contribution

Before committing, please take care of source code format and proper license information.

`make format && make license-annotate && make license-lint`
