// SPDX-FileCopyrightText: 2025, 2026 QuantumHole <QuantumHole@github.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "graphics_svg.h"
#include "util/file_system.h"
#include "util/node_xml.h"

GraphicsSVG::GraphicsSVG(const std::string& file_name) :
	ShapeSet()
{
	// fix locale settings for numerical parsing
	setlocale(LC_ALL | ~LC_NUMERIC, "");

	// read file content
	FileSystem fs;
	const std::string content = fs.read_file(file_name);

	NodeXML xml;
	xml.parse(content);
}

GraphicsSVG::~GraphicsSVG(void)
{
}
