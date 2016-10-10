#include <ets_sys.h>
#include <osapi.h>
#include <gpio.h>
#include <os_type.h>
#include <pwm.h>
#include <user_interface.h>
#include "user_config.h"

void wifi_event_handler (System_Event_t *evt) 
{
	os_printf("event %x\n", evt->event);
    switch (evt->event) {
	    case EVENT_STAMODE_CONNECTED:
	        os_printf("connect to ssid %s, channel %d\n",
	            evt->event_info.connected.ssid,
	            evt->event_info.connected.channel);
	        break;
	    case EVENT_STAMODE_DISCONNECTED:
	        os_printf("disconnect from ssid %s, reason %d\n",
	            evt->event_info.disconnected.ssid,
	            evt->event_info.disconnected.reason);
	        break;
	    case EVENT_STAMODE_AUTHMODE_CHANGE:
	        os_printf("mode: %d -> %d\n",
	            evt->event_info.auth_change.old_mode,
	            evt->event_info.auth_change.new_mode);
	        break;
	    case EVENT_STAMODE_GOT_IP:
            os_printf("ip:" IPSTR ",mask:" IPSTR ",gw:" IPSTR,
	            IP2STR(&evt->event_info.got_ip.ip),
	            IP2STR(&evt->event_info.got_ip.mask),
	            IP2STR(&evt->event_info.got_ip.gw));
	        os_printf("\n");
	        break;
	    case EVENT_SOFTAPMODE_STACONNECTED:
	        os_printf("station: " MACSTR "join, AID = %d\n",
                MAC2STR(evt->event_info.sta_connected.mac),	
				evt->event_info.sta_connected.aid);
			break;
		case EVENT_SOFTAPMODE_STADISCONNECTED:
		    os_printf("station: " MACSTR "leave, AID = %d\n",
	        MAC2STR(evt->event_info.sta_disconnected.mac),
			    evt->event_info.sta_disconnected.aid);
		    break;
		default:
			break;
	}
}

void init_done_cb (void) 
{
	os_printf("SDK version: %s\n",system_get_sdk_version());
#ifdef AP
char hostname[] = "MCANDLE_AP";
	// Define our mode as an Access Point
	wifi_set_opmode_current(SOFTAP_MODE);
	// Build our Access Point configuration details
	struct softap_config config;
	os_strcpy(config.ssid, SSID);
	os_strcpy(config.password, PASSWORD);
	config.ssid_len = 0;
	config.authmode = AUTH_WPA2_PSK;
	config.ssid_hidden = 0;
	config.max_connection = 4;
	wifi_softap_set_config_current(&config);
#else
char hostname[] = "MCANDLE_ST";
	wifi_set_opmode_current(STATION_MODE);
	wifi_station_set_hostname(hostname);
	struct station_config config;
	strncpy(config.ssid, SSID, 32);
	strncpy(config.password, PASSWORD, 64);
	wifi_station_set_config(&config);
	wifi_set_event_handler_cb(wifi_event_handler);
	wifi_station_connect();
#endif
}

void user_rf_pre_init(void) {
}

//Init function 
void ICACHE_FLASH_ATTR
user_init()
{
	// Set USB baud rate
    uart_div_modify( 0, UART_CLK_FREQ / ( 115200 ) );
	// Turn on WIFI
	wifi_set_sleep_type(NONE_SLEEP_T);
	// Turn off WIFI auto connect 
	wifi_station_set_auto_connect(0);
	// Set Callback function after system init
	system_init_done_cb(init_done_cb);
}
