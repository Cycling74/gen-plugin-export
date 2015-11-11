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


#include "genlib.h"
#include "genlib_exportfunctions.h"
#include "genlib_ops.h"

namespace C74_GENPLUGIN {

int num_inputs();
int num_outputs();
int num_params();
int perform(CommonState *cself, t_sample **ins, long numins, t_sample **outs, long numouts, long n);
void reset(CommonState *cself);
void setparameter(CommonState *cself, long index, t_param value, void *ref);
void getparameter(CommonState *cself, long index, t_param *value);
const char *getparametername(CommonState *cself, long index);
t_param getparametermin(CommonState *cself, long index);
t_param getparametermax(CommonState *cself, long index);
char getparameterhasminmax(CommonState *cself, long index);
const char *getparameterunits(CommonState *cself, long index);
size_t getstatesize(CommonState *cself);
short getstate(CommonState *cself, char *state);
short setstate(CommonState *cself, const char *state);
void * create(t_param sr, long vs);
void destroy(CommonState *cself);

} // C74_GENPLUGIN::
