#include <ets_sys.h>
#include <osapi.h>
#include <gpio.h>
#include <os_type.h>
#include <ip_addr.h>
#include <espconn.h>
#include <user_interface.h>
#include "user_config.h"

void blink_led (int count);
void init_done_cb (void);
void wifi_event_cb (System_Event_t *evt);
void sent_cb (void *arg);
void received_cb (void *arg, char *pdata, unsigned short len);
void timer_cb (void *arg);

// Need to define these as global variables so that they exist for the Callback functions
struct espconn connsend;
esp_udp udpsend;
struct espconn connrecv;
esp_udp udprecv;
os_timer_t tmr_1ms;
bool sync=false;
unsigned int sync_counter=0;
unsigned int action_counter=0;
bool action;

void init_done_cb (void) 
{
	// Initialisation is done. System is ready.
	os_printf("SDK version: %s\n",system_get_sdk_version());

	// Setup WIFI
#ifdef MASTER
	char hostname[] = "MASTER";
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
	wifi_set_event_handler_cb(wifi_event_cb);
#else
	char hostname[] = "STATION";
	wifi_set_opmode_current(STATION_MODE);
	wifi_station_set_hostname(hostname);
	struct station_config config;
	strncpy(config.ssid, SSID, 32);
	strncpy(config.password, PASSWORD, 64);
	wifi_station_set_config(&config);
	wifi_station_connect();
	wifi_set_event_handler_cb(wifi_event_cb);
#endif

    // Setup GPIO2 to output mode
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U, FUNC_GPIO2);

	blink_led(1);
}

void wifi_event_cb (System_Event_t *evt) 
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
			{
#ifdef MASTER
#else
				// Prepare for sending UDP message
				connsend.type = ESPCONN_UDP;
				connsend.state = ESPCONN_NONE;
				connsend.proto.udp = &udpsend;
				espconn_regist_sentcb(&connsend, sent_cb);

				// Wait at UDP port
				connrecv.type = ESPCONN_UDP;
				connrecv.state = ESPCONN_NONE;
				connrecv.proto.udp = &udprecv;
				udprecv.local_port = MCPORT;
				espconn_regist_recvcb(&connrecv, received_cb);
				espconn_create(&connrecv);
#endif
				// Set timer
			    os_timer_disarm(&tmr_1ms);
			    os_timer_setfn(&tmr_1ms, (os_timer_func_t *)timer_cb, NULL);
			    os_timer_arm(&tmr_1ms, 1, 1);
				//hw_timer_init(FRC1_SOURCE,1);
				//hw_timer_set_func(timer_cb);
				//hw_timer_arm(1000);
			}
			blink_led(2);
	        break;

	    case EVENT_SOFTAPMODE_STACONNECTED:
	        os_printf("station: " MACSTR " join, AID = %d\n",
                MAC2STR(evt->event_info.sta_connected.mac),	
				evt->event_info.sta_connected.aid);
#ifdef MASTER
				struct ip_info ip;
				wifi_get_ip_info(STATION_IF, &ip);
				//os_printf("IP:" IPSTR ", MASK:" IPSTR ", GATEWAY:" IPSTR "\n", IP2STR(&(ip.ip)),IP2STR(&(ip.netmask)),IP2STR(&(ip.gw)));
				uint32 bcast_ip = ip.ip.addr | ~(ip.netmask.addr);
				//os_printf("Broadcast IP:" IPSTR "\n", IP2STR(&bcast_ip));
	
				// Send a UDP message
				connsend.type = ESPCONN_UDP;
				connsend.state = ESPCONN_NONE;
				connsend.proto.udp = &udpsend;
				udpsend.remote_ip[3] = bcast_ip>>24;
				udpsend.remote_ip[2] = (bcast_ip>>16) & 0xFF;
				udpsend.remote_ip[1] = (bcast_ip>>8) & 0xFF;
				udpsend.remote_ip[0] = bcast_ip & 0xFF;
				udpsend.remote_port = MCPORT;
				espconn_regist_sentcb(&connsend, sent_cb);

				// Wait at UDP port
				connrecv.type = ESPCONN_UDP;
				connrecv.state = ESPCONN_NONE;
				connrecv.proto.udp = &udprecv;
				udprecv.local_port = MCPORT+1;
				espconn_regist_recvcb(&connrecv, received_cb);
				espconn_create(&connrecv);
				// Set timer
			    os_timer_disarm(&tmr_1ms);
			    os_timer_setfn(&tmr_1ms, (os_timer_func_t *)timer_cb, NULL);
			    os_timer_arm(&tmr_1ms, 1, 1);
				//hw_timer_init(FRC1_SOURCE,1);
				//hw_timer_set_func(timer_cb);
				//hw_timer_arm(1000);
#else
#endif
			break;

		case EVENT_SOFTAPMODE_STADISCONNECTED:
		    os_printf("station: " MACSTR " leave, AID = %d\n",
	        MAC2STR(evt->event_info.sta_disconnected.mac),
			    evt->event_info.sta_disconnected.aid);
		    break;

		default:
			break;
	}
}

void sent_cb (void *arg)
{
//	struct espconn *conn = arg;
//	esp_udp *udp = conn->proto.udp;
//	os_printf("Sent [Status=%d]\n",conn->state);
//	os_printf("Local  " IPSTR ":%d\n",IP2STR(udp->local_ip),udp->local_port);
//	os_printf("Remote " IPSTR ":%d\n",IP2STR(udp->remote_ip),udp->remote_port);

//	os_printf("SENT - %10d\n",sync_counter);

}

void received_cb (void *arg, char *pdata, unsigned short len) 
{
//	struct espconn *conn = arg;
//	esp_udp *udp = conn->proto.udp;
//	os_printf("Received [Status=%d]\n",conn->state);
//	os_printf("Local  IP:" IPSTR ", Port:%d\n",udp->local_ip,udp->local_port);
//	os_printf("Remote IP:" IPSTR ", Port:%d\n",udp->remote_ip,udp->remote_port);
	
//	int i;
//	for (i=0; i<len; i++)
//		os_printf("%c",pdata[i]);
//	os_printf("\n");
	
#ifdef MASTER
	os_printf("%10d %s\n",sync_counter,pdata);
#else
	if (!sync) {
//		sync_counter = 0;
		sync = true;
	} else {
		struct espconn *conn = (struct espconn *)arg;
		int i = atoi(pdata);
		udpsend.remote_ip[3] = conn->proto.udp->remote_ip[3];
		udpsend.remote_ip[2] = conn->proto.udp->remote_ip[2];
		udpsend.remote_ip[1] = conn->proto.udp->remote_ip[1];
		udpsend.remote_ip[0] = conn->proto.udp->remote_ip[0];
		udpsend.remote_port = MCPORT + 1;
		espconn_create(&connsend);
		espconn_sent(&connsend,pdata,len);
		espconn_delete(&connsend);
		os_printf("%10d %10d\n",i,sync_counter);
	    gpio_output_set(0, BIT2, BIT2, 0);
		action_counter = sync_counter + 50;
		action = false;
//		blink_led(1);
	}
#endif
	//blink_led(1);
}

void timer_cb (void *arg)
{
	if ((sync_counter%1000)==0) {
#ifdef MASTER
		// Send message to Station
		char buffer[10];
		os_sprintf(buffer,"%9d\0",sync_counter);
		espconn_create(&connsend);
		espconn_sent(&connsend,buffer,10);
		espconn_delete(&connsend);
	    gpio_output_set(0, BIT2, BIT2, 0);
		action_counter = sync_counter + 50;
		action = false;
//		blink_led(1);
#endif
//		blink_led(1);
	}
	if (sync_counter==action_counter) {
		if (action)
		    gpio_output_set(0, BIT2, BIT2, 0);
		else
		    gpio_output_set(BIT2, 0, BIT2, 0);
	}
	sync_counter++;
}

void user_rf_pre_init(void) 
{
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

void blink_led (int count) {
	while (count>0) {
	    gpio_output_set(0, BIT2, BIT2, 0);
		os_delay_us(50000);
	    gpio_output_set(BIT2, 0, BIT2, 0);
		if (--count>0) {
			os_delay_us(50000);
		}
	}
}


