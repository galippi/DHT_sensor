#ifndef _DHT11_CONF_H_
#define _DHT11_CONF_H_

#include "timer.h"

#define DHT11_DEBUG 1

#define DHT_TimerResolution 0.000001 /* sec */
#define DHT_Time2TimerTick(sec) ((uint32_t)((sec) / (DHT_TimerResolution)))
#define DHT_Time2TimerTickUs(us) DHT_Time2TimerTick((us)*0.000001)
#define DHT_TimeCheck(timer, us, diff) (((timer) >= (DHT_Time2TimerTickUs((us)-(diff))) && ((timer) <= (DHT_Time2TimerTickUs((us)+(diff))))

#define DHT11_run_period_time 0.010
#define DHT11_run_period_time_us (uint32_t)((DHT11_run_period_time) / 0.000001)
#define DHT11_PULSE_LOW_CTR (uint32_t)(0.021 / (DHT11_run_period_time)) /* min 18/20ms */
#define DHT11_PULSE_TIMEOUT (uint32_t)(0.008 / (DHT_TimerResolution)) /* 8000 us <= 80 bit * 100us */
#define DHT11_STANDBY_CNT (uint32_t)(2.5 / (DHT11_run_period_time)) /* 2.5 sec standby after the last measurement */

/* Port-B - pin 8 - DHT11 - data - TIM4 - CH3/CH4 */
#define DHT11_PORT_IN  GPIOB, 8
#define DHT11_PORT_OUT GPIOB, 9
#define DHT11_PORT_INIT() \
    do { \
        GPIO_PortInit_Out(DHT11_PORT_OUT); \
        GPIO_PortInit_In(DHT11_PORT_IN); \
    } while(0)
//#define DHT11_OUT_INIT() GPIO_PortInit_Out(DHT11_PORT)
#define DHT11_OUT_LOW()  GPIO_Set(DHT11_PORT_OUT, 1) /* inerting by external circuit */
#define DHT11_OUT_HIGH() GPIO_Set(DHT11_PORT_OUT, 0)
#define DHT11_CC_IRQ_ENABLE() \
    do { \
        TIM4_SR_CC3IF_Reset(); \
        TIM4_SR_CC3OF_Reset(); \
        TIM4_SR_CC4IF_Reset(); \
        TIM4_SR_CC4OF_Reset(); \
        TIM4->DIER |= (TIM_DIER_CC3IE | TIM_DIER_CC4IE); \
        NVIC_EnableIRQ(TIM4_IRQn); \
    }while(0)

//#define DHT11_timer_get() TIM4_CCR3_Get()

#define DHT11_get_time_since_last_irq() \
    ((TIM4_Cnt_Get() - dht11.DHT11_lastTimer) & 0xFFFF)

//#define DHT11_WAIT_US(us)
#define DHT11_ANSWER_PULSE_CNT 84
#define DHT11_IRQ_CNT_MAX 128

#define TIM4_CC3IF_Callback() \
    do { \
        uint16_t timer = TIM4_CCR3_Get(); \
        dht11_IRQ_cb(timer); \
    }while(0)

#define TIM4_CC4IF_Callback() \
    do { \
        uint16_t timer = TIM4_CCR4_Get(); \
        dht11_IRQ_cb(timer); \
    }while(0)

#define DHT11_MissingFallingEdgeIRQ_cb() /* do nothing */

#endif /* _DHT11_CONF_H_ */
