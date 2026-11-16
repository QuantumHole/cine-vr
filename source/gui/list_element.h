// SPDX-FileCopyrightText: 2026 QuantumHole <QuantumHole@github.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef LIST_ELEMENT_H
#define LIST_ELEMENT_H

#include "database/thumbnail_generator.h"
#include <stddef.h>
#include <string>

class ListElement
{
	private:

	public:
		/** constructor. */
		explicit ListElement(void);

		size_t create_thumbnail_label(const std::string& text, const bool align_right, const bool top_half) const;
		size_t create_text_element(const std::string& text, const size_t indent_level) const;
		size_t create_list_element(
			const std::string& text,
			const size_t indent_level = 0,
			const Thumbnail* background = nullptr,
			const std::string& duration = "",
			size_t* preview_texture = nullptr) const;
};

#endif
