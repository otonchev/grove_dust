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
#include "lngpio.h"

#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <poll.h>
#include <string.h>
#include <assert.h>
#include<pthread.h>

static const char *pin_dir_str[] = {
  "in",
  "out",
};

static const char *pin_edge_str[] = {
  "none",
  "rising",
  "falling",
  "both",
};

struct _LNGPIOPinData
{
  int fd;
  struct pollfd fds;
};

struct _LNGPIOPinMonitor
{
  pthread_t thread_id;
  pthread_mutex_t lock;
  LNGPIOPinStatusChanged callback;
  LNGPIOPinData *pin_data;
  int stop_thread;
  int pin;
};

int
lngpio_is_exported (int pin)
{
  #define DIRECTION_MAX 35
  char path[DIRECTION_MAX];

  snprintf (path, DIRECTION_MAX, "/sys/class/gpio/gpio%d/direction", pin);

  if (access (path, W_OK) == 0) {
    return 1;
  }

  return 0;
}

int
lngpio_export (int pin)
{
  #define BUFFER_MAX 3
  char buffer[BUFFER_MAX];
  ssize_t bytes_to_write;
  ssize_t ret;
  int fd;

  fd = open ("/sys/class/gpio/export", O_WRONLY);
  if (-1 == fd) {
    fprintf (stderr, "Failed to open export for writing!\n");
    return (-1);
  }

  bytes_to_write = snprintf (buffer, BUFFER_MAX, "%d", pin);
  ret = write (fd, buffer, bytes_to_write);
  if (-1 == ret) {
    fprintf (stderr, "Failed to export pin %d!\n", pin);
    return (-1);
  }

  assert (ret == bytes_to_write);

  close (fd);
  return (0);
}

int
lngpio_unexport (int pin)
{
  char buffer[BUFFER_MAX];
  ssize_t bytes_to_write;
  ssize_t ret;
  int fd;

  fd = open ("/sys/class/gpio/unexport", O_WRONLY);
  if (-1 == fd) {
    fprintf (stderr, "Failed to open unexport for writing!\n");
    return (-1);
  }

  bytes_to_write = snprintf (buffer, BUFFER_MAX, "%d", pin);
  ret = write (fd, buffer, bytes_to_write);
  if (-1 == ret) {
    fprintf (stderr, "Failed to unexport pin %d!\n", pin);
    return (-1);
  }

  assert (ret == bytes_to_write);

  close (fd);
  return (0);
}

int
lngpio_wait_for_pin (int pin)
{
  #define DIRECTION_MAX 35
  char path[DIRECTION_MAX];
  int max_retries = 20;
  int count = 0;

  snprintf (path, DIRECTION_MAX, "/sys/class/gpio/gpio%d/direction", pin);

  while (1) {
    if (count++ >= max_retries) {
      fprintf (stderr, "Pin %d not exported!\n", pin);
      return (-1);
    }
    if (access (path, W_OK) == 0) {
      break;
    }
    usleep (100000);
  }

  return 0;
}

int
lngpio_set_direction (int pin, LNGPIOPinDirection direction)
{
  #define DIRECTION_MAX 35
  char path[DIRECTION_MAX];
  int fd;

  snprintf (path, DIRECTION_MAX, "/sys/class/gpio/gpio%d/direction", pin);
  fd = open (path, O_WRONLY);
  if (-1 == fd) {
    fprintf (stderr, "Failed to open lngpio direction for writing!\n");
    return (-1);
  }

  if (-1 == write (fd, pin_dir_str[direction],
      strlen (pin_dir_str[direction]))) {
    fprintf (stderr, "Failed to set direction!\n");
    return (-1);
  }

  close (fd);
  return (0);
}

/* https://www.kernel.org/doc/Documentation/gpio/sysfs.txt */
int
lngpio_set_edge (int pin, LNGPIOPinEdge edge)
{
  #define DIRECTION_MAX 35
  char path[DIRECTION_MAX];
  int fd;

  snprintf (path, DIRECTION_MAX, "/sys/class/gpio/gpio%d/edge", pin);
  fd = open (path, O_WRONLY);
  if (-1 == fd) {
    fprintf (stderr, "Failed to open lngpio direction for writing!\n");
    return (-1);
  }

  if (-1 == write (fd, pin_edge_str[edge],
      strlen (pin_edge_str[edge]))) {
    fprintf (stderr, "Failed to set edge!\n");
    return (-1);
  }

  close (fd);
  return (0);
}

int
lngpio_read (int pin)
{
  #define VALUE_MAX 30
  char path[VALUE_MAX];
  FILE *f;
  int result;

  snprintf (path, VALUE_MAX, "/sys/class/gpio/gpio%d/value", pin);
  f = fopen (path, "rw");
  fscanf (f, "%d", &result);
  fclose (f);

  return result;
}

static int
pin_get_level (int fd, struct pollfd *fds)
{
  int res;
  int rc;
  char buf[3];
  ssize_t bytes;

  rc = poll (fds, 1, 100000);
  if (rc < 0) {
    fprintf (stderr, "Error on poll!\n");
    return -1;
  }

  lseek (fd, 0, SEEK_SET);
  bytes = read (fd, buf, 3);
  buf[1] = 0;

  res = atoi (buf);

  return res;
}

static void
pin_wait_level (int fd, struct pollfd *fds, int level)
{
  int res;

  do {
    res = pin_get_level (fd, fds);
  } while (res != level);
}

LNGPIOPinData*
lngpio_pin_open (int pin)
{
  #define VALUE_MAX 30
  char path[VALUE_MAX];
  int fd;
  LNGPIOPinData *data;

  snprintf (path, VALUE_MAX, "/sys/class/gpio/gpio%d/value", pin);
  fd = open (path, O_RDONLY);
  if (fd == -1) {
    fprintf (stderr, "Unable to open %s\n", path);
    return NULL;
  }

  data = malloc (sizeof (LNGPIOPinData));
  data->fd = fd;

  data->fds = (const struct pollfd) { 0 };
  data->fds.fd = fd;
  data->fds.events = POLLPRI;

  return data;
}

int
lngpio_pin_release (LNGPIOPinData *data)
{
  close (data->fd);
  free (data);
  return 0;
}

int
lngpio_pin_pulse_len (LNGPIOPinData *data, int level)
{
  struct timeval tn, t1;
  long micros;

  pin_wait_level (data->fd, &data->fds, level);
  gettimeofday (&t1, NULL);
  pin_wait_level (data->fd, &data->fds, 1 - level);
  gettimeofday (&tn, NULL);

  micros = (tn.tv_sec - t1.tv_sec) * 1000000L;
  micros += (tn.tv_usec - t1.tv_usec);

  return micros;
}

static void*
monitor_thread (void *data)
{
  LNGPIOPinMonitor *monitor = (LNGPIOPinMonitor *)data;
  int stop_thread = 0;
  int latest_status = -1;

  while (stop_thread == 0) {
    int status;

    status = pin_get_level (monitor->pin_data->fd, &monitor->pin_data->fds);

    if (latest_status == -1) {
      latest_status = status;
      continue;
    }

    if (latest_status != status) {
      monitor->callback (monitor->pin, status);
      latest_status = status;
    }

    pthread_mutex_lock (&monitor->lock);
    stop_thread = monitor->stop_thread;
    pthread_mutex_unlock (&monitor->lock);
  }

  return NULL;
}

LNGPIOPinMonitor*
lngpio_pin_monitor_create (int pin, LNGPIOPinStatusChanged status_changed)
{
  LNGPIOPinMonitor *monitor;

  monitor = malloc (sizeof (LNGPIOPinMonitor));
  *monitor = (LNGPIOPinMonitor) { 0 };
  monitor->pin = pin;
  monitor->callback = status_changed;

  monitor->pin_data = lngpio_pin_open (pin);

  if (pthread_mutex_init (&monitor->lock, NULL) != 0) {
    free (monitor);
    return NULL;
  }

  if (pthread_create (&monitor->thread_id, NULL, monitor_thread, monitor)) {
    pthread_mutex_destroy (&monitor->lock);
    free (monitor);
    return NULL;
  }

  return monitor;
}

int
lngpio_pin_monitor_stop (LNGPIOPinMonitor *monitor)
{
  pthread_mutex_lock (&monitor->lock);
  monitor->stop_thread = 1;
  pthread_mutex_unlock (&monitor->lock);

  pthread_join (monitor->thread_id, NULL);
  pthread_mutex_destroy (&monitor->lock);

  lngpio_pin_release (monitor->pin_data);

  free (monitor);

  return 0;
}
