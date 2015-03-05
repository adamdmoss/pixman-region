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


class PixmanRegion {
public:
	/** CTORS/DTORS *******************/

	PixmanRegion() {
		pixman_region32_init(&m_region);
		clear();
	}
	PixmanRegion(PixmanRegion const &from_region) {
		pixman_region32_init(&m_region);
		pixman_region32_copy(&m_region,
				const_cast<pixman_region32_t*>(&from_region.m_region));
	}
	PixmanRegion(int x, int y,
			unsigned int width, unsigned int height) {
		pixman_region32_init_rect(&m_region,
				x, y, width, height);
	}
	PixmanRegion(pixman_region32_t const &from_pixman_region32) {
		pixman_region32_init(&m_region);
		pixman_region32_copy(&m_region,
				const_cast<pixman_region32_t*>(&from_pixman_region32));
	}

	virtual ~PixmanRegion() {
		this->freeInternal();
	}

	/** OPERATORS ********************/

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

	/** METHODS ********************/

	// make this region a copy of another
	void copyFrom(PixmanRegion const &src)
	{
		pixman_region32_copy(&m_region,
				const_cast<pixman_region32_t*>(&src.m_region));
	}

	// empty a region
	void clear()
	{
		pixman_region32_clear(&m_region);
	}

	// translate a region by specified offset, in-place
	void translate(int xoffset, int yoffset)
	{
		pixman_region32_translate(&m_region, xoffset, yoffset);
	}

	// return region which is intersection of this region with other
	PixmanRegion intersectRegion(PixmanRegion const& other) const
	{
		pixman_region32_t result;
		pixman_region32_init(&result);
		pixman_region32_intersect(&result,
				const_cast<pixman_region32_t*>(&m_region),
				const_cast<pixman_region32_t*>(&other.m_region));
		return PixmanRegion(result);
	}

	// return region which is union of this region with other
	PixmanRegion unionRegion(PixmanRegion const& other) const
	{
		pixman_region32_t result;
		pixman_region32_init(&result);
		pixman_region32_union(&result,
				const_cast<pixman_region32_t*>(&m_region),
				const_cast<pixman_region32_t*>(&other.m_region));
		return PixmanRegion(result);
	}

	// return region which is a copy of this region with
	// pieces removed where it overlaps 'other'
	PixmanRegion subtractRegion(PixmanRegion const& other) const
	{
		pixman_region32_t result;
		pixman_region32_init(&result);
		pixman_region32_subtract(&result,
				const_cast<pixman_region32_t*>(&m_region),
				const_cast<pixman_region32_t*>(&other.m_region));
		return PixmanRegion(result);
	}

	// returns whether this region contains point at given x,y
	bool containsPoint(int x, int y) const
	{
		return !!pixman_region32_contains_point(
				const_cast<pixman_region32_t*>(&m_region),
				x, y, nullptr);
	}

	// returns whether this region intersects other region at all
	bool intersects(PixmanRegion const &other) const
	{
		PixmanRegion const &intersection =
				this->intersectRegion(other);
		return !intersection.isEmpty();
	}

	// returns whether this region entirely contains 'other'
	bool containsEntirely(PixmanRegion const &other) const
	{
		PixmanRegion const &subbed =
				other.subtractRegion(*this);
		return subbed.isEmpty();
	}

	// returns whether this region is empty
	bool isEmpty() const
	{
		return !pixman_region32_not_empty(
				const_cast<pixman_region32_t*>(&m_region)
				);
	}

	// returns whether this region covers exactly the same
	// area as 'other'
	bool isEqual(PixmanRegion const& other) const
	{
		return !!pixman_region32_equal(
				const_cast<pixman_region32_t*>(&m_region),
				const_cast<pixman_region32_t*>(&other.m_region));
	}

	pixman_box32_t getExtents()
	{
		pixman_box32_t* extents =
				pixman_region32_extents(&m_region);
		return *extents; // return copy
	}

	// Returns pointer to array of boxes that make up the region.
	// Array should be treated read-only, non-user-freed,
	// and should not be accessed after further modifications to
	// this region or region destruction.
	void getBoxes(const pixman_box32_t **box_list_ptr_out, int *num_boxes_out) const
	{
		*box_list_ptr_out = pixman_region32_rectangles(
				const_cast<pixman_region32_t*>(&m_region),
				num_boxes_out);
	}

	/** END PUBLIC *******************/

protected:

	void freeInternal()
	{
		pixman_region32_fini(&m_region);
	}

private:
	pixman_region32_t m_region;
};


#ifdef PIMAN_REGION_TEST_MAIN
#include <cassert>
void main()
{
	PixmanRegion r1(0,0, 10,10);
	PixmanRegion r2(5,5, 10,10);
	auto isect = r1.intersectRegion(r2);
	PixmanRegion sub = r1.subtractRegion(r2);
	auto uni = r1.unionRegion(r2);

	pixman_box32_t const *box_list_ptr;
	int num_boxes;
	sub.getBoxes(&box_list_ptr, &num_boxes);
	assert(num_boxes == 2);
	// Note - constants below aren't the only correct solution
	// but they will be what pixman does, barring big changes to
	// pixman's internal strategy
	assert(box_list_ptr[0].x1 == 0);
	assert(box_list_ptr[0].y1 == 0);
	assert(box_list_ptr[0].x2 == 10);
	assert(box_list_ptr[0].y2 == 50);
	assert(box_list_ptr[1].x1 == 0);
	assert(box_list_ptr[1].y1 == 5);
	assert(box_list_ptr[1].x2 == 5);
	assert(box_list_ptr[1].y2 == 10);
}
#endif


#endif /* PIXMANREGION_HPP_ */
