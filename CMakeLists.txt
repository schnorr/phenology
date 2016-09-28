# This file is part of Phenology
#
# Phenology is free software: you can redistribute it and/or modify it
# under the terms of the GNU Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# Phenology is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Public License for more details.
#
# You should have received a copy of the GNU Public License
# along with Phenology. If not, see <http://www.gnu.org/licenses/>.
cmake_minimum_required (VERSION 2.8)
project(phenology)
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
find_package(OpenMP)
find_package(JPEG)
find_package(TIFF)
find_package(GeoTIFF)
message(${GEOTIFF_INCLUDE_DIR})

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c99")
if(OPENMP_FOUND)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
endif()
if(TIFF_FOUND)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -I${TIFF_INCLUDE_DIR}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I${TIFF_INCLUDE_DIR}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${TIFF_LIBRARIES}")
endif()
if(GEOTIFF_FOUND)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -I${GEOTIFF_INCLUDE_DIR}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I${GEOTIFF_INCLUDE_DIR}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${GEOTIFF_LIBRARIES}")
endif()
add_executable(pga_hist ../src/get_green_average.c ../src/metrics.c ../src/args.c)
target_link_libraries(pga_hist jpeg m)
#target_link_libraries(pga_hist argp)
install(PROGRAMS ${CMAKE_BINARY_DIR}/pga_hist DESTINATION bin)

add_executable(pga_palette ../src/avg_green_segment.c ../src/palette.c ../src/metrics.c)
include_directories(../src)
target_link_libraries(pga_palette jpeg m)
install(PROGRAMS ${CMAKE_BINARY_DIR}/pga_palette DESTINATION bin)

add_executable(pga_tiff_palette ../src/tiff_palette.c ../src/metrics.c ../src/palette.c)
target_link_libraries(pga_tiff_palette tiff geotiff m)
install(PROGRAMS ${CMAKE_BINARY_DIR}/pga_tiff_palette DESTINATION bin)

add_executable(pga_tiff_hist ../src/tiff_hist.c ../src/metrics.c ../src/palette.c)
target_link_libraries(pga_tiff_hist tiff geotiff m)
install(PROGRAMS ${CMAKE_BINARY_DIR}/pga_tiff_hist DESTINATION bin)



install(PROGRAMS ${CMAKE_HOME_DIRECTORY}/scripts/pga_csv DESTINATION bin)
install(PROGRAMS ${CMAKE_HOME_DIRECTORY}/scripts/pga_cpm DESTINATION bin)
install(PROGRAMS ${CMAKE_HOME_DIRECTORY}/scripts/pga_palette DESTINATION bin)
