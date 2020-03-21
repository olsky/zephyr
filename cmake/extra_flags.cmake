# SPDX-License-Identifier: Apache-2.0

separate_arguments(EXTRA_CPPFLAGS_AS_LIST UNIX_COMMAND ${EXTRA_CPPFLAGS})
separate_arguments(EXTRA_LDFLAGS_AS_LIST UNIX_COMMAND  ${EXTRA_LDFLAGS})
separate_arguments(EXTRA_CFLAGS_AS_LIST   UNIX_COMMAND ${EXTRA_CFLAGS})
separate_arguments(EXTRA_CXXFLAGS_AS_LIST UNIX_COMMAND ${EXTRA_CXXFLAGS})
separate_arguments(EXTRA_AFLAGS_AS_LIST   UNIX_COMMAND ${EXTRA_AFLAGS})

if(EXTRA_CPPFLAGS)
  zephyr_compile_options(${EXTRA_CPPFLAGS_AS_LIST})
endif()
if(EXTRA_LDFLAGS)
  zephyr_link_libraries(${EXTRA_LDFLAGS_AS_LIST})
endif()
if(EXTRA_CFLAGS)
  foreach(F ${EXTRA_CFLAGS_AS_LIST})
    zephyr_compile_options($<$<COMPILE_LANGUAGE:C>:${F}>)
  endforeach()
endif()
if(EXTRA_CXXFLAGS)
  foreach(F ${EXTRA_CXXFLAGS_AS_LIST})
    zephyr_compile_options($<$<COMPILE_LANGUAGE:CXX>:${F}>)
  endforeach()
endif()
if(EXTRA_AFLAGS)
  foreach(F ${EXTRA_AFLAGS_AS_LIST})
    zephyr_compile_options($<$<COMPILE_LANGUAGE:ASM>:${F}>)
  endforeach()
endif()
