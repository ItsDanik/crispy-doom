//
// Copyright(C) 1993-1996 Id Software, Inc.
// Copyright(C) 2005-2014 Simon Howard
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// DESCRIPTION:
//	Fixed point implementation.
//



#include "stdlib.h"

#include "doomtype.h"
#include "i_system.h"

#include "m_fixed.h"




// Fixme. __USE_C_FIXED__ or something.

fixed_t
FixedMul
( fixed_t	a,
  fixed_t	b )
{
    return ((int64_t) a * (int64_t) b) >> FRACBITS;
}



//
// FixedDiv, C version.
//

#ifdef MISTER_HYBRID
// MiSTer: the ARM has no division instruction, and the C library's 64-bit
// one took 5% of the frame time at 640x240 (R_MapPlane() divides for every
// row of a floor). The VFP divides doubles: a << 16 is exact in one, and the
// quotient rounded to a double is at most 2^-22 from the true one, so its
// whole part is the true quotient's, or one further from 0 when that is
// just below a whole number: the remainder says which.
fixed_t FixedDiv(fixed_t a, fixed_t b)
{
    if ((abs(a) >> 14) >= abs(b))
    {
	return (a^b) < 0 ? INT_MIN : INT_MAX;
    }
    else
    {
	const int64_t n = (int64_t) a * FRACUNIT;
	int q = (int) ((double) a * FRACUNIT / b);
	const int64_t r = n - (int64_t) q * b;

	// the C quotient's remainder has n's sign (or is 0)
	if (r != 0 && (r < 0) != (n < 0))
	{
	    q += q > 0 ? -1 : 1;
	}

	return q;
    }
}
#else
fixed_t FixedDiv(fixed_t a, fixed_t b)
{
    if ((abs(a) >> 14) >= abs(b))
    {
	return (a^b) < 0 ? INT_MIN : INT_MAX;
    }
    else
    {
	int64_t result;

	result = ((int64_t) a << FRACBITS) / b;

	return (fixed_t) result;
    }
}
#endif

