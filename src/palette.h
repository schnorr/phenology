/*
    This file is part of Phenology

    Phenology is free software: you can redistribute it and/or modify
    it under the terms of the GNU Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Phenology is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Public License for more details.

    You should have received a copy of the GNU Public License
    along with Phenology. If not, see <http://www.gnu.org/licenses/>.
*/
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tiffio.h>

typedef struct palette {
  int size;
  unsigned char *colors;
} palette_t;

typedef struct tiff_palette {
  int size;
  uint32 *colors;
} tiff_palette_t;

palette_t *read_palette_from_file (const char *filename);
tiff_palette_t *read_tiff_palette_from_file (const char *filename);
uint32 get_tiff_color_from_palette (int selected, tiff_palette_t *palette);
uint32 tiff_gray (void);
uint32 tiff_black (void);
