// SPDX-FileCopyrightText: 2025 QuantumHole <QuantumHole@github.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#version 330 core

layout(location = 0) in vec3 pos;
layout(location = 2) in vec3 color;
layout(location = 4) in mat4 inModel;

uniform mat4 projview;

out vec3 vColor;

void main()
{
	vColor = color;
	gl_Position = projview * inModel * vec4(pos,1.0);
}
