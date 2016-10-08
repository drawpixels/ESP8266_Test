#include <ets_sys.h>
#include <osapi.h>
#include <gpio.h>
#include <os_type.h>
#include <pwm.h>
#include <user_interface.h>
#include "user_config.h"

#define user_procTaskPrio        0
#define user_procTaskQueueLen    1
os_event_t    user_procTaskQueue[user_procTaskQueueLen];
static void user_procTask(os_event_t *events);

static volatile os_timer_t some_timer;

int intensity = 0;

void some_timerfunc(void *arg)
{
    intensity = (intensity==4) ? 0 : (intensity+1);
    os_printf("Intensity=%d\n\r",intensity);

    pwm_set_duty ((4-intensity)*25, 0);
    pwm_start();
/*---	
	pwm_set_duty(22222,0);
	if (intensity==0) {
        gpio_output_set(BIT2, 0, BIT2, 0);
	} else if (intensity==4) {
        gpio_output_set(0, BIT2, BIT2, 0);
	} else {
	    pwm_set_duty ((4-intensity)*5555, 0);
	    pwm_start();
	}
---*/
		
    //Do blinky stuff
/*----
    if (GPIO_REG_READ(GPIO_OUT_ADDRESS) & BIT2)
    {
        //Set GPIO2 to LOW
        gpio_output_set(0, BIT2, BIT2, 0);
		os_printf("LOW\n");
	}
    else
    {
        //Set GPIO2 to HIGH
        gpio_output_set(BIT2, 0, BIT2, 0);
		os_printf("HIGH\n");
    }
----*/
}

void init_done_cb (void) 
{
	os_printf("SDK version: %s\n",system_get_sdk_version());
    // Initialize the GPIO subsystem.
    gpio_init();

    //Set GPIO2 to output mode
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U, FUNC_GPIO2);

	// Init PWM
	uint32 pwm_duty = 0;
	uint32 pwm_io[][3] = {{PERIPHS_IO_MUX_GPIO2_U, FUNC_GPIO2, 2}};
	pwm_init(100, &pwm_duty, 1, pwm_io);
//	pwm_start();

    //Set GPIO2 low
    gpio_output_set(0, BIT2, BIT2, 0);

    //Disarm timer
    os_timer_disarm(&some_timer);

    //Setup timer
    os_timer_setfn(&some_timer, (os_timer_func_t *)some_timerfunc, NULL);

    //Arm the timer
    //&some_timer is the pointer
    //1000 is the fire time in ms
    //0 for once and 1 for repeating
    os_timer_arm(&some_timer, 1000, 1);
    
}

//Do nothing function
static void ICACHE_FLASH_ATTR
user_procTask(os_event_t *events)
{
    os_delay_us(10);
}

void user_rf_pre_init(void) {
}

//Init function 
void ICACHE_FLASH_ATTR
user_init()
{
	//Set USB baud rate
    uart_div_modify( 0, UART_CLK_FREQ / ( 115200 ) );
	
    //Turn off WIFI
	wifi_set_sleep_type(MODEM_SLEEP_T);
	
	system_init_done_cb(init_done_cb);

    //Start os task
//    system_os_task(user_procTask, user_procTaskPrio,user_procTaskQueue, user_procTaskQueueLen);
}
