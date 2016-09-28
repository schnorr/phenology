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
#include "metrics.h"
#include "args.h"

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

static char doc[] = "Calculate the histogram of FILE, dumping a CSV-like output";
static char args_doc[] = "FILE [FILES]";
static struct argp_option options[] = {
  {"metric", 'm', "METRIC", 0, "Either RED, GREEN or BLUE"},
  {"mask", 's', "MASK", 0, "The image mask that should be used"},
  {"grain", 'g', "GRAIN", 0, "The number of buckets of the histogram"},
  { 0 }
};
struct argp argp = { options, parse_options, args_doc, doc };

image_t *load_jpeg_image (const char *filename)
{
  image_t *image;        // data for the image
  
  struct jpeg_decompress_struct info; //for our jpeg info
  struct jpeg_error_mgr err;          //the error handler
  FILE *file = fopen(filename, "rb");  //open the file
  if(!file) {
     fprintf(stderr, "Error reading JPEG file %s!\n", filename);
     return NULL;
  }
  info.err = jpeg_std_error(&err);
  jpeg_create_decompress(&info);   //fills info structure

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
  if (image->width != mask->width || image->height != image->height) {
    printf ("Image and Mask have different dimensions. Error.\n");
    exit(1);
  }

  int total_after_mask = 0;
  for (int i = 0; i < mask->size; i = i+3) {
    unsigned char r = mask->image[i+0];
    unsigned char g = mask->image[i+1];
    unsigned char b = mask->image[i+2];
    if (!(r == 255 && g == 255 && b == 255)){
      image->image[i+0] = 0;
      image->image[i+1] = 0;
      image->image[i+2] = 0;
    }else{
      total_after_mask++;
    }
  }
  return total_after_mask;
}

int get_bin (PGAMetricType type, int grain, int i, image_t *image)
{
  unsigned char *iimage = image->image;
  unsigned char r, g, b;
  r = iimage[i+0];
  g = iimage[i+1];
  b = iimage[i+2];
  if (is_black(r, g, b)) return -1;
  float value = get_average (type, r, g, b) * grain;
  if (value >= grain) value = grain - 1;
  return (int)value;
}

int *get_metric(int grain, PGAMetricType type, image_t *image)
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
      int x = get_bin (type, grain, i, image);
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
  struct arguments arguments;
  bzero (&arguments, sizeof(struct arguments));
  if (argp_parse (&argp, argc, argv, 0, 0, &arguments) == ARGP_KEY_ERROR){
    fprintf(stderr, "%s, error during the parsing of parameters\n", argv[0]);
    return 1;
  }

  image_t *mask = load_jpeg_image(arguments.mask);
  if (!mask && arguments.mask){
    fprintf (stderr, "%s, you provided mask %s, but the file cannot be openned correctly.\n", argv[0], arguments.mask);
    exit(1);
  }

  int j, i;
  for (j = 0; j < arguments.ninput; j++){
    image_t *image = load_jpeg_image(arguments.input[j]);
    if (mask){
      apply_mask (image, mask);
    }

    int *histogram = get_metric (arguments.grain, arguments.metric, image);
    for (i = 0; i < arguments.grain; i++){
      printf ("%d", histogram[i]);
      if((i+1) == arguments.grain){
	printf("\n");
      }else{
	printf(",");
      }
    }
    free(histogram);
    free(image->image);
    free(image);
  }
  if (mask){
    free(mask->image);
  }
  free(mask);
  return 0;
}
