#include "lngpio.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>

#define LOW  0
#define HIGH 1

#define PIN  4

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

static void
loop (LNGPIOPinData *data)
{
  unsigned long pulse_duration;

  pulse_duration = lngpio_pin_pulse_len (data, LOW);
  printf ("pulse duration: %ld\n", pulse_duration);

  lowpulseoccupancy = lowpulseoccupancy + pulse_duration;

  if ((millis () - starttime) > sampletime_ms) {
    float ratio;
    float concentration_pcs;
    float concentration_ugm3;

    ratio = lowpulseoccupancy / (sampletime_ms * 10.0);
    concentration_pcs =
        1.1 * pow (ratio, 3) - 3.8 * pow (ratio, 2) + 520 * ratio + 0.62;
    concentration_ugm3 = pcs2ugm3 (concentration_pcs);

    printf ("%f pcs/0.01cf, %f μg/m3\n", concentration_pcs, concentration_ugm3);

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
