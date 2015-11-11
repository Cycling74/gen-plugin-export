#include "C74_GENPLUGIN.h"

namespace C74_GENPLUGIN {


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


// global noise generator
Noise noise;
static const int GENLIB_LOOPCOUNT_BAIL = 100000;


// The State struct contains all the state and procedures for the gendsp kernel
typedef struct State { 
	CommonState __commonstate;
	Delay m_delay_44;
	Delay m_delay_24;
	Delay m_delay_25;
	Delay m_delay_27;
	Delay m_delay_23;
	Delay m_delay_43;
	Delay m_delay_22;
	Delay m_delay_19;
	Delay m_delay_17;
	Delay m_delay_29;
	Delay m_delay_33;
	Delay m_delay_41;
	Delay m_delay_42;
	Delay m_delay_31;
	Delay m_delay_39;
	Delay m_delay_35;
	Delay m_delay_37;
	Delay m_delay_15;
	Delay m_delay_21;
	Delay m_delay_9;
	Delay m_delay_5;
	Delay m_delay_13;
	Delay m_delay_7;
	Delay m_delay_11;
	int __exception;
	int vectorsize;
	t_sample m_history_36;
	t_sample m_fb_3;
	t_sample m_history_34;
	t_sample m_history_40;
	t_sample m_fb_1;
	t_sample m_history_38;
	t_sample m_spread_2;
	t_sample samplerate;
	t_sample m_damp_4;
	t_sample m_history_30;
	t_sample m_history_16;
	t_sample m_history_12;
	t_sample m_history_20;
	t_sample m_history_18;
	t_sample m_history_32;
	t_sample m_history_10;
	t_sample m_history_26;
	t_sample m_history_8;
	t_sample m_history_6;
	t_sample m_history_28;
	t_sample m_history_14;
	// re-initialize all member variables;
	inline void reset(t_param __sr, int __vs) { 
		__exception = 0;
		vectorsize = __vs;
		samplerate = __sr;
		m_fb_1 = 0.9;
		m_spread_2 = 0;
		m_fb_3 = 0.5;
		m_damp_4 = 0.5;
		m_delay_5.reset("m_delay_5", 2000);
		m_history_6 = 0;
		m_delay_7.reset("m_delay_7", 2000);
		m_history_8 = 0;
		m_delay_9.reset("m_delay_9", 2000);
		m_history_10 = 0;
		m_delay_11.reset("m_delay_11", 2000);
		m_history_12 = 0;
		m_delay_13.reset("m_delay_13", 2000);
		m_history_14 = 0;
		m_delay_15.reset("m_delay_15", 2000);
		m_history_16 = 0;
		m_delay_17.reset("m_delay_17", 2000);
		m_history_18 = 0;
		m_delay_19.reset("m_delay_19", 2000);
		m_history_20 = 0;
		m_delay_21.reset("m_delay_21", 2000);
		m_delay_22.reset("m_delay_22", 2000);
		m_delay_23.reset("m_delay_23", 2000);
		m_delay_24.reset("m_delay_24", 2000);
		m_delay_25.reset("m_delay_25", 2000);
		m_history_26 = 0;
		m_delay_27.reset("m_delay_27", 2000);
		m_history_28 = 0;
		m_delay_29.reset("m_delay_29", 2000);
		m_history_30 = 0;
		m_delay_31.reset("m_delay_31", 2000);
		m_history_32 = 0;
		m_delay_33.reset("m_delay_33", 2000);
		m_history_34 = 0;
		m_delay_35.reset("m_delay_35", 2000);
		m_history_36 = 0;
		m_delay_37.reset("m_delay_37", 2000);
		m_history_38 = 0;
		m_delay_39.reset("m_delay_39", 2000);
		m_history_40 = 0;
		m_delay_41.reset("m_delay_41", 2000);
		m_delay_42.reset("m_delay_42", 2000);
		m_delay_43.reset("m_delay_43", 2000);
		m_delay_44.reset("m_delay_44", 2000);
		genlib_reset_complete(this);
		
	};
	// the signal processing routine;
	inline int perform(t_sample ** __ins, t_sample ** __outs, int __n) { 
		vectorsize = __n;
		const t_sample * __in1 = __ins[0];
		const t_sample * __in2 = __ins[1];
		t_sample * __out1 = __outs[0];
		t_sample * __out2 = __outs[1];
		if (__exception) { 
			return __exception;
			
		} else if (( (__in1 == 0) || (__in2 == 0) || (__out1 == 0) || (__out2 == 0) )) { 
			__exception = GENLIB_ERR_NULL_BUFFER;
			return __exception;
			
		};
		t_sample mul_125 = (m_fb_3 * 0.5);
		t_sample mul_68 = (m_fb_3 * 0.5);
		t_sample add_111 = (225 + m_spread_2);
		t_sample add_54 = (225 + m_spread_2);
		t_sample add_113 = (341 + m_spread_2);
		t_sample add_56 = (341 + m_spread_2);
		t_sample add_123 = (441 + m_spread_2);
		t_sample add_66 = (441 + m_spread_2);
		t_sample add_109 = (556 + m_spread_2);
		t_sample add_52 = (556 + m_spread_2);
		t_sample damp_23 = m_damp_4;
		t_sample damp_94 = damp_23;
		t_sample damp_93 = damp_23;
		t_sample damp_95 = damp_23;
		t_sample damp_96 = damp_23;
		t_sample damp_97 = damp_23;
		t_sample damp_98 = damp_23;
		t_sample damp_99 = damp_23;
		t_sample damp_100 = damp_23;
		t_sample damp_21 = damp_23;
		t_sample damp_28 = damp_23;
		t_sample damp_27 = damp_23;
		t_sample damp_26 = damp_23;
		t_sample damp_25 = damp_23;
		t_sample damp_24 = damp_23;
		t_sample damp_22 = damp_23;
		t_sample add_116 = (1557 + m_spread_2);
		t_sample rsub_29 = (1 - damp_94);
		t_sample add_115 = (1617 + m_spread_2);
		t_sample rsub_132 = (1 - damp_93);
		t_sample add_117 = (1491 + m_spread_2);
		t_sample rsub_144 = (1 - damp_95);
		t_sample add_118 = (1422 + m_spread_2);
		t_sample rsub_156 = (1 - damp_96);
		t_sample add_119 = (1356 + m_spread_2);
		t_sample rsub_168 = (1 - damp_97);
		t_sample add_120 = (1277 + m_spread_2);
		t_sample rsub_180 = (1 - damp_98);
		t_sample add_121 = (1188 + m_spread_2);
		t_sample rsub_195 = (1 - damp_99);
		t_sample add_122 = (1116 + m_spread_2);
		t_sample rsub_206 = (1 - damp_100);
		t_sample add_58 = (1617 + m_spread_2);
		t_sample rsub_236 = (1 - damp_21);
		t_sample add_65 = (1116 + m_spread_2);
		t_sample rsub_248 = (1 - damp_28);
		t_sample add_64 = (1188 + m_spread_2);
		t_sample rsub_260 = (1 - damp_27);
		t_sample add_63 = (1277 + m_spread_2);
		t_sample rsub_273 = (1 - damp_26);
		t_sample add_62 = (1356 + m_spread_2);
		t_sample rsub_285 = (1 - damp_25);
		t_sample add_61 = (1422 + m_spread_2);
		t_sample rsub_296 = (1 - damp_24);
		t_sample add_60 = (1491 + m_spread_2);
		t_sample rsub_306 = (1 - damp_23);
		t_sample add_59 = (1557 + m_spread_2);
		t_sample rsub_318 = (1 - damp_22);
		// the main sample loop;
		while ((__n--)) { 
			const t_sample in1 = (*(__in1++));
			const t_sample in2 = (*(__in2++));
			t_sample mul_127 = (in1 * 0.015);
			t_sample tap_36 = m_delay_5.read_linear(add_116);
			t_sample gen_107 = tap_36;
			t_sample mul_34 = (tap_36 * damp_94);
			t_sample mul_32 = (m_history_6 * rsub_29);
			t_sample add_33 = (mul_34 + mul_32);
			t_sample mul_30 = (add_33 * m_fb_1);
			t_sample add_37 = (mul_127 + mul_30);
			t_sample history_31_next_38 = add_33;
			t_sample tap_129 = m_delay_7.read_linear(add_115);
			t_sample gen_126 = tap_129;
			t_sample mul_133 = (tap_129 * damp_93);
			t_sample mul_134 = (m_history_8 * rsub_132);
			t_sample add_135 = (mul_133 + mul_134);
			t_sample mul_130 = (add_135 * m_fb_1);
			t_sample add_128 = (mul_127 + mul_130);
			t_sample history_31_next_136 = add_135;
			t_sample tap_141 = m_delay_9.read_linear(add_117);
			t_sample gen_106 = tap_141;
			t_sample mul_145 = (tap_141 * damp_95);
			t_sample mul_146 = (m_history_10 * rsub_144);
			t_sample add_147 = (mul_145 + mul_146);
			t_sample mul_142 = (add_147 * m_fb_1);
			t_sample add_140 = (mul_127 + mul_142);
			t_sample history_31_next_148 = add_147;
			t_sample tap_153 = m_delay_11.read_linear(add_118);
			t_sample gen_105 = tap_153;
			t_sample mul_157 = (tap_153 * damp_96);
			t_sample mul_158 = (m_history_12 * rsub_156);
			t_sample add_159 = (mul_157 + mul_158);
			t_sample mul_154 = (add_159 * m_fb_1);
			t_sample add_152 = (mul_127 + mul_154);
			t_sample history_31_next_160 = add_159;
			t_sample tap_165 = m_delay_13.read_linear(add_119);
			t_sample gen_104 = tap_165;
			t_sample mul_169 = (tap_165 * damp_97);
			t_sample mul_170 = (m_history_14 * rsub_168);
			t_sample add_171 = (mul_169 + mul_170);
			t_sample mul_166 = (add_171 * m_fb_1);
			t_sample add_164 = (mul_127 + mul_166);
			t_sample history_31_next_172 = add_171;
			t_sample tap_177 = m_delay_15.read_linear(add_120);
			t_sample gen_103 = tap_177;
			t_sample mul_181 = (tap_177 * damp_98);
			t_sample mul_182 = (m_history_16 * rsub_180);
			t_sample add_183 = (mul_181 + mul_182);
			t_sample mul_178 = (add_183 * m_fb_1);
			t_sample add_176 = (mul_127 + mul_178);
			t_sample history_31_next_184 = add_183;
			t_sample tap_188 = m_delay_17.read_linear(add_121);
			t_sample gen_102 = tap_188;
			t_sample mul_196 = (tap_188 * damp_99);
			t_sample mul_191 = (m_history_18 * rsub_195);
			t_sample add_197 = (mul_196 + mul_191);
			t_sample mul_189 = (add_197 * m_fb_1);
			t_sample add_194 = (mul_127 + mul_189);
			t_sample history_31_next_192 = add_197;
			t_sample tap_200 = m_delay_19.read_linear(add_122);
			t_sample gen_101 = tap_200;
			t_sample mul_207 = (tap_200 * damp_100);
			t_sample mul_202 = (m_history_20 * rsub_206);
			t_sample add_209 = (mul_207 + mul_202);
			t_sample mul_208 = (add_209 * m_fb_1);
			t_sample add_205 = (mul_127 + mul_208);
			t_sample history_31_next_203 = add_209;
			t_sample add_124 = ((((((((gen_101 + gen_102) + gen_103) + gen_104) + gen_105) + gen_106) + gen_126) + gen_107) + 0);
			t_sample tap_50 = m_delay_21.read_linear(add_109);
			t_sample sub_46 = (add_124 - tap_50);
			t_sample mul_48 = (tap_50 * mul_125);
			t_sample add_47 = (add_124 + mul_48);
			t_sample tap_216 = m_delay_22.read_linear(add_123);
			t_sample sub_213 = (sub_46 - tap_216);
			t_sample mul_215 = (tap_216 * mul_125);
			t_sample add_212 = (sub_46 + mul_215);
			t_sample tap_222 = m_delay_23.read_linear(add_113);
			t_sample sub_219 = (sub_213 - tap_222);
			t_sample mul_221 = (tap_222 * mul_125);
			t_sample add_218 = (sub_213 + mul_221);
			t_sample tap_228 = m_delay_24.read_linear(add_111);
			t_sample sub_225 = (sub_219 - tap_228);
			t_sample mul_227 = (tap_228 * mul_125);
			t_sample add_224 = (sub_219 + mul_227);
			t_sample out1 = sub_225;
			t_sample mul_70 = (in2 * 0.015);
			t_sample tap_230 = m_delay_25.read_linear(add_58);
			t_sample gen_69 = tap_230;
			t_sample mul_237 = (tap_230 * damp_21);
			t_sample mul_232 = (m_history_26 * rsub_236);
			t_sample add_239 = (mul_237 + mul_232);
			t_sample mul_238 = (add_239 * m_fb_1);
			t_sample add_235 = (mul_70 + mul_238);
			t_sample history_31_next_233 = add_239;
			t_sample tap_242 = m_delay_27.read_linear(add_65);
			t_sample gen_39 = tap_242;
			t_sample mul_249 = (tap_242 * damp_28);
			t_sample mul_244 = (m_history_28 * rsub_248);
			t_sample add_250 = (mul_249 + mul_244);
			t_sample mul_251 = (add_250 * m_fb_1);
			t_sample add_247 = (mul_70 + mul_251);
			t_sample history_31_next_245 = add_250;
			t_sample tap_254 = m_delay_29.read_linear(add_64);
			t_sample gen_40 = tap_254;
			t_sample mul_261 = (tap_254 * damp_27);
			t_sample mul_256 = (m_history_30 * rsub_260);
			t_sample add_263 = (mul_261 + mul_256);
			t_sample mul_262 = (add_263 * m_fb_1);
			t_sample add_259 = (mul_70 + mul_262);
			t_sample history_31_next_257 = add_263;
			t_sample tap_266 = m_delay_31.read_linear(add_63);
			t_sample gen_41 = tap_266;
			t_sample mul_274 = (tap_266 * damp_26);
			t_sample mul_269 = (m_history_32 * rsub_273);
			t_sample add_275 = (mul_274 + mul_269);
			t_sample mul_267 = (add_275 * m_fb_1);
			t_sample add_272 = (mul_70 + mul_267);
			t_sample history_31_next_270 = add_275;
			t_sample tap_278 = m_delay_33.read_linear(add_62);
			t_sample gen_42 = tap_278;
			t_sample mul_286 = (tap_278 * damp_25);
			t_sample mul_281 = (m_history_34 * rsub_285);
			t_sample add_287 = (mul_286 + mul_281);
			t_sample mul_279 = (add_287 * m_fb_1);
			t_sample add_284 = (mul_70 + mul_279);
			t_sample history_31_next_282 = add_287;
			t_sample tap_290 = m_delay_35.read_linear(add_61);
			t_sample gen_43 = tap_290;
			t_sample mul_297 = (tap_290 * damp_24);
			t_sample mul_299 = (m_history_36 * rsub_296);
			t_sample add_298 = (mul_297 + mul_299);
			t_sample mul_291 = (add_298 * m_fb_1);
			t_sample add_295 = (mul_70 + mul_291);
			t_sample history_31_next_293 = add_298;
			t_sample tap_310 = m_delay_37.read_linear(add_60);
			t_sample gen_44 = tap_310;
			t_sample mul_311 = (tap_310 * damp_23);
			t_sample mul_303 = (m_history_38 * rsub_306);
			t_sample add_307 = (mul_311 + mul_303);
			t_sample mul_308 = (add_307 * m_fb_1);
			t_sample add_305 = (mul_70 + mul_308);
			t_sample history_31_next_309 = add_307;
			t_sample tap_321 = m_delay_39.read_linear(add_59);
			t_sample gen_45 = tap_321;
			t_sample mul_323 = (tap_321 * damp_22);
			t_sample mul_315 = (m_history_40 * rsub_318);
			t_sample add_319 = (mul_323 + mul_315);
			t_sample mul_322 = (add_319 * m_fb_1);
			t_sample add_317 = (mul_70 + mul_322);
			t_sample history_31_next_320 = add_319;
			t_sample add_67 = ((((((((gen_45 + gen_44) + gen_43) + gen_42) + gen_41) + gen_40) + gen_39) + gen_69) + 0);
			t_sample tap_328 = m_delay_41.read_linear(add_52);
			t_sample sub_327 = (add_67 - tap_328);
			t_sample mul_329 = (tap_328 * mul_68);
			t_sample add_326 = (add_67 + mul_329);
			t_sample tap_333 = m_delay_42.read_linear(add_66);
			t_sample sub_335 = (sub_327 - tap_333);
			t_sample mul_334 = (tap_333 * mul_68);
			t_sample add_332 = (sub_327 + mul_334);
			t_sample tap_341 = m_delay_43.read_linear(add_56);
			t_sample sub_340 = (sub_335 - tap_341);
			t_sample mul_339 = (tap_341 * mul_68);
			t_sample add_338 = (sub_335 + mul_339);
			t_sample tap_347 = m_delay_44.read_linear(add_54);
			t_sample sub_346 = (sub_340 - tap_347);
			t_sample mul_345 = (tap_347 * mul_68);
			t_sample add_344 = (sub_340 + mul_345);
			t_sample out2 = sub_346;
			m_delay_5.write(add_37);
			m_delay_44.write(add_344);
			m_delay_43.write(add_338);
			m_delay_42.write(add_332);
			m_delay_41.write(add_326);
			m_history_40 = history_31_next_320;
			m_delay_39.write(add_317);
			m_history_38 = history_31_next_309;
			m_delay_37.write(add_305);
			m_history_36 = history_31_next_293;
			m_delay_35.write(add_295);
			m_history_34 = history_31_next_282;
			m_delay_33.write(add_284);
			m_history_32 = history_31_next_270;
			m_delay_31.write(add_272);
			m_history_30 = history_31_next_257;
			m_delay_29.write(add_259);
			m_history_28 = history_31_next_245;
			m_delay_27.write(add_247);
			m_history_26 = history_31_next_233;
			m_delay_25.write(add_235);
			m_delay_24.write(add_224);
			m_delay_23.write(add_218);
			m_delay_22.write(add_212);
			m_delay_21.write(add_47);
			m_history_20 = history_31_next_203;
			m_delay_19.write(add_205);
			m_history_18 = history_31_next_192;
			m_delay_17.write(add_194);
			m_history_16 = history_31_next_184;
			m_delay_15.write(add_176);
			m_history_14 = history_31_next_172;
			m_delay_13.write(add_164);
			m_history_12 = history_31_next_160;
			m_delay_11.write(add_152);
			m_history_10 = history_31_next_148;
			m_delay_9.write(add_140);
			m_history_8 = history_31_next_136;
			m_delay_7.write(add_128);
			m_history_6 = history_31_next_38;
			m_delay_5.step();
			m_delay_7.step();
			m_delay_9.step();
			m_delay_11.step();
			m_delay_13.step();
			m_delay_15.step();
			m_delay_17.step();
			m_delay_19.step();
			m_delay_21.step();
			m_delay_22.step();
			m_delay_23.step();
			m_delay_24.step();
			m_delay_25.step();
			m_delay_27.step();
			m_delay_29.step();
			m_delay_31.step();
			m_delay_33.step();
			m_delay_35.step();
			m_delay_37.step();
			m_delay_39.step();
			m_delay_41.step();
			m_delay_42.step();
			m_delay_43.step();
			m_delay_44.step();
			// assign results to output buffer;
			(*(__out1++)) = out1;
			(*(__out2++)) = out2;
			
		};
		return __exception;
		
	};
	inline void set_fb1(t_param _value) {
		m_fb_1 = (_value < 0 ? 0 : (_value > 1 ? 1 : _value));
	};
	inline void set_spread(t_param _value) {
		m_spread_2 = (_value < 0 ? 0 : (_value > 400 ? 400 : _value));
	};
	inline void set_fb2(t_param _value) {
		m_fb_3 = (_value < 0 ? 0 : (_value > 1 ? 1 : _value));
	};
	inline void set_damp(t_param _value) {
		m_damp_4 = (_value < 0 ? 0 : (_value > 1 ? 1 : _value));
	};
	
} State;


/// 
///	Configuration for the genlib API
///

/// Number of signal inputs and outputs 

int gen_kernel_numins = 2;
int gen_kernel_numouts = 2;

int num_inputs() { return gen_kernel_numins; }
int num_outputs() { return gen_kernel_numouts; }
int num_params() { return 4; }

/// Assistive lables for the signal inputs and outputs 

const char * gen_kernel_innames[] = { "in1", "in2" };
const char * gen_kernel_outnames[] = { "out1", "out2" };

/// Invoke the signal process of a State object

int perform(CommonState *cself, t_sample **ins, long numins, t_sample **outs, long numouts, long n) { 
	State * self = (State *)cself;
	return self->perform(ins, outs, n);
}

/// Reset all parameters and stateful operators of a State object

void reset(CommonState *cself) { 
	State * self = (State *)cself;
	self->reset(cself->sr, cself->vs); 
}

/// Set a parameter of a State object 

void setparameter(CommonState *cself, long index, t_param value, void *ref) {
	State * self = (State *)cself;
	switch (index) {
		case 0: self->set_damp(value); break;
		case 1: self->set_fb1(value); break;
		case 2: self->set_fb2(value); break;
		case 3: self->set_spread(value); break;
		
		default: break;
	}
}

/// Get the value of a parameter of a State object 

void getparameter(CommonState *cself, long index, t_param *value) {
	State *self = (State *)cself;
	switch (index) {
		case 0: *value = self->m_damp_4; break;
		case 1: *value = self->m_fb_1; break;
		case 2: *value = self->m_fb_3; break;
		case 3: *value = self->m_spread_2; break;
		
		default: break;
	}
}

/// Get the name of a parameter of a State object

const char *getparametername(CommonState *cself, long index) {
	if (index >= 0 && index < cself->numparams) {
		return cself->params[index].name;
	}
	return 0;
}

/// Get the minimum value of a parameter of a State object

t_param getparametermin(CommonState *cself, long index) {
	if (index >= 0 && index < cself->numparams) {
		return cself->params[index].outputmin;
	}
	return 0;
}

/// Get the maximum value of a parameter of a State object

t_param getparametermax(CommonState *cself, long index) {
	if (index >= 0 && index < cself->numparams) {
		return cself->params[index].outputmax;
	}
	return 0;
}

/// Get parameter of a State object has a minimum and maximum value

char getparameterhasminmax(CommonState *cself, long index) {
	if (index >= 0 && index < cself->numparams) {
		return cself->params[index].hasminmax;
	}
	return 0;
}

/// Get the units of a parameter of a State object

const char *getparameterunits(CommonState *cself, long index) {
	if (index >= 0 && index < cself->numparams) {
		return cself->params[index].units;
	}
	return 0;
}

/// Get the size of the state of all parameters of a State object

size_t getstatesize(CommonState *cself) {
	return genlib_getstatesize(cself, &getparameter);
}

/// Get the state of all parameters of a State object

short getstate(CommonState *cself, char *state) {
	return genlib_getstate(cself, state, &getparameter);
}

/// set the state of all parameters of a State object

short setstate(CommonState *cself, const char *state) {
	return genlib_setstate(cself, state, &setparameter);
}

/// Allocate and configure a new State object and it's internal CommonState:

void * create(t_param sr, long vs) {
	State *self = new State;
	self->reset(sr, vs);
	ParamInfo *pi;
	self->__commonstate.inputnames = gen_kernel_innames;
	self->__commonstate.outputnames = gen_kernel_outnames;
	self->__commonstate.numins = gen_kernel_numins;
	self->__commonstate.numouts = gen_kernel_numouts;
	self->__commonstate.sr = sr;
	self->__commonstate.vs = vs;
	self->__commonstate.params = (ParamInfo *)genlib_sysmem_newptr(4 * sizeof(ParamInfo));
	self->__commonstate.numparams = 4;
	// initialize parameter 0 ("m_damp_4")
	pi = self->__commonstate.params + 0;
	pi->name = "damp";
	pi->paramtype = GENLIB_PARAMTYPE_FLOAT;
	pi->defaultvalue = self->m_damp_4;
	pi->defaultref = 0;
	pi->hasinputminmax = false;
	pi->inputmin = 0; 
	pi->inputmax = 1;
	pi->hasminmax = true;
	pi->outputmin = 0;
	pi->outputmax = 1;
	pi->exp = 0;
	pi->units = "";		// no units defined
	// initialize parameter 1 ("m_fb_1")
	pi = self->__commonstate.params + 1;
	pi->name = "fb1";
	pi->paramtype = GENLIB_PARAMTYPE_FLOAT;
	pi->defaultvalue = self->m_fb_1;
	pi->defaultref = 0;
	pi->hasinputminmax = false;
	pi->inputmin = 0; 
	pi->inputmax = 1;
	pi->hasminmax = true;
	pi->outputmin = 0;
	pi->outputmax = 1;
	pi->exp = 0;
	pi->units = "";		// no units defined
	// initialize parameter 2 ("m_fb_3")
	pi = self->__commonstate.params + 2;
	pi->name = "fb2";
	pi->paramtype = GENLIB_PARAMTYPE_FLOAT;
	pi->defaultvalue = self->m_fb_3;
	pi->defaultref = 0;
	pi->hasinputminmax = false;
	pi->inputmin = 0; 
	pi->inputmax = 1;
	pi->hasminmax = true;
	pi->outputmin = 0;
	pi->outputmax = 1;
	pi->exp = 0;
	pi->units = "";		// no units defined
	// initialize parameter 3 ("m_spread_2")
	pi = self->__commonstate.params + 3;
	pi->name = "spread";
	pi->paramtype = GENLIB_PARAMTYPE_FLOAT;
	pi->defaultvalue = self->m_spread_2;
	pi->defaultref = 0;
	pi->hasinputminmax = false;
	pi->inputmin = 0; 
	pi->inputmax = 1;
	pi->hasminmax = true;
	pi->outputmin = 0;
	pi->outputmax = 400;
	pi->exp = 0;
	pi->units = "";		// no units defined
	
	return self;
}

/// Release all resources and memory used by a State object:

void destroy(CommonState *cself) { 
	State * self = (State *)cself;
	genlib_sysmem_freeptr(cself->params);
		
	delete self; 
}


} // C74_GENPLUGIN::
