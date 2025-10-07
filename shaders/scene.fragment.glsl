// SPDX-FileCopyrightText: 2025 QuantumHole <QuantumHole@github.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#version 330 core

uniform sampler2D diffuse0;
uniform float color_fade;
uniform bool greyscale;

in vec3 vColor;
in vec2 texCoords;

out vec4 outColor;

void main(void)
{
	vec4 texColor = texture(diffuse0, texCoords);
	vec4 vtxColor = vec4(vColor,1.0);

	if (greyscale)
	{
		float grey = dot(texColor.rgb, vec3(0.299, 0.587, 0.114));
		texColor = vec4(grey, grey, grey, texColor.a);
	}

	vec4 cmix = vec4(0.0, 0.0, 0.0, texColor.a);
	vec4 texFade = mix(texColor, cmix, color_fade);

	// show texture color, if it is different than pure black
	// show vertex color, if texture color is not defined or pure black
	outColor = mix(texFade, vtxColor, step(0.01, vtxColor.r+vtxColor.g+vtxColor.b));
}
