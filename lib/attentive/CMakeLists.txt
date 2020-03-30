# Copyright (c) 2018 Nordic Semiconductor
#
# SPDX-License-Identifier: Apache-2.0
#

# Attentive library sources 
zephyr_sources(cellular.c)
zephyr_sources(parser.c)
zephyr_sources(at-zephyr.c)
zephyr_sources(modem/common.c)
zephyr_sources(modem/generic.c)
zephyr_sources(modem/sim800.c)
	
# Attentive library includes
zephyr_include_directories(include)