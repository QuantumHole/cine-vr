// SPDX-FileCopyrightText: 2025 QuantumHole <QuantumHole@github.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef ID_H
#define ID_H

#include <mutex>

class ID
{
	private:
		static ID* m_instance;
		static std::mutex m_mutex;
		size_t m_id;

		explicit ID(void);

	public:
		static size_t unique_id(void);
};

#endif
