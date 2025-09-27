#include "id.h"

ID* ID::m_instance = nullptr;
std::mutex ID::m_mutex;

ID::ID(void) :
	m_id(0)
{
}

size_t ID::unique_id(void)
{
	const std::lock_guard<std::mutex> lock(m_mutex);
	if (m_instance == nullptr)
	{
		m_instance = new ID();
	}
	m_instance->m_id++;
	return m_instance->m_id;
}
