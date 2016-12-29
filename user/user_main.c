#include "ets_sys.h"
#include "osapi.h"
#include "user_interface.h"
#include "espconn.h"
 
LOCAL struct espconn esp_conn;
LOCAL esp_tcp esptcp;
 
#define SERVER_LOCAL_PORT   1112
int counter = 0;
 
/******************************************************************************
 * FunctionName : tcp_server_sent_cb
 * Description  : data sent callback.
 * Parameters   : arg -- Additional argument to pass to the callback function
 * Returns      : none
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
tcp_server_sent_cb(void *arg)
{
   //data sent successfully
 
    os_printf("tcp sent cb \r\n");
	
}
 
 
/******************************************************************************
 * FunctionName : tcp_server_recv_cb
 * Description  : receive callback.
 * Parameters   : arg -- Additional argument to pass to the callback function
 * Returns      : none
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
tcp_server_recv_cb(void *arg, char *pusrdata, unsigned short length)
{
   //received some data from tcp connection
    
   struct espconn *pespconn = arg;
   os_printf("tcp recv : %s \r\n", pusrdata);
    
   char pbuf[1000];
   os_sprintf(pbuf,
     "HTTP/1.0 200 OK\n\rServer: ESPsrv\n\rContent-Type: text/html\n\r\n\r<!DOCTYPE HTML>\n\r"
	 "<html>\n\r<h1>ESP8266 Web Server</h1>\n\r%d"
     "<form>Length = <input name=\"length\"/>\r\nRED = <input name=\"red\"/>\r\nGREEN = <input name=\"green\"/>\r\nBLUE = <input name=\"blue\"/>\r\n"
	 "<input type=\"Submit\" value=\"Set\"/></form>\n\r"
	 "<form><br>Relay state:<br/>New State:<input type=\"Submit\" name=\"relay\" Value=\"A\"/><input type=\"Submit\" name=\"relay\" Value=\"B\"/></form>"
	 "</html>\n",++counter);
   espconn_sent(pespconn, pbuf, os_strlen(pbuf));
   espconn_disconnect(pespconn);    
}
 
/******************************************************************************
 * FunctionName : tcp_server_discon_cb
 * Description  : disconnect callback.
 * Parameters   : arg -- Additional argument to pass to the callback function
 * Returns      : none
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
tcp_server_discon_cb(void *arg)
{
   //tcp disconnect successfully
    
    os_printf("tcp disconnect succeed !!! \r\n");
}
 
/******************************************************************************
 * FunctionName : tcp_server_recon_cb
 * Description  : reconnect callback, error occured in TCP connection.
 * Parameters   : arg -- Additional argument to pass to the callback function
 * Returns      : none
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
tcp_server_recon_cb(void *arg, sint8 err)
{
   //error occured , tcp connection broke. 
    
    os_printf("reconnect callback, error code %d !!! \r\n",err);
}
 
LOCAL void tcp_server_multi_send(void)
{
   struct espconn *pesp_conn = &esp_conn;
 
   remot_info *premot = NULL;
   uint8 count = 0;
   sint8 value = ESPCONN_OK;
   if (espconn_get_connection_info(pesp_conn,&premot,0) == ESPCONN_OK){
 	  os_printf("link_cnt=%d\n\r",pesp_conn->link_cnt);
//      char pbuf[1000];
//	  os_sprintf(pbuf,
//	     "HTTP/1.0 200 OK\r\nServer: ESPsrv\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HT// ML>\r\n"
//		 "<html>\r\n<h1>ESP8266 Web Server</h1>\r\n%d"
//	     "<form>LED period: <input name=\"period\"/>msec<input type=\"Submit\" value=\"Set\"/></form>"
//		 "<form>Relay state:<br/>New State:<input type=\"Submit\" name=\"relay\" Value=\"A\"/><input type=\"Submit\" name=\"relay\" Value=\"B\"/></form>"
//		 "</html>\n",++counter);
//      for (count = 0; count < pesp_conn->link_cnt; count ++){
//         pesp_conn->proto.tcp->remote_port = premot[count].remote_port;          
//         pesp_conn->proto.tcp->remote_ip[0] = premot[count].remote_ip[0];
//         pesp_conn->proto.tcp->remote_ip[1] = premot[count].remote_ip[1];
//         pesp_conn->proto.tcp->remote_ip[2] = premot[count].remote_ip[2];
//         pesp_conn->proto.tcp->remote_ip[3] = premot[count].remote_ip[3];
 
//         espconn_sent(pesp_conn, pbuf, os_strlen(pbuf));
//		 espconn_disconnect(pesp_conn);
//      }
   }
}
 
 
/******************************************************************************
 * FunctionName : tcp_server_listen
 * Description  : TCP server listened a connection successfully
 * Parameters   : arg -- Additional argument to pass to the callback function
 * Returns      : none
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
tcp_server_listen(void *arg)
{
    struct espconn *pesp_conn = arg;
    os_printf("tcp_server_listen !!! \r\n");
 
    espconn_regist_recvcb(pesp_conn, tcp_server_recv_cb);
    espconn_regist_reconcb(pesp_conn, tcp_server_recon_cb);
    espconn_regist_disconcb(pesp_conn, tcp_server_discon_cb);
     
    espconn_regist_sentcb(pesp_conn, tcp_server_sent_cb);
   tcp_server_multi_send();
}
 
/******************************************************************************
 * FunctionName : user_tcpserver_init
 * Description  : parameter initialize as a TCP server
 * Parameters   : port -- server port
 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR
user_tcpserver_init(uint32 port)
{
    esp_conn.type = ESPCONN_TCP;
    esp_conn.state = ESPCONN_NONE;
    esp_conn.proto.tcp = &esptcp;
    esp_conn.proto.tcp->local_port = port;
    espconn_regist_connectcb(&esp_conn, tcp_server_listen);
 
    sint8 ret = espconn_accept(&esp_conn);
     
    os_printf("espconn_accept [%d] !!! \r\n", ret);
 
}
LOCAL os_timer_t test_timer;
 
/******************************************************************************
 * FunctionName : user_esp_platform_check_ip
 * Description  : check whether get ip addr or not
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR
user_esp_platform_check_ip(void)
{
    struct ip_info ipconfig;
 
   //disarm timer first
    os_timer_disarm(&test_timer);
 
   //get ip info of ESP8266 station
    wifi_get_ip_info(STATION_IF, &ipconfig);
 
    if (wifi_station_get_connect_status() == STATION_GOT_IP && ipconfig.ip.addr != 0) {
 
      os_printf("got ip !!! \r\n");
      user_tcpserver_init(SERVER_LOCAL_PORT);
 
    } else {
        
        if ((wifi_station_get_connect_status() == STATION_WRONG_PASSWORD ||
                wifi_station_get_connect_status() == STATION_NO_AP_FOUND ||
                wifi_station_get_connect_status() == STATION_CONNECT_FAIL)) {
                 
         os_printf("connect fail !!! \r\n");
          
        } else {
           //re-arm timer to check ip
            os_timer_setfn(&test_timer, (os_timer_func_t *)user_esp_platform_check_ip, NULL);
            os_timer_arm(&test_timer, 100, 0);
        }
    }
}
 
/******************************************************************************
 * FunctionName : user_set_station_config
 * Description  : set the router info which ESP8266 station will connect to 
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR
user_set_station_config(void)
{
   // Wifi configuration 
   char ssid[32] = SSID; 
   char password[64] = PASSWORD; 
   struct station_config stationConf; 
 
   //Set  station mode 
   //wifi_set_opmode(STATIONAP_MODE); 
   wifi_set_opmode_current(STATION_MODE); 
   wifi_station_set_hostname("ESP");

   //need not mac address
   stationConf.bssid_set = 0; 
    
   //Set ap settings 
   os_memcpy(&stationConf.ssid, ssid, 32); 
   os_memcpy(&stationConf.password, password, 64); 
   wifi_station_set_config(&stationConf); 
   wifi_station_connect();
 
   //set a timer to check whether got ip from router succeed or not.
   os_timer_disarm(&test_timer);
   os_timer_setfn(&test_timer, (os_timer_func_t *)user_esp_platform_check_ip, NULL);
   os_timer_arm(&test_timer, 100, 0);
 
}
 
void user_rf_pre_init(void) 
{
}

/******************************************************************************
 * FunctionName : user_init
 * Description  : entry of user application, init user function here
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
void user_init(void)
{
	// Set USB baud rate
    uart_div_modify( 0, UART_CLK_FREQ / ( 115200 ) );
    os_printf("SDK version:%s\n", system_get_sdk_version());
    
	system_init_done_cb(user_set_station_config);

   //Set  station mode 
   //wifi_set_opmode(STATIONAP_MODE); 
   //--wifi_set_opmode_current(STATION_MODE); 
   //--wifi_station_set_hostname("ESP");
 
   // ESP8266 connect to router.
   //--user_set_station_config();
    
}