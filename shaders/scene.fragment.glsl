// SPDX-FileCopyrightText: 2025, 2026 QuantumHole <QuantumHole@github.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#version 330 core

uniform sampler2D diffuse0;
uniform sampler3D preview3d;
uniform bool use_preview3d;
uniform float preview_slice;
uniform bool background;
uniform bool greyscale;
uniform float render_opacity;

in vec4 vtxColor;
in vec2 texCoords;

out vec4 outColor;

void main(void)
{
	vec4 texColor = use_preview3d
		? texture(preview3d, vec3(texCoords, preview_slice))
		: texture(diffuse0, texCoords);

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

	if (greyscale)
	{
		float grey = dot(outColor.rgb, vec3(0.299, 0.587, 0.114));
		outColor.rgb = vec3(grey);
	}
	outColor.a *= render_opacity;
}
