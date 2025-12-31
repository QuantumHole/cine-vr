// SPDX-FileCopyrightText: 2025 QuantumHole <QuantumHole@github.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "string_tools.h"
#include <algorithm>

// Trim from both ends
std::string trim(const std::string& t)
{
	std::string s(t);

	// Trim from the end (in place)
	s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
		return !std::isspace(ch);
	}).base(), s.end());

	// Trim from the start (in place)
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
		return !std::isspace(ch);
	}));

	return s;
}
