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

#include "cpu/ve/simple_q10n_ve_f32_int.hpp" // assembler specialization

namespace dnnl {
namespace impl {
namespace cpu {
namespace reorder {

// clang-format off

// f32 -> bf16
const std::vector<rpd_create_f>& f32_s8_3() {
    static const std::vector<rpd_create_f> v = {
        REG_SR(f32, any, s8, wio, fmt_order::keep, spec::conv_s8s8),
        REG_SR(f32, oiw, s8, OIw4i16o4i, fmt_order::keep, spec::conv_s8s8),
        REG_SR(f32, wio, s8, OIw4i16o4i, fmt_order::keep, spec::conv_s8s8),
        REG_SR(f32, oiw, s8, OIw2i8o4i, fmt_order::keep, spec::conv_s8s8),
        REG_SR(f32, wio, s8, OIw2i8o4i, fmt_order::keep, spec::conv_s8s8),
        REG_SR(f32, oiw, s8, OIw4o4i, fmt_order::keep, spec::conv_s8s8),
        REG_SR(f32, wio, s8, OIw4o4i, fmt_order::keep, spec::conv_s8s8),

        nullptr,
    };
    return v;
}

} // namespace reorder
} // namespace cpu
} // namespace impl
} // namespace dnnl

