#include <stdint.h>

#include "dht11.h"
#include "gpio.h"

#define enum_packed enum

typedef enum_packed {
    e_DHT11_State_Init,
    e_DHT11_State_Running_StartPulseLow,
    e_DHT11_State_Running_StartPulseLowDone,
    e_DHT11_State_Running_StartPulseHigh,
    e_DHT11_State_Running,
    e_DHT11_State_Error,
    e_DHT11_State_Standby,
    e_DHT11_State_Timeout,
}t_DHT11_State;

#ifdef DHT11_DEBUG
typedef struct {
    uint16_t times[DHT11_IRQ_CNT_MAX + 4];
    uint16_t dt[DHT11_IRQ_CNT_MAX + 3];
    t_DHT11_error errorReason;
}t_DHT11_Debug;

#define DHT11_ERROR(reason) dht11.dbg.errorReason = reason

#else

#define DHT11_ERROR(reason) /* do nothing */

#endif

typedef struct {
    t_DHT11_State state;
    t_DHT11_Result result;
    uint16_t counter;
    uint8_t runningTimeoutCtr;
    uint16_t DHT11_lastTimer;
#ifdef DHT11_DEBUG
    t_DHT11_Debug dbg;
#endif
}t_DHT11;

t_DHT11 dht11;

static void dht11_processResult(void)
{
    dht11.result.temperature = DHT11_TEMPERATURE_NA;
    dht11.result.humidity = DHT11_HUMIDITY_NA;
}

void dht11_init(void)
{
    dht11.state = e_DHT11_State_Init;
    dht11.result.temperature = DHT11_TEMPERATURE_NA;
    dht11.result.humidity = DHT11_HUMIDITY_NA;
    DHT11_OUT_INIT();
    DHT11_OUT_HIGH();
    DHT11_CC_IRQ_ENABLE();
}

typedef struct {
    uint16_t ctr, tlast, tavg;
}t_dbg2;
extern t_dbg2 dbg2;

void dht11_run(void)
{
    switch(dht11.state)
    {
        case e_DHT11_State_Init:
            break;
        case e_DHT11_State_Running_StartPulseLow:
        case e_DHT11_State_Running_StartPulseLowDone:
            if (dht11.counter < DHT11_PULSE_LOW_CTR)
                dht11.counter++;
            else {
                if (dht11.state != e_DHT11_State_Running_StartPulseLowDone) {
                    DHT11_MissingFallingEdgeIRQ_cb();
                }
                dht11.state = e_DHT11_State_Running_StartPulseHigh;
                dht11.counter = 0;
                dht11.runningTimeoutCtr = 0;
                DHT11_OUT_HIGH();
            }
            break;
        case e_DHT11_State_Running:
            if (DHT11_get_time_since_last_irq() > DHT11_PULSE_TIMEOUT) {
                if (dht11.counter == DHT11_ANSWER_PULSE_CNT) {
                    dht11_processResult();
                    dht11.state = e_DHT11_State_Standby;
                    dht11.counter = 0;
                }else{
                    DHT11_ERROR(e_DHT11_error_Timeout);
                    dht11.state = e_DHT11_State_Error;
                }
            }else{

            }
            break;
        case e_DHT11_State_Standby:
            if (dht11.counter < DHT11_STANDBY_CNT)
                dht11.counter++;
            else {
                dht11.state = e_DHT11_State_Init;
                dht11.counter = 0;
            }
            break;
        default:
            break;
    }
    {
        uint16_t t = TIM4_Cnt_Get();
        dbg2.tavg = ((t - dbg2.tlast) + (4 * dbg2.tavg)) / 5;
        dbg2.tlast = t;
        dbg2.ctr++;
    }
}
t_dbg2 dbg2;

t_DHT11_status dht11_request(void)
{
    if (dht11.state == e_DHT11_State_Init) {
        dht11.state = e_DHT11_State_Running_StartPulseLow;
        dht11.counter = 0;
        DHT11_OUT_LOW();
        return e_DHT11_status_Ok;
    }else
    if (dht11.state == e_DHT11_State_Standby) {
        DHT11_ERROR(e_DHT11_error_Standby);
        return e_DHT11_status_Pause;
    }else{
        DHT11_ERROR(e_DHT11_error_RequestIsRejected);
        return e_DHT11_status_Error;
    }
}

void dht11_IRQ_cb(uint16_t timer)
{
    switch(dht11.state)
    {
        case e_DHT11_State_Running_StartPulseLow:
#ifdef DHT11_DEBUG
            dht11.dbg.times[0] = timer;
#endif
            dht11.state = e_DHT11_State_Running_StartPulseLowDone;
            break;
        case e_DHT11_State_Running_StartPulseHigh:
#ifdef DHT11_DEBUG
            dht11.dbg.times[1] = timer;
            dht11.dbg.dt[0] = dht11.dbg.times[1] - dht11.dbg.times[0];
#endif
            dht11.state = e_DHT11_State_Running;
            break;
        case e_DHT11_State_Running:
        {
            #ifdef DHT11_DEBUG
                uint8_t idx = 2 + dht11.counter;
                dht11.dbg.times[idx] = timer;
                dht11.dbg.dt[idx - 1] = timer - dht11.dbg.times[idx - 1];
            #endif
            if (dht11.counter < DHT11_IRQ_CNT_MAX)
                dht11.counter++;
            else {
                dht11.state = e_DHT11_State_Error;
                DHT11_ERROR(e_DHT11_error_TooManyIrq);
            }
        }
            break;
        default:
            dht11.state = e_DHT11_State_Error;
            DHT11_ERROR(e_DHT11_error_InvalidIrq);
            break;
    }
    dht11.DHT11_lastTimer = timer;
}
