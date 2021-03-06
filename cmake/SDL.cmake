#===============================================================================
# Copyright 2017-2020 Intel Corporation
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#===============================================================================

# Manage secure Development Lifecycle-related compiler flags
#===============================================================================

if(SDL_cmake_included)
    return()
endif()
set(SDL_cmake_included true)
include("cmake/utils.cmake")

set(CMAKE_CCXX_FLAGS)
if(NECVE) # handle cross-compiler toolchains separately
    if(DNNL_LIBRARY_TYPE STREQUAL "SHARED")
        set(CMAKE_CCXX_FLAGS "-fPIC")
    endif()
    append(CMAKE_EXE_LINKER_FLAGS "-Wl,-z,noexecstack -Wl,-z,relro -Wl,-z,now")
    append(CMAKE_SHARED_LINKER_FLAGS "-Wl,-z,noexecstack -Wl,-z,relro -Wl,-z,now")

    # /opt/nec/ve/bin/nld: warning: -z stacksize=0x1000000 ignored.
    #append(CMAKE_SHARED_LINKER_FLAGS "-Wl,-z,stacksize=0x1000000") # 16M stack
    #append(CMAKE_EXE_LINKER_FLAGS    "-Wl,-z,stacksize=0x1000000") # 16M stack

    #append(CMAKE_CCXX_FLAGS "-D_VE_ITERATOR_DEBUG")
    append(CMAKE_CCXX_FLAGS "-D_FORTIFY_SOURCE=0")
    #append(CMAKE_CCXX_FLAGS "-D_FORTIFY_SOURCE=1")
    #append(CMAKE_CCXX_FLAGS "-D_FORTIFY_SOURCE=2") #--> printf in every object file
    #append(CMAKE_EXE_LINKER_FLAGS "-Wl,-z,muldefs") # if you choose -D_FORTIFY_SOURCE=2
    #append(CMAKE_SHARED_LINKER_FLAGS "-Wl,-z,muldefs")
elseif(UNIX)
    # DNNL_LIBRARY_TYPE STATIC might not need -fPIC
    set(CMAKE_CCXX_FLAGS "-fPIC -Wformat -Wformat-security")
    append(CMAKE_CXX_FLAGS_RELEASE "-D_FORTIFY_SOURCE=2")
    append(CMAKE_C_FLAGS_RELEASE "-D_FORTIFY_SOURCE=2")
    if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
        if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.9)
            append(CMAKE_CCXX_FLAGS "-fstack-protector-all")
        else()
            append(CMAKE_CCXX_FLAGS "-fstack-protector-strong")
        endif()

        # GCC might be very paranoid for partial structure initialization, e.g.
        #   struct { int a, b; } s = { 0, };
        # However the behavior is triggered by `Wmissing-field-initializers`
        # only. To prevent warnings on users' side who use the library and turn
        # this warning on, let's use it too. Applicable for the library sources
        # and interfaces only (tests currently rely on that fact heavily)
        append(CMAKE_SRC_CCXX_FLAGS "-Wmissing-field-initializers")
        append(CMAKE_EXAMPLE_CCXX_FLAGS "-Wmissing-field-initializers")
    elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
        append(CMAKE_CCXX_FLAGS "-fstack-protector-all")
    elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
        append(CMAKE_CCXX_FLAGS "-fstack-protector")
    endif()
    if(APPLE)
        append(CMAKE_SHARED_LINKER_FLAGS "-Wl,-bind_at_load")
        append(CMAKE_EXE_LINKER_FLAGS "-Wl,-bind_at_load")
    else()
        append(CMAKE_EXE_LINKER_FLAGS "-pie")
        append(CMAKE_SHARED_LINKER_FLAGS "-Wl,-z,noexecstack -Wl,-z,relro -Wl,-z,now")
        append(CMAKE_EXE_LINKER_FLAGS "-Wl,-z,noexecstack -Wl,-z,relro -Wl,-z,now")
    endif()
elseif(MSVC AND ${CMAKE_CXX_COMPILER_ID} STREQUAL MSVC)
    set(CMAKE_CCXX_FLAGS "/guard:cf")
endif()
if(CMAKE_CCXX_FLAGS)
    append(CMAKE_C_FLAGS "-D_SDL_beg ${CMAKE_CCXX_FLAGS} -D_SDL_end")
    append(CMAKE_CXX_FLAGS "-D_SDL_beg ${CMAKE_CCXX_FLAGS} -D_SDL_end")
endif()

