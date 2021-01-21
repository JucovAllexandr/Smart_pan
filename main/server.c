#include "server.h"
#include <string.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_http_server.h"
#include "perc_converter.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "nvs_def.h"

#define TAG "SERVER"

#define ESP_WIFI_SSID "Smart Pan"
#define ESP_WIFI_PASS ""
#define MAX_STA_CONN 1

esp_err_t main_get_handler(httpd_req_t *req);
esp_err_t web_cfg_post_handler(httpd_req_t *req);

static httpd_handle_t handle;
static const httpd_uri_t uri_get = {
    .uri = "/",
    .method = HTTP_GET,
    .handler = main_get_handler,
    .user_ctx  = NULL
};
static const httpd_uri_t uri_post = {
    .uri = "/",
    .method = HTTP_POST,
    .handler = web_cfg_post_handler,
    .user_ctx  = NULL
};

void server_init()
{
    perc_init();

    tcpip_adapter_init();
    
    tcpip_adapter_ip_info_t ipInfo;
    IP4_ADDR(&ipInfo.ip, 192,168,0,1);
	IP4_ADDR(&ipInfo.gw, 192,168,0,1);
	IP4_ADDR(&ipInfo.netmask, 255,255,255,0);
	tcpip_adapter_dhcps_stop(TCPIP_ADAPTER_IF_AP);
	tcpip_adapter_set_ip_info(TCPIP_ADAPTER_IF_AP, &ipInfo);
	tcpip_adapter_dhcps_start(TCPIP_ADAPTER_IF_AP);

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = ESP_WIFI_SSID,
            .ssid_len = strlen(ESP_WIFI_SSID),
            .password = ESP_WIFI_PASS,
            .max_connection = MAX_STA_CONN,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK
        }
    };

    if (strlen(ESP_WIFI_PASS) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_softap finished. SSID:%s password:%s",
             ESP_WIFI_SSID, ESP_WIFI_PASS);

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.stack_size = 6144;
    if (httpd_start(&handle, &config) == ESP_OK)
    {
        httpd_register_uri_handler(handle, &uri_get);
        httpd_register_uri_handler(handle, &uri_post);
    }
}

esp_err_t main_get_handler(httpd_req_t *req)
{    
    httpd_resp_send(req, web_page, sizeof(web_page));
    return ESP_OK;
}

esp_err_t web_cfg_post_handler(httpd_req_t *req)
{
    char content[512];
    memset(content, 0, sizeof(content));
    int ret = httpd_req_recv(req, content, sizeof(content));

    if (ret <= 0) { 
        if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
            httpd_resp_send_408(req);
        }

        return ESP_FAIL;
    }

    printf("%s\n", content);
    char ssid_buf[32] = {0};
    char pass_buf[32] = {0};
    if(httpd_query_key_value(content, WEB_NAME_WIFI_SSID, ssid_buf, sizeof(ssid_buf)) == ESP_OK && 
            httpd_query_key_value(content, WEB_NAME_WIFI_PASS, pass_buf, sizeof(pass_buf)) == ESP_OK)
    {
        char *ssid = perc_convert(ssid_buf, sizeof(ssid_buf));
        char *pass = perc_convert(pass_buf, sizeof(pass_buf));
        printf("Wi-Fi save ssid '%s', pass '%s'\n", ssid, pass);
        nvs_handle hnd;
        nvs_open(NVS_DEF_NAME, NVS_READWRITE, &hnd);
        nvs_set_str(hnd, NVS_DEF_WIFI_SSID, ssid);
        nvs_set_str(hnd, NVS_DEF_WIFI_PASS, pass);
        nvs_set_u8(hnd, NVS_DEF_CONFIG_MODE, MODE_NORMAL);
        nvs_commit(hnd);
        nvs_close(hnd);

        esp_restart();
    }
    return ESP_OK;
}