#ifndef __LNGPIO_H__
#define __LNGPIO_H__

typedef enum LNGPIOPinDirection
{
  LNGPIO_PIN_DIRECTION_IN,
  LNGPIO_PIN_DIRECTION_OUT,
} LNGPIOPinDirection;

typedef enum LNGPIOPinEdge
{
  LNGPIO_PIN_EDGE_NONE,
  LNGPIO_PIN_EDGE_RISING,
  LNGPIO_PIN_EDGE_FALLING,
  LNGPIO_PIN_EDGE_BOTH,
} LNGPIOPinEdge;

int lngpio_is_exported (int pin);
int lngpio_export (int pin);
int lngpio_unexport (int pin);
int lngpio_wait_for_pin (int pin);
int lngpio_set_direction (int pin, LNGPIOPinDirection direction);
int lngpio_set_edge (int pin, LNGPIOPinEdge edge);
int lngpio_read (int pin);

typedef struct _LNGPIOPinData LNGPIOPinData;

LNGPIOPinData* lngpio_pin_open (int pin);
int lngpio_pin_release (LNGPIOPinData *data);
int lngpio_pin_pulse_len (LNGPIOPinData *data, int level);

typedef struct _LNGPIOPinMonitor LNGPIOPinMonitor;
typedef void (*LNGPIOPinStatusChanged) (int, int);

LNGPIOPinMonitor* lngpio_pin_monitor_create (int pin,
    LNGPIOPinStatusChanged status_changed);
int lngpio_pin_monitor_stop (LNGPIOPinMonitor *monitor);

#endif //__LNGPIO_H__
