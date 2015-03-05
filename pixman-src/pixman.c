/* -*- Mode: c; c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t; -*- */
/*
 * Copyright © 2000 SuSE, Inc.
 * Copyright © 2007 Red Hat, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of SuSE not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  SuSE makes no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 *
 * SuSE DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL SuSE
 * BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Author:  Keith Packard, SuSE, Inc.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "pixman-private.h"

#include <stdlib.h>

pixman_implementation_t *global_implementation;

#ifdef TOOLCHAIN_SUPPORTS_ATTRIBUTE_CONSTRUCTOR
static void __attribute__((constructor))
pixman_constructor (void)
{
    global_implementation = _pixman_choose_implementation ();
}
#endif

typedef struct operator_info_t operator_info_t;

struct operator_info_t
{
    uint8_t	opaque_info[4];
};


/*
 * Computing composite region
 */
static inline pixman_bool_t
clip_general_image (pixman_region32_t * region,
                    pixman_region32_t * clip,
                    int                 dx,
                    int                 dy)
{
    if (pixman_region32_n_rects (region) == 1 &&
        pixman_region32_n_rects (clip) == 1)
    {
	pixman_box32_t *  rbox = pixman_region32_rectangles (region, NULL);
	pixman_box32_t *  cbox = pixman_region32_rectangles (clip, NULL);
	int v;

	if (rbox->x1 < (v = cbox->x1 + dx))
	    rbox->x1 = v;
	if (rbox->x2 > (v = cbox->x2 + dx))
	    rbox->x2 = v;
	if (rbox->y1 < (v = cbox->y1 + dy))
	    rbox->y1 = v;
	if (rbox->y2 > (v = cbox->y2 + dy))
	    rbox->y2 = v;
	if (rbox->x1 >= rbox->x2 || rbox->y1 >= rbox->y2)
	{
	    pixman_region32_init (region);
	    return FALSE;
	}
    }
    else if (!pixman_region32_not_empty (clip))
    {
	return FALSE;
    }
    else
    {
	if (dx || dy)
	    pixman_region32_translate (region, -dx, -dy);

	if (!pixman_region32_intersect (region, region, clip))
	    return FALSE;

	if (dx || dy)
	    pixman_region32_translate (region, dx, dy);
    }

    return pixman_region32_not_empty (region);
}

static inline pixman_bool_t
clip_source_image (pixman_region32_t * region,
                   pixman_image_t *    image,
                   int                 dx,
                   int                 dy)
{
    /* Source clips are ignored, unless they are explicitly turned on
     * and the clip in question was set by an X client. (Because if
     * the clip was not set by a client, then it is a hierarchy
     * clip and those should always be ignored for sources).
     */
    if (!image->common.clip_sources || !image->common.client_clip)
	return TRUE;

    return clip_general_image (region,
                               &image->common.clip_region,
                               dx, dy);
}

/*
 * returns FALSE if the final region is empty.  Indistinguishable from
 * an allocation failure, but rendering ignores those anyways.
 */
pixman_bool_t
_pixman_compute_composite_region32 (pixman_region32_t * region,
				    pixman_image_t *    src_image,
				    pixman_image_t *    mask_image,
				    pixman_image_t *    dest_image,
				    int32_t             src_x,
				    int32_t             src_y,
				    int32_t             mask_x,
				    int32_t             mask_y,
				    int32_t             dest_x,
				    int32_t             dest_y,
				    int32_t             width,
				    int32_t             height)
{
    region->extents.x1 = dest_x;
    region->extents.x2 = dest_x + width;
    region->extents.y1 = dest_y;
    region->extents.y2 = dest_y + height;

    region->extents.x1 = MAX (region->extents.x1, 0);
    region->extents.y1 = MAX (region->extents.y1, 0);
    region->extents.x2 = MIN (region->extents.x2, dest_image->bits.width);
    region->extents.y2 = MIN (region->extents.y2, dest_image->bits.height);

    region->data = 0;

    /* Check for empty operation */
    if (region->extents.x1 >= region->extents.x2 ||
        region->extents.y1 >= region->extents.y2)
    {
	region->extents.x1 = 0;
	region->extents.x2 = 0;
	region->extents.y1 = 0;
	region->extents.y2 = 0;
	return FALSE;
    }

    if (dest_image->common.have_clip_region)
    {
	if (!clip_general_image (region, &dest_image->common.clip_region, 0, 0))
	    return FALSE;
    }

    if (dest_image->common.alpha_map)
    {
	if (!pixman_region32_intersect_rect (region, region,
					     dest_image->common.alpha_origin_x,
					     dest_image->common.alpha_origin_y,
					     dest_image->common.alpha_map->width,
					     dest_image->common.alpha_map->height))
	{
	    return FALSE;
	}
	if (!pixman_region32_not_empty (region))
	    return FALSE;
	if (dest_image->common.alpha_map->common.have_clip_region)
	{
	    if (!clip_general_image (region, &dest_image->common.alpha_map->common.clip_region,
				     -dest_image->common.alpha_origin_x,
				     -dest_image->common.alpha_origin_y))
	    {
		return FALSE;
	    }
	}
    }

    /* clip against src */
    if (src_image->common.have_clip_region)
    {
	if (!clip_source_image (region, src_image, dest_x - src_x, dest_y - src_y))
	    return FALSE;
    }
    if (src_image->common.alpha_map && src_image->common.alpha_map->common.have_clip_region)
    {
	if (!clip_source_image (region, (pixman_image_t *)src_image->common.alpha_map,
	                        dest_x - (src_x - src_image->common.alpha_origin_x),
	                        dest_y - (src_y - src_image->common.alpha_origin_y)))
	{
	    return FALSE;
	}
    }
    /* clip against mask */
    if (mask_image && mask_image->common.have_clip_region)
    {
	if (!clip_source_image (region, mask_image, dest_x - mask_x, dest_y - mask_y))
	    return FALSE;

	if (mask_image->common.alpha_map && mask_image->common.alpha_map->common.have_clip_region)
	{
	    if (!clip_source_image (region, (pixman_image_t *)mask_image->common.alpha_map,
	                            dest_x - (mask_x - mask_image->common.alpha_origin_x),
	                            dest_y - (mask_y - mask_image->common.alpha_origin_y)))
	    {
		return FALSE;
	    }
	}
    }

    return TRUE;
}




PIXMAN_EXPORT pixman_bool_t
pixman_image_fill_rectangles (pixman_op_t                 op,
                              pixman_image_t *            dest,
			      const pixman_color_t *      color,
                              int                         n_rects,
                              const pixman_rectangle16_t *rects)
{
    pixman_box32_t stack_boxes[6];
    pixman_box32_t *boxes;
    pixman_bool_t result;
    int i;

    if (n_rects > 6)
    {
        boxes = pixman_malloc_ab (sizeof (pixman_box32_t), n_rects);
        if (boxes == NULL)
            return FALSE;
    }
    else
    {
        boxes = stack_boxes;
    }

    for (i = 0; i < n_rects; ++i)
    {
        boxes[i].x1 = rects[i].x;
        boxes[i].y1 = rects[i].y;
        boxes[i].x2 = boxes[i].x1 + rects[i].width;
        boxes[i].y2 = boxes[i].y1 + rects[i].height;
    }

    result = pixman_image_fill_boxes (op, dest, color, n_rects, boxes);

    if (boxes != stack_boxes)
        free (boxes);
    
    return result;
}

/**
 * pixman_version:
 *
 * Returns the version of the pixman library encoded in a single
 * integer as per %PIXMAN_VERSION_ENCODE. The encoding ensures that
 * later versions compare greater than earlier versions.
 *
 * A run-time comparison to check that pixman's version is greater than
 * or equal to version X.Y.Z could be performed as follows:
 *
 * <informalexample><programlisting>
 * if (pixman_version() >= PIXMAN_VERSION_ENCODE(X,Y,Z)) {...}
 * </programlisting></informalexample>
 *
 * See also pixman_version_string() as well as the compile-time
 * equivalents %PIXMAN_VERSION and %PIXMAN_VERSION_STRING.
 *
 * Return value: the encoded version.
 **/
PIXMAN_EXPORT int
pixman_version (void)
{
    return PIXMAN_VERSION;
}

/**
 * pixman_version_string:
 *
 * Returns the version of the pixman library as a human-readable string
 * of the form "X.Y.Z".
 *
 * See also pixman_version() as well as the compile-time equivalents
 * %PIXMAN_VERSION_STRING and %PIXMAN_VERSION.
 *
 * Return value: a string containing the version.
 **/
PIXMAN_EXPORT const char*
pixman_version_string (void)
{
    return PIXMAN_VERSION_STRING;
}

/**
 * pixman_format_supported_source:
 * @format: A pixman_format_code_t format
 *
 * Return value: whether the provided format code is a supported
 * format for a pixman surface used as a source in
 * rendering.
 *
 * Currently, all pixman_format_code_t values are supported.
 **/
PIXMAN_EXPORT pixman_bool_t
pixman_format_supported_source (pixman_format_code_t format)
{
    switch (format)
    {
    /* 32 bpp formats */
    case PIXMAN_a2b10g10r10:
    case PIXMAN_x2b10g10r10:
    case PIXMAN_a2r10g10b10:
    case PIXMAN_x2r10g10b10:
    case PIXMAN_a8r8g8b8:
    case PIXMAN_a8r8g8b8_sRGB:
    case PIXMAN_x8r8g8b8:
    case PIXMAN_a8b8g8r8:
    case PIXMAN_x8b8g8r8:
    case PIXMAN_b8g8r8a8:
    case PIXMAN_b8g8r8x8:
    case PIXMAN_r8g8b8a8:
    case PIXMAN_r8g8b8x8:
    case PIXMAN_r8g8b8:
    case PIXMAN_b8g8r8:
    case PIXMAN_r5g6b5:
    case PIXMAN_b5g6r5:
    case PIXMAN_x14r6g6b6:
    /* 16 bpp formats */
    case PIXMAN_a1r5g5b5:
    case PIXMAN_x1r5g5b5:
    case PIXMAN_a1b5g5r5:
    case PIXMAN_x1b5g5r5:
    case PIXMAN_a4r4g4b4:
    case PIXMAN_x4r4g4b4:
    case PIXMAN_a4b4g4r4:
    case PIXMAN_x4b4g4r4:
    /* 8bpp formats */
    case PIXMAN_a8:
    case PIXMAN_r3g3b2:
    case PIXMAN_b2g3r3:
    case PIXMAN_a2r2g2b2:
    case PIXMAN_a2b2g2r2:
    case PIXMAN_c8:
    case PIXMAN_g8:
    case PIXMAN_x4a4:
    /* Collides with PIXMAN_c8
       case PIXMAN_x4c4:
     */
    /* Collides with PIXMAN_g8
       case PIXMAN_x4g4:
     */
    /* 4bpp formats */
    case PIXMAN_a4:
    case PIXMAN_r1g2b1:
    case PIXMAN_b1g2r1:
    case PIXMAN_a1r1g1b1:
    case PIXMAN_a1b1g1r1:
    case PIXMAN_c4:
    case PIXMAN_g4:
    /* 1bpp formats */
    case PIXMAN_a1:
    case PIXMAN_g1:
    /* YUV formats */
    case PIXMAN_yuy2:
    case PIXMAN_yv12:
	return TRUE;

    default:
	return FALSE;
    }
}

/**
 * pixman_format_supported_destination:
 * @format: A pixman_format_code_t format
 *
 * Return value: whether the provided format code is a supported
 * format for a pixman surface used as a destination in
 * rendering.
 *
 * Currently, all pixman_format_code_t values are supported
 * except for the YUV formats.
 **/
PIXMAN_EXPORT pixman_bool_t
pixman_format_supported_destination (pixman_format_code_t format)
{
    /* YUV formats cannot be written to at the moment */
    if (format == PIXMAN_yuy2 || format == PIXMAN_yv12)
	return FALSE;

    return pixman_format_supported_source (format);
}

PIXMAN_EXPORT pixman_bool_t
pixman_compute_composite_region (pixman_region16_t * region,
                                 pixman_image_t *    src_image,
                                 pixman_image_t *    mask_image,
                                 pixman_image_t *    dest_image,
                                 int16_t             src_x,
                                 int16_t             src_y,
                                 int16_t             mask_x,
                                 int16_t             mask_y,
                                 int16_t             dest_x,
                                 int16_t             dest_y,
                                 uint16_t            width,
                                 uint16_t            height)
{
    pixman_region32_t r32;
    pixman_bool_t retval;

    pixman_region32_init (&r32);

    retval = _pixman_compute_composite_region32 (
	&r32, src_image, mask_image, dest_image,
	src_x, src_y, mask_x, mask_y, dest_x, dest_y,
	width, height);

    if (retval)
    {
	if (!pixman_region16_copy_from_region32 (region, &r32))
	    retval = FALSE;
    }

    pixman_region32_fini (&r32);
    return retval;
}
