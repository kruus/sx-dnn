/*******************************************************************************
* Copyright 2016-2017 Intel Corporation
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*******************************************************************************/

#ifndef JIT_AVX512_COMMON_CONV_KERNEL_F32_HPP
#define JIT_AVX512_COMMON_CONV_KERNEL_F32_HPP

#include "c_types_map.hpp"
#include "jit_generator.hpp"
#include "jit_primitive_conf.hpp"

namespace mkldnn {
namespace impl {
namespace cpu {

struct jit_avx512_common_conv_fwd_kernel : public jit_generator {

    jit_avx512_common_conv_fwd_kernel(jit_conv_conf_t ajcp) : jcp(ajcp)
    {
        generate();
        jit_ker = (void (*)(jit_conv_call_s *))getCode();
    }

    static status_t init_conf(jit_conv_conf_t &jcp,
            const convolution_desc_t &cd,
            const memory_desc_wrapper &src_d,
            const memory_desc_wrapper &weights_d,
            const memory_desc_wrapper &dst_d,
            bool with_relu = false,
            double relu_negative_slope = 0.);

    jit_conv_conf_t jcp;
    void (*jit_ker)(jit_conv_call_s *);

private:
    using reg64_t = const Xbyak::Reg64;
    enum {
        typesize = sizeof(float),
        ker_reg_base_idx = 28,
    };

    reg64_t param = abi_param1;
    reg64_t reg_inp = r8;
    reg64_t reg_ker = r9;
    reg64_t reg_out = r10;

    reg64_t reg_inp_prf = r11;
    reg64_t reg_ker_prf = r12;
    reg64_t reg_out_prf = r13;

    reg64_t aux_reg_inp = r14;
    reg64_t aux_reg_ker = r15;

    reg64_t aux_reg_inp_prf = rsi;
    reg64_t aux_reg_ker_prf = rdx;

    reg64_t reg_channel = rsi;
    reg64_t reg_bias = rdx;

    reg64_t reg_kj = rax;
    reg64_t reg_relu_ns = rax;
    reg64_t reg_oi = rbx;
    reg64_t reg_kh = abi_not_param1;

    reg64_t reg_tmp = rbp;

    reg64_t reg_ic_loop = rdx;
    reg64_t reg_inp_loop = rsi;

    reg64_t reg_init_flag = r13;
    reg64_t reg_bias_ptr = param;

    reg64_t aux_reg_ic = r12;
    reg64_t reg_binp = rax;
    reg64_t reg_bout = r11;
    reg64_t aux1_reg_inp = rbx;
    reg64_t aux_reg_out = abi_not_param1;

    inline Xbyak::Zmm zmm_ker(int i_ic) {
        assert(i_ic < 4);
        return Xbyak::Zmm(ker_reg_base_idx + i_ic);
    }

    inline Xbyak::Zmm zmm_out(int i_ur, int i_oc) {
        int idx = i_ur + i_oc * jcp.ur_w;
        assert(idx < ker_reg_base_idx);
        return Xbyak::Zmm(idx);
    }

    Xbyak::Zmm zmm_relu_ns = Xbyak::Zmm(30);
    Xbyak::Zmm zmm_zero = Xbyak::Zmm(31);

    inline void prepare_output(int ur_w);
    inline void store_output(int ur_w);
    inline void compute_loop_fma(int ur_w, int pad_l, int pad_r);
    inline void compute_loop_4vnni(int ur_w, int pad_l, int pad_r);
    inline void compute_loop_4fma(int ur_w, int pad_l, int pad_r);
    inline void compute_loop(int ur_w, int pad_l, int pad_r);

    void generate();

    inline void vadd(Xbyak::Zmm zmm, reg64_t reg, int offset)   {
        if (jcp.ver == ver_4vnni)
            vpaddd(zmm, zmm, EVEX_compress_addr(reg, offset));
        else
            vaddps(zmm, zmm, EVEX_compress_addr(reg, offset));
    }

    inline void vcmp(Xbyak::Opmask kmask,
        Xbyak::Zmm zmm_src1, Xbyak::Zmm zmm_src2, const unsigned char cmp) {
        if (jcp.ver == ver_4vnni)
            vpcmpd(kmask, zmm_src1, zmm_src2, cmp);
        else
            vcmpps(kmask, zmm_src1, zmm_src2, cmp);
    }

    inline void vmul(Xbyak::Zmm zmm_dst, Xbyak::Opmask kmask,
                     Xbyak::Zmm zmm_src1, Xbyak::Zmm zmm_src2) {
        if (jcp.ver == ver_4vnni)
            vpmulld(zmm_dst | kmask, zmm_src1, zmm_src2);
        else
            vmulps(zmm_dst | kmask, zmm_src1, zmm_src2);
    }

    inline int get_output_offset(int oi, int n_oc_block) {
        return jcp.typesize_out
            * (n_oc_block * jcp.oh * jcp.ow + oi) * jcp.oc_block;
    }

    inline int get_input_offset(int ki, int ic, int oi, int pad_l) {
        int scale = (jcp.ver == ver_4vnni) ? 2 : 1;
        int iw_str = !jcp.is_1stconv ? jcp.ic_block : 1;
        int ic_str = !jcp.is_1stconv ? 1 : jcp.iw * jcp.ih;
        return jcp.typesize_in
            * ((ki + oi * jcp.stride_w - pad_l) * iw_str + scale * ic * ic_str);
    }

    inline int get_kernel_offset(int ki,int ic,int n_oc_block,int ker_number) {
        int scale = (jcp.ver == ver_4vnni) ? 2 : 1;
        return jcp.typesize_in * jcp.oc_block
            * (n_oc_block * jcp.nb_ic * jcp.ic_block * jcp.kh * jcp.kw
                    + (ic + ker_number) * scale + ki * jcp.ic_block);
    }

    inline int get_ow_start(int ki, int pad_l) {
        return nstl::max(0, (pad_l - ki + jcp.stride_w - 1) / jcp.stride_w);
    }

    inline int get_ow_end(int ki, int pad_r) {
        return jcp.ur_w - nstl::max(0,
            (ki + pad_r - (jcp.kw - 1) + jcp.stride_w - 1) / jcp.stride_w);
    }
};

struct jit_avx512_common_conv_bwd_data_kernel_f32: public jit_generator {

    jit_avx512_common_conv_bwd_data_kernel_f32(jit_conv_conf_t ajcp): jcp(ajcp)
    {
        generate();
        jit_ker = (void (*)(jit_conv_call_s *))getCode();
    }

    static status_t init_conf(jit_conv_conf_t &jcp,
            const convolution_desc_t &cd,
            const memory_desc_wrapper &diff_src_d,
            const memory_desc_wrapper &weights_d,
            const memory_desc_wrapper &diff_dst_d);

    jit_conv_conf_t jcp;
    void (*jit_ker)(jit_conv_call_s *);

private:
    using reg64_t = const Xbyak::Reg64;
    enum {
        typesize = sizeof(float),
        ker_reg_base_idx = 28,
    };

    reg64_t param = abi_param1;
    reg64_t reg_dst = r8;
    reg64_t reg_ker = r9;
    reg64_t reg_src = r10;

    reg64_t reg_dst_prf = r11;
    reg64_t reg_ker_prf = r12;
    reg64_t reg_src_prf = r13;

    reg64_t aux_reg_dst = r14;
    reg64_t aux_reg_ker = r15;

    reg64_t aux_reg_dst_prf = rsi;
    reg64_t aux_reg_ker_prf = rdx;

    reg64_t reg_kj = rax;
    reg64_t reg_oi = rbx;
    reg64_t reg_kh = abi_not_param1;

    reg64_t reg_channel = rsi;

    reg64_t reg_tmp = rbp;

    inline Xbyak::Zmm zmm_ker(int i_ic) {
        assert(i_ic < 4);
        return Xbyak::Zmm(ker_reg_base_idx + i_ic);
    }

    inline Xbyak::Zmm zmm_out(int i_ur, int i_oc) {
        int idx = i_ur + i_oc * jcp.ur_w;
        assert(idx < ker_reg_base_idx);
        return Xbyak::Zmm(idx);
    }
    inline void vadd(Xbyak::Zmm zmm, reg64_t reg, int offset) {
        if (jcp.ver == ver_4vnni)
            vpaddd(zmm, zmm, EVEX_compress_addr(reg, offset));
        else
            vaddps(zmm, zmm, EVEX_compress_addr(reg, offset));
    }

    inline void prepare_output(int ur_w);
    inline void store_output(int ur_w);
    inline void compute_loop_4fma(int ur_w, int l_overflow, int r_overflow);
    inline void compute_loop_4vnni(int ur_w, int l_overflow, int r_overflow);
    inline void compute_loop_fma(int ur_w, int l_overflow, int r_overflow);
    inline void compute_loop(int ur_w, int l_overflow, int r_overflow);
    void generate();
    inline int get_iw_start(int ki, int l_overflow) {
        return nstl::max(0, l_overflow - (jcp.kw - 1) + ki);
    }
    inline int get_iw_end(int ki, int r_overflow) {
        return jcp.ur_w - nstl::max(0, r_overflow - ki);
    }
};

struct jit_avx512_common_conv_bwd_weights_kernel_f32 : public jit_generator {

    jit_avx512_common_conv_bwd_weights_kernel_f32(jit_conv_conf_t ajcp)
        : jcp(ajcp)
    {
        generate();
        jit_ker = (void (*)(jit_conv_call_s *))getCode();
    }

    static status_t init_conf(jit_conv_conf_t &jcp,
        const convolution_desc_t &cd,
        const memory_desc_wrapper &src_d,
        const memory_desc_wrapper &diff_weights_d,
        const memory_desc_wrapper &diff_dst_d);

    jit_conv_conf_t jcp;
    void (*jit_ker)(jit_conv_call_s *);

private:
    using reg64_t = const Xbyak::Reg64;
    enum {typesize = sizeof(float)};
    static const int max_ur_w;

    reg64_t param = abi_param1;
    reg64_t reg_input = rax;
    reg64_t reg_kernel = rdx;
    reg64_t reg_output = rsi;
    reg64_t b_ic = abi_not_param1;
    reg64_t kj = r8;
    reg64_t reg_kh  = r9;
    reg64_t reg_ur_w_trips  = r10;
    reg64_t reg_oj = r15;
    reg64_t reg_ih_count = rbx;

    inline void compute_oh_step_unroll_ow_icblock(int ic_block_step,
            int max_ur_w);
    inline void oh_step_comeback_pointers();
    inline void compute_oh_step_unroll_ow(int ic_block_step, int max_ur_w);
    inline void compute_ic_block_step(int ur_w,
            int pad_l, int pad_r, int ic_block_step,
            int input_offset, int kernel_offset, int output_offset,
            bool input_wraparound = false);
    inline void compute_ic_block_step_fma(int ur_w,
            int pad_l, int pad_r, int ic_block_step,
            int input_offset, int kernel_offset, int output_offset,
            bool input_wraparound);
    inline void compute_ic_block_step_4fma(int ur_w,
            int pad_l, int pad_r, int ic_block_step,
            int input_offset, int kernel_offset, int output_offset,
            bool input_wraparound);
    inline void compute_oh_step_common(int ic_block_step, int max_ur_w);
    inline void compute_oh_step_disp();
    inline void compute_oh_loop_common();

    void generate();
};


}
}
}

#endif
