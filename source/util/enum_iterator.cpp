// SPDX-FileCopyrightText: 2025, 2026 QuantumHole <QuantumHole@github.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "enum_iterator.h"
#include <openvr/openvr.h>

/** constructor for arguments that can be cast to int.
 * @param f object that can be cast to int.
 */
template <typename C, C beginVal, C endVal>
EnumIterator<C, beginVal, endVal>::EnumIterator(const C& f) :
	val(static_cast<int>(f))
{
}

/** default constructor.
 * Iteration begins at beginVal.
 */
template <typename C, C beginVal, C endVal>
EnumIterator<C, beginVal, endVal>::EnumIterator() :
	val(static_cast<int>(beginVal))
{
}

/** incremental operator. */
template <typename C, C beginVal, C endVal>
EnumIterator<C, beginVal, endVal>& EnumIterator<C, beginVal, endVal>::operator++()
{
	++val;
	return *this;
}

/** dereference operator. */
template <typename C, C beginVal, C endVal>
C EnumIterator<C, beginVal, endVal>::operator*()
{
	return static_cast<C>(val);
}

/** iteration start element. */
template <typename C, C beginVal, C endVal>
EnumIterator<C, beginVal, endVal> EnumIterator<C, beginVal, endVal>::begin()
{
	return *this; // default ctor is good
}

/** iteration end element. */
template <typename C, C beginVal, C endVal>
EnumIterator<C, beginVal, endVal> EnumIterator<C, beginVal, endVal>::end()
{
	static const EnumIterator endIter = ++EnumIterator(endVal); // cache it

	return endIter;
}

/** not-equal comparison operator.
 * @param i enum iterator for comparison.
 * @return comparison result.
 */
template <typename C, C beginVal, C endVal>
bool EnumIterator<C, beginVal, endVal>::operator!=(const EnumIterator& i)
{
	return val != i.val;
}

/** size of the enum.
 * @return number of elements in enum.
 */
template <typename C, C beginVal, C endVal>
size_t EnumIterator<C, beginVal, endVal>::size(void)
{
	return endVal - beginVal + 1;
}

/** explicit template instantiation for enum vr::Hmd_Eye. */
template class EnumIterator<vr::Hmd_Eye, vr::Eye_Left, vr::Eye_Right>;
