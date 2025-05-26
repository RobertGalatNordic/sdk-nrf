#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(nrf_rpc_serialize_client_test)

FILE(GLOB app_sources src/*.c)

target_include_directories(app PRIVATE
  ${ZEPHYR_NRF_MODULE_DIR}/subsys/nrf_rpc/common
  ${ZEPHYR_NRF_MODULE_DIR}/subsys/nrf_rpc/client
  ${ZEPHYR_NRF_MODULE_DIR}/subsys/nrf_rpc/include
  ${ZEPHYR_NRF_MODULE_DIR}/include
  ${ZEPHYR_NRF_MODULE_DIR}/include/nrf_rpc
  ${ZEPHYR_NRF_MODULE_DIR}/nrfxlib/nrf_rpc/include
  ${ZEPHYR_NRF_MODULE_DIR}/tests/include
  ../common
)

target_sources(app PRIVATE
  ${app_sources}
  ../common/nrf_rpc_single_thread.c
)

# Enforce single-threaded nRF RPC command processing.
target_link_options(app PUBLIC
  -Wl,--wrap=nrf_rpc_os_init,--wrap=nrf_rpc_os_thread_pool_send
)

zephyr_library()
zephyr_library_sources(
  src/serialize_suite.c
)

zephyr_library_include_directories(
  ${ZEPHYR_NRF_MODULE_DIR}/subsys/nrf_rpc/common
  ${ZEPHYR_NRF_MODULE_DIR}/subsys/nrf_rpc/client
  ${ZEPHYR_NRF_MODULE_DIR}/subsys/nrf_rpc/include
  ${ZEPHYR_NRF_MODULE_DIR}/include/nrf_rpc
  ${ZEPHYR_NRF_MODULE_DIR}/nrfxlib/nrf_rpc/include
  ${ZEPHYR_NRF_MODULE_DIR}/tests/include
  ../common
)

# Use Zephyr module dependencies instead of direct library linking
zephyr_library_link_libraries(nrf_rpc)
