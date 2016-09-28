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
#include "metrics.h"
#include "args.h"

static float get_red_average (unsigned char r, unsigned char g, unsigned char b)
{
  return (r+g+b) == 0 ? 0 : (float)r/(float)(r+g+b);
}

static float get_green_average (unsigned char r, unsigned char g, unsigned char b)
{
  return (r+g+b) == 0 ? 0 : (float)g/(float)(r+g+b);
}

static float get_blue_average (unsigned char r, unsigned char g, unsigned char b)
{
  return (r+g+b) == 0 ? 0 : (float)b/(float)(r+g+b);
}

float get_average (PGAMetricType type, unsigned char r, unsigned char g, unsigned char b)
{
  switch (type){
  case Red: return get_red_average (r, g, b);
  case Green: return get_green_average (r, g, b);
  case Blue: return get_blue_average (r, g, b);
  case Undef:
  default: return get_green_average (r, g, b);
  }
  return 0;
}

int is_black (unsigned char r, unsigned char g, unsigned char b)
{
  return !(r+g+b);
}

