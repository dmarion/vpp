# SPDX-License-Identifier: Apache-2.0
# Copyright(c) 2021 Cisco Systems, Inc.

if(NOT CMAKE_SYSTEM_PROCESSOR MATCHES "amd64.*|x86_64.*|AMD64.*")
  return()
endif()

add_vpp_plugin(dsa
  SOURCES
  dsa.c
)
