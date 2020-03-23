cmake_minimum_required(VERSION 3.13.1)

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(mesh_gateway)

target_sources(app PRIVATE
	src/main.c
	src/mqtt_publisher.c
	src/uart_mqtt_bridge.c
	src/board_lights.c)
zephyr_include_directories(src)
