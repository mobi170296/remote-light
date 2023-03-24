#include <stdio.h>
#include <osapi.h>
#include <user_interface.h>
#include <espconn.h>
#include <spi_flash.h>
#include <mem.h>
#include <sntp.h>
#include <time.h>

#define HTTP_HEADER "HTTP/1.1 200 OK\r\nServer: MG\r\nContent-Type: application/json\r\nConnection: close\r\n\r\n%s"
#define HTTP_HEADER_HOME "HTTP/1.1 200 OK\r\nServer: MG\r\nConnection: close\r\n\r\n"
#define FLASH_START 0x200000
#define FLASH_SIZE ((PAGE_LENGTH/4 + 1)*4)

#define RELAY_PIN 5
// #define RELAY_PIN 2
#define BUTTON_PIN 4

#define PINNAME_GPIO2 PERIPHS_IO_MUX_GPIO2_U
#define PINNAME_GPIO4 PERIPHS_IO_MUX_GPIO4_U
#define PINNAME_GPIO5 PERIPHS_IO_MUX_GPIO5_U

#define SSID "Light Controller"
#define PASSWORD "cucdangdexuong"

#define CLIENT_SSID "Here"
#define CLIENT_PASSWORD "Tvl@0795759696"

#define SET_PIN_STATE(pin, state) (GPIO_OUTPUT_SET(pin, state))
#define GET_PIN_STATE(pin) (!GPIO_INPUT_GET(pin))

#define RELAY1_PIN 5
#define RELAY2_PIN 4
#define RELAY_OFF 1
#define RELAY_ON 0

struct softap_config wifi_config;
struct station_config client_config;
struct ip_info wifi_ip;
struct dhcps_lease dhcps_ip_range;

os_timer_t sntp_timer;

struct espconn tcp_connection;
esp_tcp tcp_esp;

// os_timer_t polling_button_timer;
// int last_btn_status;

void memmovetail(void *head, unsigned int head_length, unsigned int total_length)
{
    char *total_tail = (char*)head + total_length - 1;
    char *head_tail = (char*)head + head_length - 1;
    unsigned int o = 0;
    for (; o < head_length; ++o)
    {
        *(total_tail - o) = *(head_tail - o);
    }
}

// void relay_off()
// {
//     GPIO_OUTPUT_SET(RELAY_PIN, 0);
// }

// void relay_on()
// {
//     GPIO_OUTPUT_SET(RELAY_PIN, 1);
// }

// void relay_toggle()
// {
//     GPIO_OUTPUT_SET(RELAY_PIN, !GPIO_INPUT_GET(RELAY_PIN));
// }

// bool relay_state()
// {
//     return GPIO_INPUT_GET(RELAY_PIN);
// }

// void polling_button_callback(void *arg)
// {
//     int btn_status = GPIO_INPUT_GET(BUTTON_PIN);
//     if (btn_status != last_btn_status)
//     {
//         last_btn_status = btn_status;
//         GPIO_OUTPUT_SET(RELAY_PIN, !GPIO_INPUT_GET(RELAY_PIN));
//     }
// }

void setup_io()
{
    ETS_GPIO_INTR_DISABLE();
    
    PIN_FUNC_SELECT(PINNAME_GPIO2, FUNC_GPIO2);
    PIN_FUNC_SELECT(PINNAME_GPIO4, FUNC_GPIO4);
    PIN_FUNC_SELECT(PINNAME_GPIO5, FUNC_GPIO5);

    SET_PIN_STATE(RELAY1_PIN, RELAY_OFF);
    SET_PIN_STATE(RELAY2_PIN, RELAY_OFF);
    // GPIO_DIS_OUTPUT(BUTTON_PIN);
    // PIN_PULLUP_DIS(PINNAME_GPIO4);
    // last_btn_status = GPIO_INPUT_GET(BUTTON_PIN);

    // relay_off();

    // os_memset(&polling_button_timer, 0, sizeof polling_button_timer);
    // os_timer_setfn(&polling_button_timer, polling_button_callback, NULL);
    // os_timer_arm(&polling_button_timer, 100, 1);
}

void setup_wifi()
{
    wifi_station_set_hostname("Light Node");

    os_strcpy(client_config.ssid, CLIENT_SSID);
    os_strcpy(client_config.password, CLIENT_PASSWORD);

    wifi_station_set_config_current(&client_config);

    wifi_station_connect();

    struct ip_info client_address;
    IP4_ADDR(&client_address.ip, 192, 168, 1, 200);
    IP4_ADDR(&client_address.gw, 192, 168, 1, 1);
    IP4_ADDR(&client_address.netmask, 255, 255, 255, 0);

    wifi_set_ip_info(STATION_IF, &client_address);

    sntp_setservername(0, "time.nist.gov");
    sntp_init();
    
    // os_memset(&wifi_config, 0, sizeof wifi_config);
    // os_strcpy(wifi_config.ssid, SSID);
    // os_strcpy(wifi_config.password, PASSWORD);
    // wifi_config.ssid_len = os_strlen(SSID);
    // wifi_config.ssid_hidden = 0;
    // wifi_config.channel = 10;
    // wifi_config.beacon_interval = 100;
    // wifi_config.max_connection = 4;
    // wifi_config.authmode = AUTH_WPA_WPA2_PSK;
    // wifi_softap_set_config_current(&wifi_config);

    // wifi_softap_dhcps_stop();
    // os_memset(&wifi_ip, 0, sizeof wifi_ip);
    // IP4_ADDR(&wifi_ip.ip, 10, 10, 10, 10);
    // IP4_ADDR(&wifi_ip.netmask, 255, 255, 255, 0);
    // IP4_ADDR(&wifi_ip.gw, 10, 10, 10, 10);
    // wifi_set_ip_info(SOFTAP_IF, &wifi_ip);

    // os_memset(&dhcps_ip_range, 0, sizeof dhcps_ip_range);
    // dhcps_ip_range.enable = TRUE;
    // IP4_ADDR(&dhcps_ip_range.start_ip, 10, 10, 10, 100);
    // IP4_ADDR(&dhcps_ip_range.end_ip, 10, 10, 10, 200);
    // wifi_softap_set_dhcps_lease(&dhcps_ip_range);
    // wifi_softap_dhcps_start();
}

void tcp_disconnect_handler(void *arg)
{
    // TODO
    // os_printf("DISCONNECT CALLBACK\n");
}

void tcp_receive_handler(void *arg, char *data, unsigned short length)
{
    // os_printf("RECEIVE CALLBACK\n");
    struct espconn* connection = (struct espconn*)arg;
    char *method, *url, *version;
    char *ndata = (char*)os_malloc(length + 1);
    os_memcpy(ndata, data, length);
    ndata[length] = '\0';
    
    char *d = strstr(ndata, "\r\n");
    if (d)
    {
        d[0] = '\0';
        method = ndata;
        d = strstr(ndata, " ");
        if (d)
        {
            d[0] = '\0';
            url = d + 1;
            d = strstr(url + 1, " ");
            if (d)
            {
                d[0] = '\0';
                version = d + 1;

                //os_printf("URL: %s\n", url);
                if (os_strstr(url, "getrelaystatus"))
                {
                    char *json = (char*)os_malloc(100);
                    char *header = (char*)os_malloc(sizeof(HTTP_HEADER) + 100);
                    os_sprintf(json, "{\"1\": %d, \"2\": %d}", GET_PIN_STATE(RELAY1_PIN), GET_PIN_STATE(RELAY2_PIN));
                    os_sprintf(header, HTTP_HEADER, json);
                    espconn_send(connection, header, strlen(header));
                    os_free(header);
                    os_free(json);
                }
                else if (os_strstr(url, "onrelay?pin=1"))
                {
                    char *header = (char*)os_malloc(sizeof(HTTP_HEADER));
                    SET_PIN_STATE(RELAY1_PIN, RELAY_ON);
                    os_sprintf(header, HTTP_HEADER, "1");
                    espconn_send(connection, header, strlen(header));
                    os_free(header);
                }
                else if (os_strstr(url, "onrelay?pin=2"))
                {
                    char *header = (char*)os_malloc(sizeof(HTTP_HEADER));
                    SET_PIN_STATE(RELAY2_PIN, RELAY_ON);
                    os_sprintf(header, HTTP_HEADER, "1");
                    espconn_send(connection, header, strlen(header));
                    os_free(header);
                }
                else if (os_strstr(url, "offrelay?pin=1"))
                {
                    char *header = (char*)os_malloc(sizeof(HTTP_HEADER));
                    SET_PIN_STATE(RELAY1_PIN, RELAY_OFF);
                    os_sprintf(header, HTTP_HEADER, "1");
                    espconn_send(connection, header, strlen(header));
                    os_free(header);
                }
                else if (os_strstr(url, "offrelay?pin=2"))
                {
                    char *header = (char*)os_malloc(sizeof(HTTP_HEADER));
                    SET_PIN_STATE(RELAY2_PIN, RELAY_OFF);
                    os_sprintf(header, HTTP_HEADER, "1");
                    espconn_send(connection, header, strlen(header));
                    os_free(header);
                }
                else
                {
                    char *body = (char*)os_malloc(sizeof(HTTP_HEADER_HOME) - 1 + FLASH_SIZE);
                    if (body)
                    {
                        spi_flash_read(FLASH_START, (uint32_t *)(body), FLASH_SIZE);
                        memmovetail(body, FLASH_SIZE, sizeof(HTTP_HEADER_HOME) - 1 + FLASH_SIZE);
                        os_memcpy(body, HTTP_HEADER_HOME, sizeof(HTTP_HEADER_HOME) - 1);
                        espconn_send(connection, body, sizeof(HTTP_HEADER_HOME) - 1 + PAGE_LENGTH);
                        os_free(body);
                    }
                }
            }
            else
            {
                // URL missing
            }
        }
        else
        {
            // Method missing
        }
    }
    else
    {
        // Terminated line missing
    }

    os_free(ndata);
}

void tcp_sent_handler(void *arg)
{
    // os_printf("SENT CALLBACK\n");
    espconn_disconnect((struct espconn*)arg);
}

void tcp_connect_handler(void *arg)
{
    // os_printf("CONNECT CALLBACK\n");
    struct espconn *connection = (struct espconn*)arg;
    espconn_regist_disconcb(connection, tcp_disconnect_handler);
    espconn_regist_recvcb(connection, tcp_receive_handler);
    espconn_regist_sentcb(connection, tcp_sent_handler);
}

void setup_tcp_server()
{
    os_memset(&tcp_connection, 0, sizeof tcp_connection);
    os_memset(&tcp_esp, 0, sizeof tcp_esp);
    tcp_connection.type = ESPCONN_TCP;
    tcp_connection.state = ESPCONN_NONE;
    tcp_connection.proto.tcp = &tcp_esp;
    tcp_connection.proto.tcp->local_port = 80;

    espconn_regist_connectcb(&tcp_connection, tcp_connect_handler);

    espconn_accept(&tcp_connection);
}

void sync_sntp_callback(void *a)
{
    unsigned int timestamp = sntp_get_current_timestamp();
    if (timestamp)
    {
        timestamp -= 3600;
        struct tm *datetime = localtime((time_t*)&timestamp);
        if (datetime->tm_hour == 18 && datetime->tm_min >= 0 && datetime->tm_min <= 1)
        {
            SET_PIN_STATE(RELAY1_PIN, RELAY_ON);
            SET_PIN_STATE(RELAY2_PIN, RELAY_ON);
        }
    }
}

void app_init()
{
    uart_div_modify(0, UART_CLK_FREQ / 115200);
    setup_io();
    setup_wifi();
    setup_tcp_server();
    os_timer_setfn(&sntp_timer, sync_sntp_callback, NULL);
    os_timer_arm(&sntp_timer, 5000, true);
}

void user_init(void)
{
    // Update new MAC address
    unsigned char mac[] = {0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa};
    wifi_set_opmode_current(STATION_MODE);
    wifi_set_macaddr(STATION_IF, mac);
    wifi_station_set_reconnect_policy(true);
    wifi_station_dhcpc_stop();
    system_init_done_cb(app_init);
}
