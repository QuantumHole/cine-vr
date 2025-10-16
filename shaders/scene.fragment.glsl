// SPDX-FileCopyrightText: 2025 QuantumHole <QuantumHole@github.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#version 330 core

uniform sampler2D diffuse0;
uniform bool background;
uniform bool greyscale;

in vec4 vtxColor;
in vec2 texCoords;

out vec4 outColor;

void main(void)
{
	vec4 texColor = texture(diffuse0, texCoords);

	if (greyscale)
	{
		float grey = dot(texColor.rgb, vec3(0.299, 0.587, 0.114));
		texColor = vec4(grey, grey, grey, texColor.a);
	}

	if (background)
	{
		outColor = mix(vtxColor, texColor, texColor.a);
	}
	else
	{
		// show texture color, if it is different than pure black
		// show vertex color, if texture color is not defined or pure black
		outColor = mix(texColor, vtxColor, step(0.01, vtxColor.r+vtxColor.g+vtxColor.b));
	}
}
