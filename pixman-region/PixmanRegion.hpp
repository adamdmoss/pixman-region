/*
 * PixmanRegion.hpp
 *
 *  Created on: 1 Mar 2015
 *      Author: adam
 */

#ifndef PIXMANREGION_HPP_
#define PIXMANREGION_HPP_

extern "C" {
#include "pixman-region.h"
}

/*class PixmanBox32 : pixman_box32_t
{
	PixmanBox32()
	: x1(0), y1(0), x2(0), y2(0) {}
};*/

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

	bool operator== (const PixmanRegion &other) {
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

	PixmanRegion intersectRegion(PixmanRegion const& other)
	{
		pixman_region32_t result;
		pixman_region32_intersect(&result,
				&m_region,
				const_cast<pixman_region32_t*>(&other.m_region));
		return PixmanRegion(result);
	}

	PixmanRegion unionRegion(PixmanRegion const& other)
	{
		pixman_region32_t result;
		pixman_region32_union(&result,
				&m_region,
				const_cast<pixman_region32_t*>(&other.m_region));
		return PixmanRegion(result);
	}

	PixmanRegion subtractRegion(PixmanRegion const& other)
	{
		pixman_region32_t result;
		pixman_region32_subtract(&result,
				&m_region,
				const_cast<pixman_region32_t*>(&other.m_region));
		return PixmanRegion(result);
	}

	void copyFrom(PixmanRegion const &src)
	{
		pixman_region32_copy(&m_region,
				const_cast<pixman_region32_t*>(&src.m_region));
	}

	void clear()
	{
		pixman_region32_clear(&m_region);
	}

	bool containsPoint(int x, int y) const
	{
		return !!pixman_region32_contains_point(
				const_cast<pixman_region32_t*>(&m_region),
				x, y, nullptr);
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

protected:
	PixmanRegion(pixman_rectangle32_t const &from_rect) {
		pixman_region32_init_rect(&m_region,
				from_rect.x, from_rect.y,
				from_rect.width, from_rect.height);
	}
	PixmanRegion(pixman_region32_t const &from_region32) {
		pixman_region32_init(&m_region);
		copyFrom(from_region32);
	}

	void freeInternal()
	{
		pixman_region32_fini(&m_region);
	}

private:
	pixman_region32_t m_region;
};

#endif /* PIXMANREGION_HPP_ */