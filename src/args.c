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
#include "args.h"

static PGAMetricType strtometric (const char *arg)
{
  if (strcmp(arg, "RED") == 0){
    return Red;
  }else if (strcmp(arg, "GREEN") == 0){
    return Green;
  }else if (strcmp(arg, "BLUE") == 0){
    return Blue;
  }
  return Undef;
}

error_t parse_options (int key, char *arg, struct argp_state *state)
{
  struct arguments *arguments = (struct arguments*)(state->input);
  switch (key){
  case 'm': arguments->metric = strtometric(arg); break;
  case 's': arguments->mask = arg; break;
  case 'g': arguments->grain = atoi(arg); break;
  case ARGP_KEY_ARG:
    arguments->input[arguments->ninput] = arg;
    arguments->ninput++;
    break;
  case ARGP_KEY_END:
    //no argument is okay, nothing to check
    if (arguments->ninput == 0){
      fprintf (stderr, "Error: no file has been provided as input\n");
      exit(1);
    }
    break;
  default: return ARGP_ERR_UNKNOWN;
  }
  return 0;
}



