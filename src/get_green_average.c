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
#include <stdio.h>
#include <stdlib.h>
#include <jpeglib.h>
#include <sys/time.h>
#include <math.h>
#include <strings.h>

typedef struct image {
  unsigned char *image;
  int width;
  int height;
  unsigned long size;
  int channels;
}image_t;

static double gettime (void)
{
  struct timeval tr;
  gettimeofday(&tr, NULL);
  return (double)tr.tv_sec+(double)tr.tv_usec/1000000;
}

image_t *load_jpeg_image (const char *filename)
{
  image_t *image;        // data for the image
  
  struct jpeg_decompress_struct info; //for our jpeg info
  struct jpeg_error_mgr err;          //the error handler
  FILE *file = fopen(filename, "rb");  //open the file
  info.err = jpeg_std_error(&err);
  jpeg_create_decompress(&info);   //fills info structure
  if(!file) {
     fprintf(stderr, "Error reading JPEG file %s!", filename);
     return NULL;
  }
  jpeg_stdio_src(&info, file);    
  jpeg_read_header(&info, TRUE);   // read jpeg file header
  jpeg_start_decompress(&info);    // decompress the file

  image = (image_t*)malloc(sizeof(image_t));
  image->width = info.output_width;
  image->height = info.output_height;
  image->channels = info.num_components;
  image->size = image->width * image->height * image->channels;
  image->image = (unsigned char *)malloc(image->size);
  
  while (info.output_scanline < info.output_height){
    unsigned char *rowptr = &image->image[info.output_scanline * image->width * image->channels];
    jpeg_read_scanlines(&info, &rowptr, 1);
  }
  jpeg_finish_decompress(&info);   //finish decompressing
  jpeg_destroy_decompress(&info);
  fclose(file);
  return image; /* should be freed */
}

void convert_to_ppm (image_t *image)
{
  //generate PPM file format for checking
  printf ("P3\n");
  printf ("%d %d\n255\n", image->width, image->height);
  int i,j;
  for (i = 0; i < image->size; i += image->width*image->channels){ //para cada linha
    for (j = 0; j < (image->width*image->channels); j++){
      printf ("%u ", image->image[i + j]);
    }
    printf ("\n");
  }
}

int apply_mask (image_t *image, image_t *mask)
{
  if (image->width == mask->width && image->height == image->height) {
#pragma omp parallel for shared(mask, image)
    for (int i = 0; i < mask->size; i = i+3) {
      unsigned char r = mask->image[i+0];
      unsigned char g = mask->image[i+1];
      unsigned char b = mask->image[i+2];
      if (!(r == 255 && g == 255 && b == 255)){
        image->image[i+0] = 0;
        image->image[i+1] = 0;
        image->image[i+2] = 0;
      }
    }
  } else {
    printf ("Image and Mask have different dimensions. Error.\n");
    exit(1);
  }
}

float get_green_average (unsigned char r, unsigned char g, unsigned char b)
{
  return (r+g+b) == 0 ? 0 : (float)g/(float)(r+g+b);
}

int is_black (unsigned char r, unsigned char g, unsigned char b)
{
  return !(r+g+b);
}

int get_bin (int grain, int i, image_t *image)
{
  unsigned char *iimage = image->image;
  unsigned char r, g, b;
  r = iimage[i+0];
  g = iimage[i+1];
  b = iimage[i+2];
  if (is_black(r, g, b)) return -1;
  float value = get_green_average (r, g, b) * grain;
  if (value >= grain) value = grain - 1;
  return (int)value;
}

int *get_metric(int grain, image_t *image)
{
  int *ret = (int*)malloc(grain * sizeof(int));
  bzero(ret, grain*sizeof(int));

#pragma omp parallel shared(image, ret) firstprivate(grain)
  {
    int *hist_private;
    int size = grain * sizeof(int);
    hist_private = (int*) malloc (size);
    bzero(hist_private, size);
    
#pragma omp for nowait schedule(dynamic, 32*1024)
    for (int i = 0; i < image->size; i = i+3){
      int x = get_bin (grain, i, image);
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
  if (argc != 4){
    printf("Usage: %s <IMAGE.jpg> <MASK.jpg> <GRAIN>\n", argv[0]);
    return 0;
  }

  image_t *image = load_jpeg_image(argv[1]);
  image_t *mask = load_jpeg_image(argv[2]);
  if (mask){
    apply_mask (image, mask);
  }

  int NUMBER_OF_BUCKETS = atoi(argv[3]);
  int *histogram = get_metric (NUMBER_OF_BUCKETS, image);
  int i;
  for (i = 0; i < NUMBER_OF_BUCKETS; i++){
    printf ("%d", histogram[i]);
    if((i+1) == NUMBER_OF_BUCKETS){
      printf("\n");
    }else{
      printf(",");
    }
  }
  free(histogram);
  if (mask){
    free(mask->image);
  }
  free(mask);
  free(image->image);
  free(image);
  return 0;
}
