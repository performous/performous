# FindGLM - attempts to locate the glm matrix/vector library.
#
# This module defines the following variables (on success):
#   GLM_INCLUDE_DIRS  - where to find glm/glm.hpp
#   GLM_FOUND         - if the library was successfully located
#
# It is trying a few standard installation locations, but can be customized
# with the following variables:
#   GLM_ROOT_DIR      - root directory of a glm installation
#                       Headers are expected to be found in either:
#                       <GLM_ROOT_DIR>/glm/glm.hpp           OR
#                       <GLM_ROOT_DIR>/include/glm/glm.hpp
#                       This variable can either be a cmake or environment
#                       variable. Note however that changing the value
#                       of the environment varible will NOT result in
#                       re-running the header search and therefore NOT
#                       adjust the variables set by this module.

#=============================================================================
# Copyright 2012 Carsten Neumann
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# * Redistributions of source code must retain the above copyright
#   notice, this list of conditions and the following disclaimer.
#
# * Redistributions in binary form must reproduce the above copyright
#   notice, this list of conditions and the following disclaimer in the
#   documentation and/or other materials provided with the distribution.
#
# * Neither the name of Kitware, Inc. nor the names of Contributors
#   may be used to endorse or promote products derived from this
#   software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#=============================================================================

# default search dirs
set(_glm_HEADER_SEARCH_DIRS
    "/usr/include"
    "/usr/local/include")

# check environment variable
set(_glm_ENV_ROOT_DIR "$ENV{GLM_ROOT_DIR}")

IF(NOT GLM_ROOT_DIR AND _glm_ENV_ROOT_DIR)
    set(GLM_ROOT_DIR "${_glm_ENV_ROOT_DIR}")
ENDIF(NOT GLM_ROOT_DIR AND _glm_ENV_ROOT_DIR)

# put user specified location at beginning of search
IF(GLM_ROOT_DIR)
    set(_glm_HEADER_SEARCH_DIRS "${GLM_ROOT_DIR}"
                                "${GLM_ROOT_DIR}/include"
                                 ${_glm_HEADER_SEARCH_DIRS})
ENDIF(GLM_ROOT_DIR)

# locate header
FIND_PATH(GLM_INCLUDE_DIR "glm/glm.hpp"
    PATHS ${_glm_HEADER_SEARCH_DIRS})

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(GLM DEFAULT_MSG
    GLM_INCLUDE_DIR)

IF(GLM_FOUND)
    set(GLM_INCLUDE_DIRS "${GLM_INCLUDE_DIR}")

    MESSAGE(STATUS "GLM_INCLUDE_DIR = ${GLM_INCLUDE_DIR}")
ENDIF(GLM_FOUND)
