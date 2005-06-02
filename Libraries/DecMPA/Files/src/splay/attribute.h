/*
  align attribut definition (g++)
  Copyright (C) 2001  Martin Vogt

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU Library General Public License as published by
  the Free Software Foundation.

  For more information look at the file License.txt in this package

 */


#ifndef __ATTRIBUTE_H
#define __ATTRIBUTE_H


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/* use gcc attribs to align critical data structures */

#ifdef ATTRIBUTE_ALIGNED_MAX
#define ATTR_ALIGN(align) __attribute__ \
                          ((__aligned__ ((ATTRIBUTE_ALIGNED_MAX <align) ? \
                           ATTRIBUTE_ALIGNED_MAX : align)))
#else
#define ATTR_ALIGN(align)
#endif


#endif
