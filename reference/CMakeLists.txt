cmake_minimum_required(VERSION 3.16.0)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(synth-idf)

# otherwise segmentation compilation idk man
set_source_files_properties(${CMAKE_SOURCE_DIR}/components/esp_lcd/rgb/esp_lcd_panel_rgb.c PROPERTIES HEADER_FILE_ONLY TRUE)
