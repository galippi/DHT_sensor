
typedef enum {
    e_DHT11_State_Init,
    e_DHT11_State_Running_StartPulseLow,
    e_DHT11_State_Running,
    e_DHT11_State_Standby,
    e_DHT11_State_Timeout,
}t_DHT11_State;

typedef struct {
    t_DHT11_State state;
    uint16_t temp;
    uint16_t humidity;
}t_DHT11;

t_DHT11 dht11;

#define DHT11_TEMP_NA 0xFFFF
#define DHT11_HUMIDITY_NA 0xFFFF

void dht11_init(void)
{
    dht11.state = e_DHT11_State_Init;
    dht11.temp = DHT11_TEMP_NA;
    dht11.humidity = DHT11_HUMIDITY_NA;
}

void dht11_run(void)
{

}

void dht11_request(void)
{
    dht11.state = e_DHT11_State_Running_StartPulseLow;
    DHT11_OUT_LOW();
}

