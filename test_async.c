/*
 * (c) 2016 Ognyan Tonchev otonchev@gmail.com
 * Example application monitoring Fine particle (PM2.5) with Grove Dust sensor
 * (Shinyei PPD42NS) and a Raspberry Pi.
 * The app uses lngpio's asynchronous API.
 */
#include "lngpio.h"
#include "air_utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>

#define LOW  0
#define HIGH 1

#define PIN  17

static unsigned long starttime;
static unsigned long sampletime_ms = 30000; /* 30s */
static unsigned long lowpulseoccupancy;
static struct timeval t_low, t_high;

static long
millis ()
{
  struct timeval  tv;
  gettimeofday (&tv, NULL);

  return (tv.tv_sec) * 1000 + (tv.tv_usec) / 1000 ;
}

static void
pulse_detected (unsigned long pulse_duration)
{
  lowpulseoccupancy = lowpulseoccupancy + pulse_duration;

  if ((millis () - starttime) > sampletime_ms) {
    float ratio;
    float concentration_pcs;
    float concentration_ugm3;
    int aqi;

    ratio = lowpulseoccupancy / (sampletime_ms * 10.0);
    concentration_pcs =
        1.1 * pow (ratio, 3) - 3.8 * pow (ratio, 2) + 520 * ratio + 0.62;
    concentration_ugm3 = pm25pcs2ugm3 (concentration_pcs);
    aqi = pm25ugm32aqi (concentration_ugm3);

    printf ("%f pcs/0.01cf, %f μg/m3, %d AQI\n", concentration_pcs,
        concentration_ugm3, aqi);

    lowpulseoccupancy = 0;
    starttime = millis ();
  }
}

static void
status_changed (int pin, int status)
{
  long micros;

  if (status == 0) {
    gettimeofday (&t_low, NULL);
  } else if (status == 1) {
    gettimeofday (&t_high, NULL);

    micros = (t_high.tv_sec - t_low.tv_sec) * 1000000L;
    micros += (t_high.tv_usec - t_low.tv_usec);

    if (micros > 95000 || micros < 8500)
      printf ("pulse duration out of bounds: %ld\n", micros);

    pulse_detected (micros);
  }
}

int
main (int argc, char * argv[])
{
  LNGPIOPinMonitor *monitor;

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

  monitor = lngpio_pin_monitor_create (PIN, status_changed);
  if (NULL == monitor)
    return (1);

  while (1) {
    usleep (100000);
  }

  if (-1 == lngpio_pin_monitor_stop (monitor))
    return (1);

  if (-1 == lngpio_unexport (PIN))
    return (1);

  return (0);
}
