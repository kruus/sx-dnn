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
const std::vector<rpd_create_f> s8_s8_6 = {
        REG_SR(s8, any, s8, dhwigo, fmt_order::keep, spec::conv_s8s8),
        REG_SR(s8, goidhw, s8, gOIdhw4i16o4i, fmt_order::keep, spec::conv_s8s8),
        REG_SR(s8, goidhw, s8, gOIdhw2i8o4i, fmt_order::keep, spec::conv_s8s8),
        REG_SR(s8, goidhw, s8, gOIdhw4o4i, fmt_order::keep, spec::conv_s8s8),

        nullptr,
    };

} // namespace reorder
} // namespace cpu
} // namespace impl
} // namespace dnnl

