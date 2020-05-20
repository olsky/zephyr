@echo off

set ZEPHYR_BASE=%~dp0

if exist "%userprofile%\zephyrrc.cmd" (
	call "%userprofile%\zephyrrc.cmd"
)

set ZEPHYR_TOOLCHAIN_VARIANT=gnuarmemb
set GNUARMEMB_TOOLCHAIN_PATH=c:\Users\michaelweisberg\gcc-arm-none-eabi-9-2019-q4-major-win32
