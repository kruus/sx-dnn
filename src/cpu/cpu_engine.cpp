/*******************************************************************************
* Copyright 2016-2020 Intel Corporation
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

#include <assert.h>

#include "common/memory.hpp"
#include "common/type_helpers.hpp"

#include "cpu/cpu_engine.hpp"
#include "cpu/cpu_memory_storage.hpp"
#include "cpu/cpu_stream.hpp"

#if defined(__ve)
#include <fenv.h>
#include <stdio.h>
//? #pragma STDC FENV_ACCESS ON
#endif

namespace dnnl {
namespace impl {
namespace cpu {

cpu_engine_t::cpu_engine_t()
    : engine_t(engine_kind::cpu, get_default_runtime(engine_kind::cpu)) {
#if defined(__ve)
        if(0){
            int const originalRounding = fesetround(FE_DOWNWARD);
            int const tonearest = fegetround();
            printf(" cpu engine rounding mode %d --> %d (nearest)\n",
                   originalRounding, tonearest);
        }
        if(1){
            int const rounding_mode = fegetround();
            printf(" cpu engine rounding mode %d [ FE_DOWNWARD, FE_TONEAREST, FE_TOWARDZERO, FE_UPWARD ] = [%x, %x, %x, %x]\n",
                   rounding_mode, FE_DOWNWARD, FE_TONEAREST, FE_TOWARDZERO, FE_UPWARD);
        }
#endif
    }

status_t cpu_engine_t::create_memory_storage(
        memory_storage_t **storage, unsigned flags, size_t size, void *handle) {
    auto _storage = new cpu_memory_storage_t(this);
    if (_storage == nullptr) return status::out_of_memory;
    status_t status = _storage->init(flags, size, handle);
    if (status != status::success) {
        delete _storage;
        return status;
    }
    *storage = _storage;
    return status::success;
}

status_t cpu_engine_t::create_stream(
        stream_t **stream, unsigned flags, const stream_attr_t *attr) {
    return safe_ptr_assign<stream_t>(
            *stream, new cpu_stream_t(this, flags, attr));
}

} // namespace cpu
} // namespace impl
} // namespace dnnl

// vim: et ts=4 sw=4 cindent cino+=l0,\:4,N-s
