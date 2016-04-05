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

static void
pin_wait_level (int fd, int level)
{
  struct pollfd fds;
  int res;

  fds = (const struct pollfd) { 0 };
  fds.fd = fd;
  fds.events = POLLPRI;

  do {
    int rc;
    char buf[2];

    rc = poll (&fds, 1, 100000);
    if (rc < 0) {
      fprintf (stderr, "Error on poll!\n");
      return;
    }

    lseek (fd, 0, SEEK_SET);
    read (fd, buf, 2);
    buf[1] = 0;

    res = atoi (buf);

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

  pin_wait_level (data->fd, level);
  gettimeofday (&t1, NULL);
  pin_wait_level (data->fd, 1 - level);
  gettimeofday (&tn, NULL);

  micros = (tn.tv_sec - t1.tv_sec) * 1000000L;
  micros += (tn.tv_usec - t1.tv_usec);

  return micros;
}
