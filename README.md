PIXMAN-REGION
=============

<https://github.com/adamdmoss/pixman-region>

This is a library for performing boolean (and other) operations on
regions (lists of rectangles) - intersection, union, etc.

It's based on pixman (which is itself based on the well-used X11
implementations of these routines), with most of the stuff stripped
out which isn't related to region operations.

The raw C pixman region interface is preserved intact, or you can
use the included - new shiny - C++ binding.

BUILDING
========

This is intended to be used as a static library or included wholesale
into your application.

An included CMake configuration is provided for building the library
and the test app.  Alternatively, if you don't like CMake:

* Add pixman-src/*.c to your build system.
* Add the project root to your include paths

USING
=====

* `#include <pixman-region/pixman-region.h>`
  if you want to raw pixman C interface; the pixman_region*
  functions should be usable, see private/pixman.h for description.
* `#include <pixman-region/PixmanRegion.hpp>` header-only implementation
  for a C++ objectified region wrapper.  This may be cleaner for your
  purposes, and offers a (large, useful) subset of the full
  pixman region interface.

LICENSE
=======

MIT license, see COPYING

TODO
====

* Strip more unused pixman interface/implementation away
* Better unit test and/or examples (there is a weak assertion-
based test, cribbed from the pixman source, as
test/pixman-region-test.c)

CONTACT
=======

Adam D. Moss <c@yotes.com>

UPSTREAM
========

This source is based on <http://cgit.freedesktop.org/pixman/>
revision 594e6a6c93e92fcfb495e987aec5617f6c37f467
