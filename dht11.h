#ifndef _DHT11_H_
#define _DHT11_H_

#include "dht11_conf.h"

typedef enum {
    e_DHT11_status_Ok,
    e_DHT11_status_Pause,
    e_DHT11_status_Error,
}t_DHT11_status;

typedef struct {
    uint16_t temperature;
    uint16_t humidity;
}t_DHT11_Result;

typedef enum {
    e_DHT11_error_Ok,
    e_DHT11_error_NoLowIrq,
    e_DHT11_error_NoHighIrq,
    e_DHT11_error_Timeout,
    e_DHT11_error_Standby,
    e_DHT11_error_RequestIsRejected,
    e_DHT11_error_TooManyIrq,
    e_DHT11_error_InvalidIrq,
}t_DHT11_error;

#define DHT11_TEMPERATURE_NA 0xFFFF
#define DHT11_HUMIDITY_NA    0xFFFF

extern void dht11_init(void);
extern void dht11_run(void);
extern void dht11_IRQ_cb(uint16_t timer);
extern t_DHT11_status dht11_request(void);

#endif /* _DHT11_H_ */
