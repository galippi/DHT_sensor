#include <stdint.h>

#include "dht11.h"
#include "gpio.h"

#ifndef DHT_MODE
#define DHT_MODE DHT_MODE_11
#endif

#define enum_packed enum

typedef enum_packed {
    e_DHT11_State_Init,
    e_DHT11_State_Running_StartPulseLow,
    e_DHT11_State_Running_StartPulseLowDone,
    e_DHT11_State_Running_StartPulseHigh,
    e_DHT11_State_Running_AnswerPulse,
    e_DHT11_State_Running_PreDataPulse,
    e_DHT11_State_Running_DataPulse,
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
    uint32_t addData;
    uint16_t resultCtr;
    uint16_t resultChecksumCtr;
}t_DHT11_Debug;

#if 1
#define DHT11_ERROR(reason, addDataVal) \
    do { \
        if ((dht11.dbg.errorReason == e_DHT11_error_Ok) || ((reason) == e_DHT11_error_Ok)) { \
            dht11.dbg.errorReason = (reason); \
            dht11.dbg.addData = (addDataVal); \
            if ((reason) != e_DHT11_error_Standby) \
                dht11.counter = 0; \
        } \
    } while (0)
#else

static inline void DHT11_ERROR(t_DHT11_State reason, uint32_t addDataVal) {
    if ((dht11.dbg.errorReason == e_DHT11_error_Ok) || ((reason) == e_DHT11_error_Ok)) { \
        dht11.dbg.errorReason = (reason); \
        dht11.dbg.addData = (addDataVal); \
        if ((reason) != e_DHT11_error_Standby) \
            dht11.counter = 0; \
}
#endif

#else

#define DHT11_ERROR(reason, addDataVal) /* do nothing */

#endif

typedef struct {
    t_DHT11_State state;
    t_DHT11_Result result;
    uint16_t counter;
    uint8_t runningTimeoutCtr;
    uint16_t DHT11_lastTimer;
    uint8_t rawResult[(((DHT11_IRQ_CNT_MAX - 4) / 2) + 7) / 8];
#ifdef DHT11_DEBUG
    t_DHT11_Debug dbg;
#endif
}t_DHT11;

t_DHT11 dht11;

#ifndef DHT_ErrorResultChecksum_cb
#define DHT_ErrorResultChecksum_cb() // do nothing
#endif

static void dht11_processResult(void)
{
    uint8_t sum = dht11.rawResult[0] + dht11.rawResult[1] + dht11.rawResult[2] + dht11.rawResult[3];
    if (dht11.rawResult[4] == sum) {
#if DHT_MODE == DHT_MODE_11
        int16_t val = ((dht11.rawResult[2] & 0x7F) * 10) + dht11.rawResult[3];
        if (dht11.rawResult[2] & 0x80)
            val = -val;
        dht11.result.temperature = val;
        dht11.result.humidity = (dht11.rawResult[0] * 10) + dht11.rawResult[1];
#elif DHT_MODE == DHT_MODE_22
        int16_t val = ((dht11.rawResult[2] & 0x7F) * 256) + dht11.rawResult[3];
        if (dht11.rawResult[2] & 0x80)
            val = -val;
        dht11.result.temperature = val;
        dht11.result.humidity = (dht11.rawResult[0] * 256) + dht11.rawResult[1];
#else
#error Invalid DHT_MODE!
#endif
#ifdef DHT11_DEBUG
        dht11.dbg.resultCtr++;
#endif
        }else{
        DHT_ErrorResultChecksum_cb();
#if defined(DHT_ResultTimeoutCtr)
        if (dht11.result.timeoutCtr >= DHT_ResultTimeoutCtr) {
#endif // defined(DHT_ResultTimeoutCtr)
            dht11.result.temperature = DHT11_TEMPERATURE_NA;
            dht11.result.humidity = DHT11_HUMIDITY_NA;
#ifdef DHT11_DEBUG
        dht11.dbg.resultChecksumCtr++;
#endif
#if defined(DHT_ResultTimeoutCtr)
        }else{
            dht11.result.timeoutCtr++;
        }
#endif // defined(DHT_ResultTimeoutCtr)
    }

}

void dht11_init(void)
{
    dht11.state = e_DHT11_State_Init;
    dht11.result.temperature = DHT11_TEMPERATURE_NA;
    dht11.result.humidity = DHT11_HUMIDITY_NA;
    DHT11_PORT_INIT();
    DHT11_OUT_HIGH();
    DHT11_CC_IRQ_ENABLE();
}

typedef struct {
    uint16_t ctr, tlast, tavg;
}t_dbg2;
extern t_dbg2 dbg2;

#ifndef DHT_Result_cb
#define DHT_Result_cb() /* do nothing */
#endif

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
                    DHT11_ERROR(e_DHT11_error_NoLowIrq, 0);
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
                    DHT_Result_cb();
                }else{
                    DHT11_ERROR(e_DHT11_error_Timeout, dht11.counter);
                    dht11.state = e_DHT11_State_Error;
                }
            }else{

            }
            break;
        case e_DHT11_State_Standby:
        case e_DHT11_State_Error:
            if (dht11.counter < DHT11_STANDBY_CNT)
                dht11.counter++;
            else {
                dht11.state = e_DHT11_State_Init;
                DHT11_ERROR(e_DHT11_error_Ok, 0);
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
        DHT11_ERROR(e_DHT11_error_Standby, 0);
        return e_DHT11_status_Pause;
    }else{
        DHT11_ERROR(e_DHT11_error_RequestIsRejected, 0);
        return e_DHT11_status_Error;
    }
}

t_DHT11_Result dht_getResult(void)
{
    return dht11.result;
}

void dht11_IRQ_cb(uint16_t timer)
{
    uint16_t dt = timer - dht11.DHT11_lastTimer;
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
            dht11.dbg.dt[0] = dt;
#endif
            dht11.state = e_DHT11_State_Running;
            break;
        case e_DHT11_State_Running:
        {
            #ifdef DHT11_DEBUG
                uint8_t idx = 2 + dht11.counter;
                dht11.dbg.times[idx] = timer;
                dht11.dbg.dt[idx - 1] = dt;
            #endif
            switch (dht11.counter)
            {
                case 0: // 1st pulse -> do nothing
                    break;
                case 1:
                case 2:
                    // the pulse with shall be c.a. 85us
                    if ((dt > DHT_Time2TimerTickUs(75)) && (dt < DHT_Time2TimerTickUs(95))) {
                        // valid answer -> do nothing
                    }else{ // invalid answer
                        dht11.state = e_DHT11_State_Error;
                        DHT11_ERROR(e_DHT11_State_Running_AnswerPulse, (dt << 8) | dht11.counter);
                    }
                    break;
                default:
                    if (dht11.counter < DHT11_IRQ_CNT_MAX) {
                        if (dht11.counter & 0x01) {
                            // odd (low) pulse width - it shall be fixed 54us
                            if ((dt > DHT_Time2TimerTickUs(54-10)) && (dt < DHT_Time2TimerTickUs(54+10))) {
                                // valid answer -> do nothing
                            }else{ // invalid answer
                                dht11.state = e_DHT11_State_Error;
                                DHT11_ERROR(e_DHT11_State_Running_PreDataPulse, (dt << 8) | dht11.counter);
                            }
                        }else{
                            // even (high) pulse width - it shall be 25us or 71us
                            uint8_t resultBitIdx = (dht11.counter - 4) / 2;
                            uint8_t resultByteIdx = resultBitIdx / 8;
                            if ((dt > DHT_Time2TimerTickUs(25-10)) && (dt < DHT_Time2TimerTickUs(25+10))) {
                                // valid 0 answer -> put it
                                dht11.rawResult[resultByteIdx] = ((dht11.rawResult[resultByteIdx] << 1) | 0);
                            }else
                            if ((dt > DHT_Time2TimerTickUs(71-10)) && (dt < DHT_Time2TimerTickUs(71+10))) {
                                // valid 1 answer -> put it
                                dht11.rawResult[resultByteIdx] = ((dht11.rawResult[resultByteIdx] << 1) | 1);
                            }else{ // invalid answer
                                dht11.state = e_DHT11_State_Error;
                                DHT11_ERROR(e_DHT11_State_Running_DataPulse, (dt << 8) | dht11.counter);
                            }
                        }
                    } else {
                        dht11.state = e_DHT11_State_Error;
                        DHT11_ERROR(e_DHT11_error_TooManyIrq, dht11.counter);
                    }
                    break;
                }
            if (dht11.state != e_DHT11_State_Error)
                dht11.counter++;
            }
            break;
        default:
            dht11.state = e_DHT11_State_Error;
            DHT11_ERROR(e_DHT11_error_InvalidIrq, 0);
            break;
    }
    dht11.DHT11_lastTimer = timer;
}
