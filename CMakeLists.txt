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
add_executable(pga_hist ../src/get_green_average.c)
target_link_libraries(pga_hist jpeg m)
install(PROGRAMS ${CMAKE_BINARY_DIR}/pga_hist DESTINATION bin)

add_executable(pga_palette ../src/avg_green_segment.c ../src/read_palette.c)
include_directories(../src)
target_link_libraries(pga_palette jpeg m)
install(PROGRAMS ${CMAKE_BINARY_DIR}/pga_palette DESTINATION bin)

install(PROGRAMS ${CMAKE_HOME_DIRECTORY}/scripts/pga_csv DESTINATION bin)
install(PROGRAMS ${CMAKE_HOME_DIRECTORY}/scripts/pga_cpm DESTINATION bin)