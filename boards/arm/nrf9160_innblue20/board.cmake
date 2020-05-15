# 2019 - innblue - nrf9160 board v2.0
#
board_runner_args(nrfjprog "--nrf-family=NRF91")
board_runner_args(jlink "--device=cortex-m33" "--speed=4000")
include(${ZEPHYR_BASE}/boards/common/nrfjprog.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)