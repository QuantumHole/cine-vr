// SPDX-FileCopyrightText: 2025, 2026 QuantumHole <QuantumHole@github.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef RESOURCES_H
#define RESOURCES_H

#include <cstddef>
#include <map>
#include <string>
#include <vector>
#include <stdint.h>

class ResourceManager
{
	private:
		static ResourceManager* m_instance;
		std::map<std::string, std::vector<uint8_t> > m_resources;

		ResourceManager(void);
		void add(const std::string& name, const char* start, const char* end);

	public:
		ResourceManager(const ResourceManager&) = delete;
		ResourceManager& operator=(const ResourceManager&) = delete;
		~ResourceManager(void);

		static const ResourceManager& instance(void);
		const std::vector<uint8_t>& get(const std::string& name) const;
};

#endif
