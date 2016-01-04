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

int cp(const char *to, const char *from)
{
    int fd_to, fd_from;
    char buf[4096];
    ssize_t nread;
    int saved_errno;

    fd_from = open(from, O_RDONLY);
    if (fd_from < 0)
        return -1;

    fd_to = open(to, O_WRONLY | O_CREAT, 0666);
    if (fd_to < 0)
        goto out_error;

    while (nread = read(fd_from, buf, sizeof buf), nread > 0)
    {
        char *out_ptr = buf;
        ssize_t nwritten;

        do {
            nwritten = write(fd_to, out_ptr, nread);

            if (nwritten >= 0)
            {
                nread -= nwritten;
                out_ptr += nwritten;
            }
            else if (errno != EINTR)
            {
                goto out_error;
            }
        } while (nread > 0);
    }

    if (nread == 0)
    {
        if (close(fd_to) < 0)
        {
            fd_to = -1;
            goto out_error;
        }
        close(fd_from);

        /* Success! */
        return 0;
    }

  out_error:
    saved_errno = errno;

    close(fd_from);
    if (fd_to >= 0)
        close(fd_to);

    errno = saved_errno;
    return -1;
}

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

void save_pixels (TIFF *tif, uint32 *raster, size_t len)
{
  unsigned char *buf = NULL;
  uint32 h, w, sampleperpixel;
  unsigned short planar;
  TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &h);
  TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &w);
  TIFFGetField(tif, TIFFTAG_SAMPLESPERPIXEL, &sampleperpixel);
  TIFFGetField(tif, TIFFTAG_PLANARCONFIG, &planar);
  for (uint32 row = 0; row < h; row++){
    uint32 position = (h - row - 1) * w;
    uint32 *p = &raster[position];
    int x = TIFFWriteScanline(tif, p, row, 1);
  }
}

uint32 *calc_pga_pixels (uint32 *raster, size_t len, int low, int high, tiff_palette_t *palette)
{
  if (!raster) return NULL;

  for (int i = 0; i < len; i++){
    unsigned char r, g, b, a;
    r = TIFFGetR(raster[i]);
    g = TIFFGetG(raster[i]);
    b = TIFFGetB(raster[i]);
    a = TIFFGetA(raster[i]);
    if (a == 0){
      //ignore transparent pixels
      continue;
    }

    //calculate GA, multiply by 100
    float value = get_green_average (r, g, b) * 100;


    if (value < low){
      raster[i] = tiff_gray();
    }
    if (value >= high){
      raster[i] = tiff_black();
    }
    if (value >= low && value < high){

      float dif = high - low;
      int index = (value - (float)low)/(dif/(float)palette->size);
      
      if (index < palette->size && index >= 0){
        raster[i] = get_tiff_color_from_palette (index, palette);
      }else{
        raster[i] = tiff_black();
      }
    }
  }
  return raster;
}

int main (int argc, char **argv)
{
  if (argc != 6){
    printf("Usage: %s <IMAGE.tif> <PALETTE> <L_LIMIT> <H_LIMIT> <OUTPUT.tif>\n", argv[0]);
    exit(1);
  }
  
  const char *source = argv[1];
  const char *dest = argv[5];
  tiff_palette_t *palette = read_tiff_palette_from_file (argv[2]);

  int l_limit = atoi(argv[3]);
  int h_limit = atoi(argv[4]);

  if (cp(dest, source)){
    printf ("Copy failed\n");
    return 1;
  }
  
  TIFF *tif = XTIFFOpen (dest, "r+");
  
  if (tif){

    uint32 sampleperpixel;
    TIFFGetField(tif, TIFFTAG_SAMPLESPERPIXEL, &sampleperpixel);
    
    char emsg[1024]; 
    if (!TIFFRGBAImageOK (tif, emsg)){
      printf("image not okay to be processed as a rgba image\n");
      return 1;
    }

    //read pixels from TIFF file
    printf("Reading pixels...\n");
    size_t len;
    uint32 *raster = read_pixels(tif, &len);
    if (!raster){
      printf("Read pixels failed.\n");
      return 1;
    }
    printf("Calculating GA pixels...\n");
    //calculate GA, then apply the palette according to parameters
    raster = calc_pga_pixels(raster, len, l_limit, h_limit, palette);
    
    printf("Saving pixels...\n");
    //save new colors to the same TIFF file
    save_pixels (tif, raster, len);
    
    _TIFFfree (raster);
  }
  XTIFFClose(tif);
  return 0;
}
