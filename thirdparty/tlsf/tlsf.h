/*
 This is modified version of TLSF 2.4.6 from http://www.gii.upv.es/tlsf/main/repo.html
 Modifications:
 * Removed all functionality dependent on system API so it can be used on unknown platform
 * Removed mt locks and statistics
 * Removed realloc/calloc
 * Strict integer types
 * C++ conformance (also placed to namespace)
*/

/*
 * Two Levels Segregate Fit memory allocator (TLSF)
 * Version 2.4.6
 *
 * Written by Miguel Masmano Tello <mimastel@doctor.upv.es>
 *
 * Thanks to Ismael Ripoll for his suggestions and reviews
 *
 * Copyright (C) 2008, 2007, 2006, 2005, 2004
 *
 * This code is released using a dual license strategy: GPL/LGPL
 * You can choose the licence that better fits your requirements.
 *
 * Released under the terms of the GNU General Public License Version 2.0
 * Released under the terms of the GNU Lesser General Public License Version 2.1
 *
 */

#ifndef _TLSF_H_
#define _TLSF_H_

#include <cstddef>
#include <cstdint>

namespace tlsf {
    size_t init_memory_pool(size_t, void *);
    void destroy_memory_pool(void *);
    size_t add_new_area(void *, size_t, void *);
    
    void *malloc_ex(size_t, void *);
    void free_ex(void *, void *);
}

#endif
