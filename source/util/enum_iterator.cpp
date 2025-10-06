// SPDX-FileCopyrightText: 2025 QuantumHole <QuantumHole@github.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "enum_iterator.h"
#include <openvr.h>

template <typename C, C beginVal, C endVal>
EnumIterator<C, beginVal, endVal>::EnumIterator(const C& f) :
	val(static_cast<int>(f))
{
}

template <typename C, C beginVal, C endVal>
EnumIterator<C, beginVal, endVal>::EnumIterator() :
	val(static_cast<int>(beginVal))
{
}

template <typename C, C beginVal, C endVal>
EnumIterator<C, beginVal, endVal>& EnumIterator<C, beginVal, endVal>::operator++()
{
	++val;
	return *this;
}

template <typename C, C beginVal, C endVal>
C EnumIterator<C, beginVal, endVal>::operator*()
{
	return static_cast<C>(val);
}

template <typename C, C beginVal, C endVal>
EnumIterator<C, beginVal, endVal> EnumIterator<C, beginVal, endVal>::begin()
{
	return *this; // default ctor is good
}

template <typename C, C beginVal, C endVal>
EnumIterator<C, beginVal, endVal> EnumIterator<C, beginVal, endVal>::end()
{
	static const EnumIterator endIter = ++EnumIterator(endVal); // cache it

	return endIter;
}

template <typename C, C beginVal, C endVal>
bool EnumIterator<C, beginVal, endVal>::operator!=(const EnumIterator& i)
{
	return val != i.val;
}

template <typename C, C beginVal, C endVal>
size_t EnumIterator<C, beginVal, endVal>::size(void)
{
	return endVal - beginVal + 1;
}

// Explicit template instantiation
template class EnumIterator<vr::Hmd_Eye, vr::Eye_Left, vr::Eye_Right>;
