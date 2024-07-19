#ifndef _DHT11_CONF_H_
#define _DHT11_CONF_H_

#include "timer.h"

#define DHT11_DEBUG 1

#define DHT11_run_period_time_us 10000
#define DHT11_PULSE_LOW_CTR 2 /* min 18/20ms */
#define DHT11_PULSE_TIMEOUT 8000 /* 8000 us <= 80 bit * 100us */
#define DHT11_STANDBY_CNT 100 /* 1 sec standby after the last measurement */

/* Port-B - pin 8 - DHT11 - data - TIM4 - CH3/CH4 */
#define DHT11_PORT GPIOB, 8
#define DHT11_OUT_INIT() GPIO_PortInit_OC(DHT11_PORT)
//#define DHT11_OUT_INIT() GPIO_PortInit_Out(DHT11_PORT)
#define DHT11_OUT_LOW()  GPIO_Set(DHT11_PORT, 0)
#define DHT11_OUT_HIGH() GPIO_Set(DHT11_PORT, 1)
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
#define DHT11_ANSWER_PULSE_CNT 80
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
