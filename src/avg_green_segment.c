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
/*
   * This program takes as input a jpeg image and a mask.
   * For every remaining pixel after the mask has been applied,
   * it calculates the green average according to the formula
   * G/(R+G+B). Then, it maps this value to a color palette. It
   * finally dumps as output a colored JPG image.
   */
  #include <stdio.h>
  #include <stdlib.h>
  #include <jpeglib.h>
  #include <sys/time.h>
  #include <math.h>
  #include <strings.h>
  #include "read_palette.h"

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

  void convert_to_jpg (image_t *image, const char *filename)
  {
    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;

    FILE *outfile = fopen( filename, "wb" );
    if (!outfile) {
      printf("Error opening output jpeg file %s\n!", filename);
      exit(1);
    }

    cinfo.err = jpeg_std_error( &jerr );
    jpeg_create_compress(&cinfo);
    jpeg_stdio_dest(&cinfo, outfile);

    cinfo.image_width = image->width;
    cinfo.image_height = image->height;
    cinfo.input_components = image->channels;
    cinfo.in_color_space = JCS_RGB;

    jpeg_set_defaults( &cinfo );

    jpeg_start_compress( &cinfo, TRUE );

    JSAMPROW row_pointer[1];

    while( cinfo.next_scanline < cinfo.image_height ){
       row_pointer[0] = &image->image[ cinfo.next_scanline * cinfo.image_width *  3];
       jpeg_write_scanlines( &cinfo, row_pointer, 1 );
    }
    jpeg_finish_compress( &cinfo );
    jpeg_destroy_compress( &cinfo );
    fclose( outfile );
  }

  int apply_mask (image_t *image, image_t *mask)
  {
    if (image->width == mask->width && image->height == image->height) {
      int i;
#pragma parallel for shared(mask, image)
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

  int *get_metric(int grain, image_t *image)
  {
    int *ret = (int*)malloc(grain * sizeof(int));
    bzero(ret, grain*sizeof(int));
    int i;
    for (i = 0; i < image->size; i = i+3) {
      unsigned char r, g, b;
      r = image->image[i+0];
      g = image->image[i+1];
      b = image->image[i+2];
      if (is_black(r, g, b)) continue;

      float value = get_green_average (r, g, b) * grain;
      if (value >= grain) value = grain - 1;
      ret[(int)floor(value)]++;
    }
    return ret;
  }

  void segment (image_t *image, palette_t *palette, int low, int high, int grain)
  {
    int i;
    for (i = 0; i < image->size; i = i+3) {
      unsigned char r, g, b;
      r = image->image[i+0];
      g = image->image[i+1];
      b = image->image[i+2];
      if (is_black(r, g, b)) continue;

      float value = get_green_average (r, g, b) * grain;
      int selected = truncf(value);
      if (selected >= low && selected < high){
        //change pixel color according to palette and selected index
        int index = selected - low - 1;
        unsigned char r, g, b;
        r = palette->colors[index*3+0];
        g = palette->colors[index*3+1];
        b = palette->colors[index*3+2];
  //          printf ("selected: %d index: %d - %u %u %u\n", selected, index, r, g, b);
        image->image[i+0] = r;
        image->image[i+1] = g;
        image->image[i+2] = b;

      }else{
        //turn pixel to black
        image->image[i+0] = 0;
        image->image[i+1] = 0;
        image->image[i+2] = 0;
      }
    }
  }

  int main (int argc, char **argv)
  {
    if (argc != 8){
      printf("Usage: %s <IMAGE.jpg> <MASK.jpg> <PALETTE> <L_LIMIT> <H_LIMIT> <GRAIN> <OUTPUT.jpg>\n", argv[0]);
      exit(1);
    }

    image_t *image = load_jpeg_image(argv[1]);
    image_t *mask = load_jpeg_image(argv[2]);
    if (mask){
      apply_mask (image, mask);
    }

    palette_t *palette = read_palette_from_file (argv[3]);

    int l_limit = atoi(argv[4]);
    int h_limit = atoi(argv[5]);
    int grain = atoi(argv[6]);

    if (h_limit - l_limit != palette->size){
      printf("Error: range (size %d) and palette size (%d) are different.\n", h_limit - l_limit, palette->size);
    }

    segment (image, palette, l_limit, h_limit, grain);

    convert_to_jpg (image, argv[7]);

    free(mask->image);
    free(mask);
    free(image->image);
    free(image);
    free(palette->colors);
    free(palette);
    return 0;
  }
