# Copyright (c) 2018 Nordic Semiconductor
#
# SPDX-License-Identifier: Apache-2.0
#

# NMEA Parser library sources 
zephyr_sources(gps/gps.c)
	
# Attentive library includes
zephyr_include_directories(include)