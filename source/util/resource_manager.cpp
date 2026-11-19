// SPDX-FileCopyrightText: 2025, 2026 QuantumHole <QuantumHole@github.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "resource_manager.h"

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wreserved-identifier"
#endif
#include "build/resources.inc"
#ifdef __clang__
#pragma clang diagnostic pop
#endif

#include <stdexcept>

ResourceManager* ResourceManager::m_instance = nullptr;

ResourceManager::ResourceManager(void) :
	m_resources()
{
#include "build/resources_map.inc"
}

ResourceManager::~ResourceManager(void)
{
}

const ResourceManager& ResourceManager::instance(void)
{
	if (m_instance == nullptr)
	{
		m_instance = new ResourceManager();
	}
	return *m_instance;
}

void ResourceManager::add(const std::string& name, const char* start, const char* end)
{
	const std::vector<uint8_t> data(start, end);

	m_resources.insert(std::pair<std::string, std::vector<uint8_t> >{name, data});
}

const std::vector<uint8_t>& ResourceManager::get(const std::string& name) const
{
	const std::map<std::string, std::vector<uint8_t> >::const_iterator iter = m_resources.find(name);

	if (iter == m_resources.end())
	{
		throw std::invalid_argument("Unknown resource: " + name);
	}
	return iter->second;
}
