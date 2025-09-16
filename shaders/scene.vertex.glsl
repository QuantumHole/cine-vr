// SPDX-FileCopyrightText: 2025 QuantumHole <QuantumHole@github.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#version 330 core

layout(location=0) in vec3 pos;
layout(location=2) in vec3 color;

uniform mat4 u_mvp;

out vec3 vColor;

void main()
{
	vColor = color;
	gl_Position = u_mvp * vec4(pos,1.0);
}
