/*******************************************************************************************************************
Copyright (c) 2012 Cycling '74

Permission is hereby granted, free of charge, to any person obtaining a copy of this software
and associated documentation files (the "Software"), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies
or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*******************************************************************************************************************/


#ifndef GENLIB_H
#define GENLIB_H 1

#include "genlib_common.h"

//////////// genlib.h ////////////
// genlib.h -- max (gen~) version

#ifndef GEN_WINDOWS
#ifndef _SIZE_T
#define	_SIZE_T
typedef __typeof__(sizeof(int)) size_t;
#endif
#endif

#ifndef __INT32_TYPE__
#define __INT32_TYPE__ int
#endif

#ifdef MSP_ON_CLANG
	// gen~ hosted:
	typedef unsigned __INT32_TYPE__ uint32_t;
	typedef unsigned __INT64_TYPE__ uint64_t;
#else
	#ifdef __GNUC__
		 #include <stdint.h>
	#endif
#endif 

#define inf (__DBL_MAX__)
#define GEN_UINT_MAX                (4294967295)
#define TWO_TO_32             (4294967296.0)

#define C74_CONST const

// max_types.h:
#ifdef C74_X64
	typedef unsigned long long t_ptr_uint;
	typedef long long t_ptr_int; 
	typedef double t_atom_float;
	typedef t_ptr_uint t_getbytes_size;
#else
	typedef unsigned long t_ptr_uint;
	typedef long t_ptr_int; 
	typedef float t_atom_float; 
	typedef short t_getbytes_size; 
#endif

typedef uint32_t t_uint32;
typedef t_ptr_int t_atom_long;		// the type that is an A_LONG in an atom

typedef t_ptr_int t_int;			///< an integer  @ingroup misc
typedef t_ptr_uint t_ptr_size;		///< unsigned pointer-sized value for counting (like size_t)  @ingroup misc
typedef t_ptr_int t_atom_long;		///< the type that is an A_LONG in a #t_atom  @ingroup misc
typedef t_atom_long t_max_err;		///< an integer value suitable to be returned as an error code  @ingroup misc

extern "C" {

	// TODO: remove (for debugging only)
	//int printf(const char * fmt, ...);
	
	// math.h:
	extern double acos( double );
	extern double asin( double );
	extern double atan( double );
	extern double atan2( double, double );
	extern double cos( double );
	extern double sin( double );
	extern double tan( double );
	extern double acosh( double );
	extern double asinh( double );
	extern double atanh( double );
	extern double cosh( double );
	extern double sinh( double );
	extern double tanh( double );
	extern double exp ( double );
	extern double log ( double );
	extern double log10 ( double );
	extern double fmod ( double, double );
	extern double modf(double, double *);
	extern double fabs( double );
	extern double hypot ( double, double );
	//extern double pow ( double, double );
	extern double gen_msp_pow ( double, double );
	#define pow gen_msp_pow
	extern double sqrt( double );
	extern double ceil ( double );
	extern double floor ( double );
	extern double trunc ( double );
	extern double round ( double );
	extern int abs(int);
	
	extern char	*strcpy(char *, const char *);
	
	// string reference handling:
	void * genlib_obtain_reference_from_string(const char * name);
	char *genlib_reference_getname(void *ref);
	
	// buffer handling:
	t_genlib_buffer *genlib_obtain_buffer_from_reference(void *ref);
	t_genlib_err genlib_buffer_edit_begin(t_genlib_buffer *b);
	t_genlib_err genlib_buffer_edit_end(t_genlib_buffer *b, long valid);
	t_genlib_err genlib_buffer_getinfo(t_genlib_buffer *b, t_genlib_buffer_info *info);
	void genlib_buffer_dirty(t_genlib_buffer *b);
	t_genlib_err genlib_buffer_perform_begin(t_genlib_buffer *b);
	void genlib_buffer_perform_end(t_genlib_buffer *b);
	
	// data handling:
	t_genlib_data *genlib_obtain_data_from_reference(void *ref);
	t_genlib_err genlib_data_getinfo(t_genlib_data *b, t_genlib_data_info *info);
	void genlib_data_resize(t_genlib_data *b, long dim, long channels);
	void genlib_data_setbuffer(t_genlib_data *b, void *ref);
	void genlib_data_release(t_genlib_data *b);
	void genlib_data_setcursor(t_genlib_data *b, long cursor);
	long genlib_data_getcursor(t_genlib_data *b);
	
	// other notification:
	void genlib_reset_complete(void *data);
	
	// get/set state of parameters
	size_t genlib_getstatesize(CommonState *cself, getparameter_method getmethod);
	short genlib_getstate(CommonState *cself, char *state, getparameter_method getmethod);
	short genlib_setstate(CommonState *cself, const char *state, setparameter_method setmethod);
	
}; // extern "C"
	
#define genlib_sysmem_newptr(s) sysmem_newptr(s)
#define genlib_sysmem_newptrclear(s) sysmem_newptrclear(s)
#define genlib_sysmem_resizeptr(p, s) sysmem_resizeptr(p, s)
#define genlib_sysmem_resizeptrclear(p, s) sysmem_resizeptrclear(p, s)
#define genlib_sysmem_ptrsize(p) sysmem_ptrsize(p)
#define genlib_sysmem_freeptr(p) sysmem_freeptr(p)
#define genlib_sysmem_copyptr(s, d, b) sysmem_copyptr(s, d, b)
#define genlib_set_zero64(d, n) set_zero64(d, n)
#define genlib_ticks systime_ticks

#endif // GENLIB_H
