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
#ifndef __ARGS_H
#define __ARGS_H
#include <stdlib.h>
#include <string.h>
#include <argp.h>

#define MAX_INPUTS 1024
#define VALIDATE_INPUT_SIZE 1

typedef enum { Red, Green, Blue, Undef } PGAMetricType;

struct arguments {
  char *input[MAX_INPUTS];
  int ninput;
  char *mask;
  int grain;
  PGAMetricType metric;
};

error_t parse_options (int key, char *arg, struct argp_state *state);
#endif
