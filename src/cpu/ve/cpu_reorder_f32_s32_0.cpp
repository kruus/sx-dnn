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
const std::vector<rpd_create_f> f32_s32_0 = {
        REG_FAST_DIRECT_COPY_COMMA(f32, s32)

        DNNL_X64_ONLY(x64::jit_uni_reorder_create,)

        REG_SR_BIDIR(f32, any, s32, nChw16c),

        REG_SR(f32, any, s32, any, fmt_order::any, spec::reference),

        nullptr,
    };

// try explicit instantiation to see diagnostics?
template class simple_reorder_t<f32,any, s32,any, fmt_order::any, spec::direct_copy>;
//template simple_reorder_t<f32,any, s32,any, fmt_order::any, spec::direct_copy_except_dim_0>;
template class simple_reorder_t<f32,any, s32,any, fmt_order::any, spec::reference>;

} // namespace reorder
} // namespace cpu
} // namespace impl
} // namespace dnnl

