/*******************************************************************************
* Copyright 2017-2018 Intel Corporation
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

#if MKLDNN_JITFUNCS <= 0
INST_TEST_CASE(Cifar10_nchw,
    PARAMS(nchw, oihw, FMT_BIAS, nchw,
        2, 1, 3, 32, 32, 32, 32, 32, 5, 5, 2, 2, 1, 1)
);
#else
INST_TEST_CASE(Cifar10_Blocked,
    PARAMS(nchw, Ohwi8o, FMT_BIAS, FMT_DATA_BLOCKED,
        2, 1, 3, 32, 32, 32, 32, 32, 5, 5, 2, 2, 1, 1)
);

INST_TEST_CASE(Cifar10_Blocked16,
    PARAMS(nchw, Ohwi8o, FMT_BIAS, FMT_DATA_BLOCKED16,
        2, 1, 3, 32, 32, 32, 32, 32, 5, 5, 2, 2, 1, 1)
);
#endif