/*
 * PixmanRegion.hpp
 *
 *  Created on: 1 Mar 2015
 *      Author: adam
 */

#ifndef PIXMANREGION_HPP_
#define PIXMANREGION_HPP_

#include <cassert>
#include <iterator>

extern "C" {
#include "pixman-region.h"
}


class PixmanBox32 : pixman_box32_t
{
	PixmanBox32() { x1=y1=x2=y2=0; }
};


// Wraps a raw, not-to-be-freed C array with bounds checking and iterator
template<typename T>
class StaticCArray
{
public:
	StaticCArray(T* ptr, size_t n) { m_ptr = ptr; m_size = n; }
	virtual ~StaticCArray() {};

#if 1
	T& operator[](size_t idx)
	{
		assert(idx < m_size);
		assert(idx >= 0);
		return m_ptr[idx];
	}

	const T& operator[](size_t idx) const
	{
		assert(idx < m_size);
		assert(idx >= 0);
		return m_ptr[idx];
	}
#endif

	size_t size(void) const
	{
		return m_size;
	}

protected:
	T* m_ptr;
	size_t m_size;
};


class PixmanRegion {
public:
	PixmanRegion() {
		pixman_region32_init(&m_region);
		clear();
	}
	PixmanRegion(PixmanRegion const &from_rect) {
		pixman_region32_init(&m_region);
		pixman_region32_copy(&m_region,
				const_cast<pixman_region32_t*>(&from_rect.m_region));
	}
	PixmanRegion(int x, int y, unsigned int width, unsigned int height) {
		pixman_region32_init_rect(&m_region, x, y, width, height);
	}

	virtual ~PixmanRegion() {
		this->freeInternal();
	}

	/**********************/

	bool operator== (const PixmanRegion &other) const {
		return this->isEqual(other);
	}

	PixmanRegion& operator=(const PixmanRegion& other) {
		if (this != &other)
		{
			copyFrom(other);
		}
		return *this;
	}

	/**********************/

	void copyFrom(PixmanRegion const &src)
	{
		pixman_region32_copy(&m_region,
				const_cast<pixman_region32_t*>(&src.m_region));
	}

	void clear()
	{
		pixman_region32_clear(&m_region);
	}

	void translate(int xoffset, int yoffset)
	{
		pixman_region32_translate(&m_region, xoffset, yoffset);
	}

	PixmanRegion intersectRegion(PixmanRegion const& other) const
	{
		pixman_region32_t result;
		pixman_region32_init(&result);
		pixman_region32_intersect(&result,
				const_cast<pixman_region32_t*>(&m_region),
				const_cast<pixman_region32_t*>(&other.m_region));
		return PixmanRegion(result);
	}

	PixmanRegion unionRegion(PixmanRegion const& other) const
	{
		pixman_region32_t result;
		pixman_region32_init(&result);
		pixman_region32_union(&result,
				const_cast<pixman_region32_t*>(&m_region),
				const_cast<pixman_region32_t*>(&other.m_region));
		return PixmanRegion(result);
	}

	PixmanRegion subtractRegion(PixmanRegion const& other) const
	{
		pixman_region32_t result;
		pixman_region32_init(&result);
		pixman_region32_subtract(&result,
				const_cast<pixman_region32_t*>(&m_region),
				const_cast<pixman_region32_t*>(&other.m_region));
		return PixmanRegion(result);
	}

	bool containsPoint(int x, int y) const
	{
		return !!pixman_region32_contains_point(
				const_cast<pixman_region32_t*>(&m_region),
				x, y, nullptr);
	}

	bool intersects(PixmanRegion const &other) const
	{
		PixmanRegion const &intersection =
				this->intersectRegion(other);
		return !intersection.isEmpty();
	}

	bool containsEntirely(PixmanRegion const &other) const
	{
		PixmanRegion const &subbed =
				other.subtractRegion(*this);
		return subbed.isEmpty();
	}

	bool isEmpty() const
	{
		return !pixman_region32_not_empty(
				const_cast<pixman_region32_t*>(&m_region)
				);
	}

	bool isEqual(PixmanRegion const& other) const
	{
		return !!pixman_region32_equal(
				const_cast<pixman_region32_t*>(&m_region),
				const_cast<pixman_region32_t*>(&other.m_region));
	}

	StaticCArray<pixman_box32_t> getBoxes() const
	{
		pixman_box32_t* boxes_ptr;
		int num_boxes;
		boxes_ptr = pixman_region32_rectangles(
				const_cast<pixman_region32_t*>(&m_region),
				&num_boxes);
		return StaticCArray<pixman_box32_t>(boxes_ptr,num_boxes);
	}

protected:
	PixmanRegion(pixman_rectangle32_t const &from_rect) {
		pixman_region32_init_rect(&m_region,
				from_rect.x, from_rect.y,
				from_rect.width, from_rect.height);
	}
	PixmanRegion(pixman_region32_t const &from_region32) {
		pixman_region32_init(&m_region);
		pixman_region32_copy(&m_region,
				const_cast<pixman_region32_t*>(&from_region32));
	}

	void freeInternal()
	{
		pixman_region32_fini(&m_region);
	}

private:
	pixman_region32_t m_region;
};




#endif /* PIXMANREGION_HPP_ */
