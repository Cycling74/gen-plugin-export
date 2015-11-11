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


#ifndef GENLIB_COMMON_H
#define GENLIB_COMMON_H 1

#include "genlib_common_win.h"

//////////// genlib_common.h ////////////
// common data structure header file -- this is the stuff required by the
// common code and accessed by the export and max code
	
#define DSP_GEN_MAX_SIGNALS 16	

typedef double t_sample;
typedef double t_param;
typedef char *t_ptr; 
	
typedef long t_genlib_err;
typedef enum {
	GENLIB_ERR_NONE =			0,	///< No error
	GENLIB_ERR_GENERIC =		-1,	///< Generic error
	GENLIB_ERR_INVALID_PTR =	-2,	///< Invalid Pointer
	GENLIB_ERR_DUPLICATE =		-3,	///< Duplicate
	GENLIB_ERR_OUT_OF_MEM =		-4,	///< Out of memory
	
	GENLIB_ERR_LOOP_OVERFLOW =  100,	// too many iterations of loops in perform()
	GENLIB_ERR_NULL_BUFFER =	101	// missing signal data in perform()
	
} e_genlib_errorcodes;

typedef enum {
	GENLIB_PARAMTYPE_FLOAT	=	0,
	GENLIB_PARAMTYPE_SYM	= 	1
} e_genlib_paramtypes;

struct ParamInfo
{
	t_param defaultvalue;
	void * defaultref;
	char hasinputminmax;
	char hasminmax;
	t_param inputmin, inputmax;
	t_param outputmin, outputmax;
	const char *name;
	const char *units;
	int paramtype;		// 0 -> float64, 1 -> symbol (table name)
	t_param exp;			// future, for scaling
};

struct CommonState
{
	t_sample sr;
	int vs;	
	int numins;
	int numouts;
	const char **inputnames;
	const char **outputnames;
	int numparams;
	ParamInfo *params;
	
	void * parammap;	// implementation-dependent
	void * api;			// implementation-dependent
};

// opaque interface to float32 buffer:
typedef struct _genlib_buffer t_genlib_buffer;
typedef struct {
	char b_name[256];	///< name of the buffer
	float *b_samples;	///< stored with interleaved channels if multi-channel
	long b_frames;		///< number of sample frames (each one is sizeof(float) * b_nchans bytes)
	long b_nchans;		///< number of channels
	long b_size;		///< size of buffer in floats
	float b_sr;			///< sampling rate of the buffer
	long b_modtime;		///< last modified time ("dirty" method)
	long b_rfu[57];		///< reserved for future use
} t_genlib_buffer_info;

// opaque interface to float64 buffer: 
typedef struct _genlib_data t_genlib_data;
typedef struct {
	int					dim, channels;
	t_sample *			data;
} t_genlib_data_info;

typedef void (*setparameter_method) (CommonState *, long, t_param, void *);
typedef void (*getparameter_method) (CommonState *, long, t_param *);

#endif // GENLIB_COMMON_H


