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
void recd_websrv_cb (void *arg, char *pdata, unsigned short len);

// Need to define these as global variables so that they exist for the Callback functions
struct espconn connwebsrv;
esp_tcp tcpwebsrv;
struct espconn connsend;
esp_tcp tcpsend;

int count = 0;

void init_done_cb (void) 
{
	// Initialisation is done. System is ready.
	os_printf("SDK version: %s\n",system_get_sdk_version());

	// Setup WIFI
	char hostname[] = "ESP";
	wifi_set_opmode_current(STATION_MODE);
	wifi_station_set_hostname(hostname);
	struct station_config config;
	strncpy(config.ssid, SSID, 32);
	strncpy(config.password, PASSWORD, 64);
	wifi_station_set_config(&config);
	wifi_station_connect();
	wifi_set_event_handler_cb(wifi_event_cb);

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
			
			// Start Web server
			connwebsrv.type = ESPCONN_TCP;
			connwebsrv.state = ESPCONN_NONE;
			connwebsrv.proto.tcp = &tcpwebsrv;
			tcpwebsrv.local_port = 80;
			espconn_regist_recvcb(&connwebsrv, recd_websrv_cb);
			espconn_accept(&connwebsrv);
			
	        break;

	    case EVENT_SOFTAPMODE_STACONNECTED:
	        os_printf("station: " MACSTR " join, AID = %d\n",
                MAC2STR(evt->event_info.sta_connected.mac),	
				evt->event_info.sta_connected.aid);
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
	struct espconn *conn = arg;
	esp_udp *udp = conn->proto.udp;
	os_printf("Sent [Status=%d]\n",conn->state);
	os_printf("Local  " IPSTR ":%d\n",IP2STR(udp->local_ip),udp->local_port);
	os_printf("Remote " IPSTR ":%d\n",IP2STR(udp->remote_ip),udp->remote_port);
}

void received_cb (void *arg, char *pdata, unsigned short len) 
{
	struct espconn *conn = arg;
	esp_udp *udp = conn->proto.udp;
	os_printf("Received [Status=%d]\n",conn->state);
	os_printf("Local  IP:" IPSTR ", Port:%d\n",IP2STR(udp->local_ip),udp->local_port);
	os_printf("Remote IP:" IPSTR ", Port:%d\n",IP2STR(udp->remote_ip),udp->remote_port);
	os_printf("length = %d\n",len);
	
	int i;
	for (i=0; i<len; i++)
		os_printf("%c",pdata[i]);
	os_printf("\n");
}

// Callback function for Web server
void recd_websrv_cb (void *arg, char *pdata, unsigned short len) 
{
	struct espconn *conn = arg;
	esp_tcp *tcp = conn->proto.tcp;
	os_printf("Received %s [Status=%d]\n",((conn->type==ESPCONN_TCP)?"TCP":"UDP"),conn->state);
	os_printf("Local  IP:" IPSTR ", Port:%d\n",IP2STR(tcp->local_ip),tcp->local_port);
	os_printf("Remote IP:" IPSTR ", Port:%d\n",IP2STR(tcp->remote_ip),tcp->remote_port);
	os_printf("length = %d\n",len);
	
	int i;
	for (i=0; i<len; i++)
		os_printf("%c",pdata[i]);
	os_printf("\n");
	
	// Respond with default web page
	if ((len>14) && strncmp(pdata,"GET / HTTP/1.1",14)==0) {
		os_printf("Respond webpage ... %d\n", ++count);
		char buffer[1000];
		int len;
		os_sprintf(buffer,"HTTP/1.0 200 OK\r\nServer: ESPsrv\r\nContent-Type: text/html\r\n\r\n<h1>ESP8266 Web Server</h1>\r\n%d",count);
		len = strlen(buffer);
		struct espconn *conn = (struct espconn *)arg;
		connsend.type = ESPCONN_TCP;
		connsend.state = ESPCONN_NONE;
		connsend.proto.tcp = &tcpsend;
		tcpsend.remote_ip[3] = conn->proto.tcp->remote_ip[3];
		tcpsend.remote_ip[2] = conn->proto.tcp->remote_ip[2];
		tcpsend.remote_ip[1] = conn->proto.tcp->remote_ip[1];
		tcpsend.remote_ip[0] = conn->proto.tcp->remote_ip[0];
		tcpsend.remote_port = conn->proto.tcp->remote_port;
		//espconn_regist_sentcb(&connsend, sent_cb);
		espconn_sent(&connsend,buffer,len);
		//espconn_delete(&connsend);
		os_printf(" ... sending\n");
	}
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


