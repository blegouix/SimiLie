# SPDX-License-Identifier: GPL-3.0

add_library("mesher" INTERFACE)

target_include_directories("mesher" INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")

target_link_libraries("mesher" INTERFACE DDC::DDC)

add_library("sil::mesher" ALIAS "mesher")
