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
#include <strings.h>
#include <stdlib.h>
#include <tiffio.h>
#include <geotiff/xtiffio.h>
#include <geotiff/geotiff.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <math.h>

#include "metrics.h"
#include "palette.h"

uint32 *read_pixels (TIFF *tif, size_t *len)
{
  if (!tif) return NULL;
  uint32 w, h;
  TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &w);
  TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &h);
  *len = w * h;
  uint32* raster = (uint32*) _TIFFmalloc(*len * sizeof (uint32));
  if (raster != NULL) {
    if (!TIFFReadRGBAImage(tif, w, h, raster, 0)) {
      _TIFFfree(raster);
      return NULL;
    }
  }else{
    return NULL;
  }
  return raster;
}

float get_bin (uint32 pixel, int grain)
{
  unsigned char r, g, b, a;
  r = TIFFGetR(pixel);
  g = TIFFGetG(pixel);
  b = TIFFGetB(pixel);
  //calculate GA, truncf
  float value = get_green_average (r, g, b) * grain;
  return value;
}

int *calc_hist_pga (uint32 *raster, size_t len, int grain)
{
  if (!raster) return NULL;

  int *ret = (int*)malloc(grain * sizeof(int));
  bzero(ret, grain*sizeof(int));

#pragma omp parallel shared(raster, ret) firstprivate(grain)
  {
    int *hist_private;
    int size = grain * sizeof(int);
    hist_private = (int*) malloc (size);
    bzero(hist_private, size);

#pragma omp for nowait schedule(dynamic, 32*1024)
    for (int i = 0; i < len; i++){
      if (TIFFGetA(raster[i]) == 0) continue;
      int x = get_bin (raster[i], grain);
      if (x >= 0) hist_private[x]++;
    }

    for (int i = 0; i < grain; i++){
#pragma omp atomic
      ret[i] += hist_private[i];
    }
  }
  return ret;
}

int main (int argc, char **argv)
{
  if (argc != 3){
    printf("Usage: %s <IMAGE.tif> <GRAIN>\n", argv[0]);
    exit(1);
  }
  
  const char *source = argv[1];
  int grain = atoi(argv[2]);
  
  TIFF *tif = XTIFFOpen (source, "r");
  if (tif){
    char emsg[1024]; 
    if (!TIFFRGBAImageOK (tif, emsg)){
      printf("image not okay to be processed as a rgba image\n");
      return 1;
    }

    //read pixels from TIFF file
    size_t len;
    uint32 *raster = read_pixels(tif, &len);
    if (!raster){
      printf("Read pixels failed.\n");
      return 1;
    }
    //calculate GA, get the histogram back
    int *histogram = calc_hist_pga (raster, len, grain);
    int i;
    for (i = 0; i < grain; i++){
      printf ("%d", histogram[i]);
      if((i+1) == grain){
        printf("\n");
      }else{
        printf(",");
      }
    }
    free(histogram);
    _TIFFfree (raster);
  }
  XTIFFClose(tif);
  return 0;
}
