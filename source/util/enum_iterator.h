// SPDX-FileCopyrightText: 2025 QuantumHole <QuantumHole@github.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef ENUM_ITERATOR_H
#define ENUM_ITERATOR_H

#include <type_traits>
#include <stddef.h>

template <typename C, C beginVal, C endVal>
class EnumIterator
{
	private:
		typedef typename std::underlying_type<C>::type val_t;
		int val;

	public:
		static size_t size(void);
		EnumIterator(const C& f);
		EnumIterator(void);
		EnumIterator& operator++(void);
		C operator*(void);
		EnumIterator begin();
		EnumIterator end();
		bool operator!=(const EnumIterator& i);
};

#endif
