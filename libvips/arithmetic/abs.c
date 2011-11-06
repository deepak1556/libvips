/* im_abs()
 *
 * Copyright: 1990, N. Dessipris, based on im_powtra()
 * Author: Nicos Dessipris
 * Written on: 02/05/1990
 * Modified on: 
 * 5/5/93 J.Cupitt
 *	- adapted from im_lintra to work with partial images
 *	- complex and signed support added
 * 30/6/93 JC
 *	- adapted for partial v2
 *	- ANSI conversion
 *	- spe29873r6k3h()**!@lling errors removed
 * 9/2/95 JC
 *	- adapted for im_wrap...
 * 20/6/02 JC
 *	- tiny speed up
 * 8/12/06
 * 	- add liboil support
 * 28/8/09
 * 	- gtkdoc
 * 	- tiny polish
 * 31/7/10
 * 	- remove liboil
 * 6/11/11
 * 	- redone as a class
 */

/*

    Copyright (C) 1991-2005 The National Gallery

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

 */

/*

    These files are distributed with VIPS - http://www.vips.ecs.soton.ac.uk

 */

/*
#define DEBUG
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /*HAVE_CONFIG_H*/
#include <vips/intl.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <vips/vips.h>

#include "arithmetic.h"
#include "unary.h"

/** 
 * VipsAbs:
 * @in: input #VipsImage
 * @out: output #VipsImage
 *
 * This operation finds the absolute value of an image. It does a copy for 
 * unsigned integer types, negate for negative values in 
 * signed integer types, <function>fabs(3)</function> for 
 * float types, and calculate modulus for complex 
 * types. 
 *
 * See also: im_sign().
 *
 * Returns: 0 on success, -1 on error
 */

typedef VipsUnary VipsAbs;
typedef VipsUnaryClass VipsAbsClass;

G_DEFINE_TYPE( VipsAbs, vips_abs, VIPS_TYPE_UNARY );

static int
vips_abs_build( VipsObject *object )
{
	VipsArithmetic *arithmetic = VIPS_ARITHMETIC( object );
	VipsUnary *unary = (VipsUnary *) object;

	if( unary->in &&
		vips_band_format_isuint( unary->in->BandFmt ) ) {
		/* This isn't set by arith until build(), so we have to set
		 * again here.
		 *
		 * Should arith set out in _init()?
		 */
		g_object_set( arithmetic, "out", vips_image_new(), NULL ); 

		return( vips_image_write( unary->in, arithmetic->out ) );
	}

	if( VIPS_OBJECT_CLASS( vips_abs_parent_class )->build( object ) )
		return( -1 );

	return( 0 );
}

/* Integer abs operation: just test and negate.
 */
#define ABS_INT( TYPE ) { \
	TYPE *p = (TYPE *) in[0]; \
	TYPE *q = (TYPE *) out; \
	int x; \
	\
	for( x = 0; x < sz; x++ ) { \
		TYPE v = p[x]; \
		\
		if( v < 0 ) \
			q[x] = 0 - v; \
		else \
			q[x] = v; \
	} \
}

/* Float abs operation: call fabs().
 */
#define ABS_FLOAT( TYPE ) { \
	TYPE *p = (TYPE *) in[0]; \
	TYPE *q = (TYPE *) out; \
	int x; \
	\
	for( x = 0; x < sz; x++ ) \
		q[x] = fabs( p[x] ); \
}

/* Complex abs operation: calculate modulus.
 */

#ifdef HAVE_HYPOT

#define ABS_COMPLEX( TYPE ) { \
	TYPE *p = (TYPE *) in[0]; \
	TYPE *q = (TYPE *) out; \
	int x; \
	\
	for( x = 0; x < sz; x++ ) { \
		q[x] = hypot( p[0], p[1] ); \
		p += 2; \
	} \
}

#else /*HAVE_HYPOT*/

#define ABS_COMPLEX( TYPE ) { \
	TYPE *p = (TYPE *) in[0]; \
	TYPE *q = (TYPE *) out; \
	int x; \
	\
	for( x = 0; x < sz; x++ ) { \
		double rp = p[0]; \
		double ip = p[1]; \
		double abs_rp = fabs( rp ); \
		double abs_ip = fabs( ip ); \
		\
		if( abs_rp > abs_ip ) { \
			double temp = ip / rp; \
			\
			q[x]= abs_rp * sqrt( 1.0 + temp * temp ); \
		} \
		else { \
			double temp = rp / ip; \
			\
			q[x]= abs_ip * sqrt( 1.0 + temp * temp ); \
		} \
		\
		p += 2; \
	} \
}

#endif /*HAVE_HYPOT*/

static void
vips_abs_buffer( VipsArithmetic *arithmetic, PEL *out, PEL **in, int width )
{
	VipsImage *im = arithmetic->ready[0];
	int sz = width * im->Bands;

	/* Abs all input types.
         */
        switch( im->BandFmt ) {
        case VIPS_FORMAT_CHAR: 		ABS_INT( signed char ); break; 
        case VIPS_FORMAT_SHORT: 	ABS_INT( signed short ); break; 
        case VIPS_FORMAT_INT: 		ABS_INT( signed int ); break; 
        case VIPS_FORMAT_FLOAT: 	ABS_FLOAT( float ); break; 
        case VIPS_FORMAT_DOUBLE:	ABS_FLOAT( float ); break; 
        case VIPS_FORMAT_COMPLEX:	ABS_COMPLEX( float ); break;
        case VIPS_FORMAT_DPCOMPLEX:	ABS_COMPLEX( double ); break;

	default:
		g_assert( 0 );
	}
}

/* Save a bit of typing.
 */
#define UC VIPS_FORMAT_UCHAR
#define C VIPS_FORMAT_CHAR
#define US VIPS_FORMAT_USHORT
#define S VIPS_FORMAT_SHORT
#define UI VIPS_FORMAT_UINT
#define I VIPS_FORMAT_INT
#define F VIPS_FORMAT_FLOAT
#define X VIPS_FORMAT_COMPLEX
#define D VIPS_FORMAT_DOUBLE
#define DX VIPS_FORMAT_DPCOMPLEX

/* Format doesn't change with abs.
 */
static const VipsBandFormat vips_bandfmt_abs[10] = {
/* UC  C   US  S   UI  I   F   X   D   DX */
   UC, C,  US, S,  UI, I,  F,  X,  D,  DX 
};

static void
vips_abs_class_init( VipsAbsClass *class )
{
	VipsObjectClass *object_class = (VipsObjectClass *) class;
	VipsArithmeticClass *aclass = VIPS_ARITHMETIC_CLASS( class );

	object_class->nickname = "abs";
	object_class->description = _( "absolute value of an image" );
	object_class->build = vips_abs_build;

	vips_arithmetic_set_format_table( aclass, vips_bandfmt_abs );

	aclass->process_line = vips_abs_buffer;
}

static void
vips_abs_init( VipsAbs *abs )
{
}

int
vips_abs( VipsImage *in, VipsImage **out, ... )
{
	va_list ap;
	int result;

	va_start( ap, out );
	result = vips_call_split( "abs", ap, in, out );
	va_end( ap );

	return( result );
}