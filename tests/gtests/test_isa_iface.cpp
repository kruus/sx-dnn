/*******************************************************************************
* Copyright 2019-2020 Intel Corporation
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

#include "gtest/gtest.h"

#include "dnnl.hpp"
#if defined(__ve)
// reinstate old version of  test_isa_iface for VE!
#else
#include "src/cpu/x64/cpu_isa_traits.hpp"
#endif

namespace dnnl {

class isa_set_once_test : public ::testing::Test {};
TEST(isa_set_once_test, TestISASetOnce) {
#if defined(__ve)
    // set max-cpu to "vanilla" (add bit to ve/cpu_isa_traits.hpp)
    printf(" TODO: reinstate VE cpu_isa_traits and test_ias_iface test\n");
#else
    auto st = set_max_cpu_isa(cpu_isa::sse41);
    ASSERT_TRUE(st == status::success || st == status::unimplemented);
    ASSERT_TRUE(impl::cpu::x64::mayiuse(impl::cpu::x64::sse41));
    st = set_max_cpu_isa(cpu_isa::sse41);
    ASSERT_TRUE(st == status::invalid_arguments || st == status::unimplemented);
#endif
};

} // namespace dnnl
