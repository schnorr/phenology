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
#include "palette.h"

palette_t *read_palette_from_file (const char *filename)
{
  palette_t *palette;
  palette = (palette_t*)malloc(sizeof(palette_t));
  palette->size = 0;
  palette->colors = NULL;

  FILE * fp;
  char * line = NULL;
  size_t len = 0;
  ssize_t read;

  fp = fopen(filename, "r");
  if (fp == NULL){
    return NULL;
  }

  while ((read = getline(&line, &len, fp)) != -1) {
    if (line[0] == '?' || line[0] == '\n') continue; //ignore comments, and empty lines
    line[strlen(line)-1] = '\0'; //remove \n
    if (read != 8){
      printf("Error: Line has less than 8 characters.\n");
      return NULL;
    }
    //line seems valid
    //allocate space for the RGB of the color
    palette->colors = (unsigned char*)realloc(palette->colors, (palette->size+1)*3);

    char aux[3];
    bzero(aux, 3);
    char *p = line+1;
    int i, j;
    for (j=0;j<3;j++){
      for (i = 0; i < 2; i++){
        aux[i] = *p++;
      }
      int base = (palette->size) * 3 + j;
      char *endptr;
      long int x = strtol(aux, &endptr, 16);
      palette->colors[base] = (unsigned char)x;
    }
    palette->size++;
  }

  fclose(fp);
  if (line)
    free(line);
  return palette;
}

/* int main(int argc, char **argv) */
/* { */
/*    palette_t *palette = read_palette_from_file (argv[1]); */
/*    int i; */
/*    for (i = 0; i < palette->size*3; i+=3){ */
/*      printf("%u %u %u\n", palette->colors[i+0], palette->colors[i+1], palette->colors[i+2]); */
/*    } */
/*    free(palette->colors); */
/*    free(palette); */
/* } */


tiff_palette_t *read_tiff_palette_from_file (const char *filename)
{
  palette_t *palette = read_palette_from_file (filename);

  tiff_palette_t *tpalette = NULL;
  tpalette = (tiff_palette_t*)malloc(sizeof(tiff_palette_t));
  tpalette->size = palette->size;
  tpalette->colors = (uint32*)malloc(sizeof(uint32)*tpalette->size);

  for (int i = 0; i < palette->size; i++){
    unsigned char r, g, b;
    r = palette->colors[i*3+0];
    g = palette->colors[i*3+1];
    b = palette->colors[i*3+2];

    uint32 color = 0;
    color |= r;
    color |= g << 8;
    color |= b << 16;
    color |= 255 << 24;
    tpalette->colors[i] = color;
  }
  free(palette->colors);
  free(palette);
  return tpalette;
}

uint32 get_tiff_color_from_palette (int selected, tiff_palette_t *palette)
{
  if (!palette) return tiff_black();
  if (selected > palette->size){
    return tiff_black();
  }
  return palette->colors[selected];
}

uint32 tiff_gray (void)
{
  uint32 gray = 0;
  gray |= 200;
  gray |= 200 << 8;
  gray |= 200 << 16;
  gray |= 255 << 24;
  return gray;
}

uint32 tiff_black (void)
{
  uint32 black = 0;
  black |= 255 << 24;
  return black;
}
