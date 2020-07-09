/*******************************************************************************
* Copyright 2017-2020 Intel Corporation
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

#include "cpu/ve/cpu_reorder_split.hpp"

namespace dnnl {
namespace impl {
namespace cpu {
namespace reorder {

// clang-format off

// f32 -> bf16
const std::vector<rpd_create_f> s32_undef_0 = {
        REG_FAST_DIRECT_COPY_COMMA(s32, f32)
        REG_FAST_DIRECT_COPY_COMMA(s32, s32)
        REG_FAST_DIRECT_COPY_COMMA(s32, s8)
        REG_FAST_DIRECT_COPY_COMMA(s32, u8)

        DNNL_X64_ONLY(x64::jit_uni_reorder_create,)

        REG_SR_BIDIR(s32, any, f32, nChw16c),
        REG_SR_BIDIR(s32, any, s32, nChw16c),
        REG_SR_BIDIR(s32, any, s8, nChw16c),
        REG_SR_BIDIR(s32, any, u8, nChw16c),

        REG_SR(s32, any, f32, any, fmt_order::any, spec::reference),
        REG_SR(s32, any, s32, any, fmt_order::any, spec::reference),
        REG_SR(s32, any, s8, any, fmt_order::any, spec::reference),
        REG_SR(s32, any, u8, any, fmt_order::any, spec::reference),

        nullptr,
    };

} // namespace reorder
} // namespace cpu
} // namespace impl
} // namespace dnnl

