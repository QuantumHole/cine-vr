// SPDX-FileCopyrightText: 2025, 2026 QuantumHole <QuantumHole@github.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef GRAPHICS_SVG_H
#define GRAPHICS_SVG_H

#include <vector>
#include <glm/glm.hpp>
#include "opengl/shape_set.h"

class GraphicsSVG : public ShapeSet
{
	private:

	public:
		explicit GraphicsSVG(const std::string& file_name);
		virtual ~GraphicsSVG(void) override;
};

#endif
