/*
 * otonchev/grove_dust
 * Copyright (C) 2016 Ognyan Tonchev otonchev@gmail.com
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <math.h>

#include "air_utils.h"

/* convert pcs/0.01cf to μg/m3 */
float
pm25pcs2ugm3 (float concentration_pcs)
{
  double pi = 3.14159;
  double density = 1.65 * pow (10, 12);
  double r25 = 0.44 * pow (10, -6);
  double vol25 = (4/3) * pi * pow (r25, 3);
  double mass25 = density * vol25;
  double K = 3531.5;

  return (concentration_pcs) * K * mass25;
}

#define AQI_LEVELS 7

static struct pm25aqi {
    float clow;
    float chigh;
    int llow;
    int lhigh;
} pm25aqi[] = {
  {0.0,    12.0,   0, 50},
  {12.1,   35.4,  51, 100},
  {35.5,   55.4, 101, 150},
  {55.5,  150.4, 151, 200},
  {150.5, 250.4, 201, 300},
  {250.5, 350.4, 301, 350},
  {350.5, 500.4, 401, 500},
};

/* calculate AQI (Air Quality Index) based on μg/m3 concentration */
int
pm25ugm32aqi (float concentration_ugm3)
{
  int i;

  for (i = 0; i < AQI_LEVELS; i++) {
    if (concentration_ugm3 >= pm25aqi[i].clow &&
        concentration_ugm3 <= pm25aqi[i].chigh) {
      return ((pm25aqi[i].lhigh - pm25aqi[i].llow) /
          (pm25aqi[i].chigh - pm25aqi[i].clow)) *
              (concentration_ugm3 - pm25aqi[i].clow) + pm25aqi[i].llow;
    }
  }

  return 0;
}
