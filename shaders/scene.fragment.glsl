// SPDX-FileCopyrightText: 2025 QuantumHole <QuantumHole@github.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#version 330 core

in vec3 vColor;

out vec4 outColor;

void main()
{
	outColor = vec4(vColor,1.0);
}
