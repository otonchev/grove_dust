#include "lngpio.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>

#define LOW  0
#define HIGH 1

#define PIN  17

unsigned long starttime;
unsigned long sampletime_ms = 30000; /* 30s */
unsigned long lowpulseoccupancy;

long
millis ()
{
  struct timeval  tv;
  gettimeofday (&tv, NULL);

  return (tv.tv_sec) * 1000 + (tv.tv_usec) / 1000 ;
}

/* convert pcs/0.01cf to μg/m3 */
static float
pcs2ugm3 (float concentration_pcs)
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
static int
ugm32aqi (float concentration_ugm3)
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

static void
loop (LNGPIOPinData *data)
{
  unsigned long pulse_duration;

  pulse_duration = lngpio_pin_pulse_len (data, LOW);
  if (pulse_duration > 95000 || pulse_duration < 8500)
     printf ("pulse duration out of bounds: %ld\n", pulse_duration);

  lowpulseoccupancy = lowpulseoccupancy + pulse_duration;

  if ((millis () - starttime) > sampletime_ms) {
    float ratio;
    float concentration_pcs;
    float concentration_ugm3;
    int aqi;

    ratio = lowpulseoccupancy / (sampletime_ms * 10.0);
    concentration_pcs =
        1.1 * pow (ratio, 3) - 3.8 * pow (ratio, 2) + 520 * ratio + 0.62;
    concentration_ugm3 = pcs2ugm3 (concentration_pcs);
    aqi = ugm32aqi (concentration_ugm3);

    printf ("%f pcs/0.01cf, %f μg/m3, %d AQI\n", concentration_pcs,
        concentration_ugm3, aqi);

    lowpulseoccupancy = 0;
    starttime = millis ();
  }
}

int
main (int argc, char * argv[])
{
  LNGPIOPinData *data;

  if (lngpio_is_exported (PIN))
    lngpio_unexport (PIN);

  if (-1 == lngpio_export (PIN))
    return (1);

  if (-1 == lngpio_wait_for_pin (PIN))
    return (1);

  if (-1 == lngpio_set_direction (PIN, LNGPIO_PIN_DIRECTION_IN))
    return (1);

  if (-1 == lngpio_set_edge (PIN, LNGPIO_PIN_EDGE_BOTH))
    return (1);

  data = lngpio_pin_open (PIN);
  if (NULL == data)
    return (1);

  while (1) {
    loop (data);
  }

  if (-1 == lngpio_pin_release (data))
    return (1);

  if (-1 == lngpio_unexport (PIN))
    return (1);

  return (0);
}
