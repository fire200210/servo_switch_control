#include "wifi_station.h"
#include "data_process.h"

static const char *TAG = "wifi station";

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group = NULL;
static int s_retry_num = 0;

extern Setting_Param_t Setting_Param;
extern Wifi_Info_t     Wifi_Info;
extern ConnectStatus_t ConnectStatus;
extern Servo_Info_t    Servo_Info;

static void wifi_data_process(void)
{
  if(Servo_Info.ButtonChange_flag == false && Wifi_Info.Rx_buffer[2] == 0xF1)
  {
    switch (Wifi_Info.Rx_buffer[0])
    {
      case 0x02: //change wifi SSID Password //add change flag
        switch (Wifi_Info.Rx_buffer[1])
        {
          case OPEN:
            Servo_Info.SwitchChange_flag = true;
            Servo_Info.ServoCtrl= OPEN;
            break;
          case CLOSE:
            Servo_Info.SwitchChange_flag = true;
            Servo_Info.ServoCtrl= CLOSE;
            break;
        }
        break;
      default:
        ESP_LOGE(TAG, "Error commend");
        break;
    }
  } else printf("button using !!!\n");
}

static void do_retransmit(const int sock)
{
  int len;

  do {
    len = recv(sock, Wifi_Info.Rx_buffer, sizeof(Wifi_Info.Rx_buffer) - 1, 0); // 當連接成功後會進入這個function 進行數據監聽，直到斷開連線
    if (len < 0)
    {
      ESP_LOGE(TAG, "Error occurred during receiving: errno %d", errno);
    } 
    else if (len == 0) 
    {
      ESP_LOGW(TAG, "Connection closed");
    } 
    else
    {
      Wifi_Info.Rx_buffer[len] = 0; // Null-terminate whatever is received and treat it like a string

      wifi_data_process();

      // send() can return less bytes than supplied length.
      // Walk-around for robust implementation.
      int to_write = len;
      while (to_write > 0)
      {
        Wifi_Info.WifiData_Statu = true;
        int written = send(sock, Wifi_Info.Rx_buffer + (len - to_write), to_write, 0);
        // printf("Rx_date_size:%u\n", to_write);

        if (written < 0)
        {
          ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
          // Failed to retransmit, giving up
          return;
        }
        to_write -= written;
      }
    }
  } while (len > 0);
}

void tcp_server_task(void *pvParameters)
{
  char addr_str[128];
  int addr_family = (int)pvParameters;
  int ip_protocol = 0;
  int keepAlive = 1;
  int keepIdle = KEEPALIVE_IDLE;
  int keepInterval = KEEPALIVE_INTERVAL;
  int keepCount = KEEPALIVE_COUNT;
  struct sockaddr_storage dest_addr;

#ifdef CONFIG_EXAMPLE_IPV4
  if (addr_family == AF_INET) 
  {
    struct sockaddr_in *dest_addr_ip4 = (struct sockaddr_in *)&dest_addr;
    dest_addr_ip4->sin_addr.s_addr = htonl(INADDR_ANY);
    dest_addr_ip4->sin_family = AF_INET;
    dest_addr_ip4->sin_port = htons(PORT);
    ip_protocol = IPPROTO_IP;
  }
#endif
#ifdef CONFIG_EXAMPLE_IPV6
  if (addr_family == AF_INET6)
  {
    struct sockaddr_in6 *dest_addr_ip6 = (struct sockaddr_in6 *)&dest_addr;
    bzero(&dest_addr_ip6->sin6_addr.un, sizeof(dest_addr_ip6->sin6_addr.un));
    dest_addr_ip6->sin6_family = AF_INET6;
    dest_addr_ip6->sin6_port = htons(PORT);
    ip_protocol = IPPROTO_IPV6;
  }
#endif

  int listen_sock = socket(addr_family, SOCK_STREAM, ip_protocol);
  if (listen_sock < 0)
  {
    ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
    vTaskDelete(NULL);
    return;
  }

  int opt = 1;
  setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

#if defined(CONFIG_EXAMPLE_IPV4) && defined(CONFIG_EXAMPLE_IPV6)
  // Note that by default IPV6 binds to both protocols, it is must be disabled
  // if both protocols used at the same time (used in CI)
  setsockopt(listen_sock, IPPROTO_IPV6, IPV6_V6ONLY, &opt, sizeof(opt));
#endif

  ESP_LOGI(TAG, "Socket created");

  int err = bind(listen_sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
  if (err != 0) 
  {
    ESP_LOGE(TAG, "Socket unable to bind: errno %d", errno);
    ESP_LOGE(TAG, "IPPROTO: %d", addr_family);
    goto CLEAN_UP;
  }
  ESP_LOGI(TAG, "Socket bound, port %d", PORT);

  err = listen(listen_sock, 1);
  if (err != 0) 
  {
    ESP_LOGE(TAG, "Error occurred during listen: errno %d", errno);
    goto CLEAN_UP;
  }

  while (1)
  {
    ESP_LOGI(TAG, "Socket listening");
    struct sockaddr_storage source_addr; // Large enough for both IPv4 or IPv6
    socklen_t addr_len = sizeof(source_addr);
    int sock = accept(listen_sock, (struct sockaddr *)&source_addr, &addr_len);
    if (sock < 0)
    {
      ESP_LOGE(TAG, "Unable to accept connection: errno %d", errno);
      break;
    }

    // Set tcp keepalive option
    setsockopt(sock, SOL_SOCKET,  SO_KEEPALIVE,  &keepAlive,    sizeof(int));
    setsockopt(sock, IPPROTO_TCP, TCP_KEEPIDLE,  &keepIdle,     sizeof(int));
    setsockopt(sock, IPPROTO_TCP, TCP_KEEPINTVL, &keepInterval, sizeof(int));
    setsockopt(sock, IPPROTO_TCP, TCP_KEEPCNT,   &keepCount,    sizeof(int));
    // Convert ip address to string
#ifdef CONFIG_EXAMPLE_IPV4
    if (source_addr.ss_family == PF_INET)
    {
      inet_ntoa_r(((struct sockaddr_in *)&source_addr)->sin_addr, addr_str, sizeof(addr_str) - 1);
    }
#endif
#ifdef CONFIG_EXAMPLE_IPV6
    if (source_addr.ss_family == PF_INET6)
    {
      inet6_ntoa_r(((struct sockaddr_in6 *)&source_addr)->sin6_addr, addr_str, sizeof(addr_str) - 1);
    }
#endif
    ESP_LOGE(TAG, "Socket accepted ip address: %s", addr_str);
    ConnectStatus.Wifi = LED_Open;

    do_retransmit(sock);

    shutdown(sock, 0);
    close(sock);
    ConnectStatus.Wifi = LED_Close;
  }

CLEAN_UP:
  close(listen_sock);
  vTaskDelete(NULL);
}

static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
  if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) 
  {
      esp_wifi_connect();
  } 
  else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) 
  {
    if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY) 
    {
      esp_wifi_connect();
      s_retry_num++;
      ESP_LOGI(TAG, "retry to connect to the AP");
    }
    else 
    {
      xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
    }
    ESP_LOGI(TAG,"connect to the AP fail");
  }
  else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
  {
    ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
    ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
    s_retry_num = 0;
    xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
  }
}

void wifi_init_sta(void)
{
  ESP_LOGI(TAG, "ESP_WIFI_MODE_STA");
  
  s_wifi_event_group = xEventGroupCreate();

  ESP_ERROR_CHECK(esp_netif_init());

  ESP_ERROR_CHECK(esp_event_loop_create_default());
  esp_netif_create_default_wifi_sta();

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));

  esp_event_handler_instance_t instance_any_id;
  esp_event_handler_instance_t instance_got_ip;
  ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                      ESP_EVENT_ANY_ID,
                                                      &event_handler,
                                                      NULL,
                                                      &instance_any_id));
  ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                      IP_EVENT_STA_GOT_IP,
                                                      &event_handler,
                                                      NULL,
                                                      &instance_got_ip));

  wifi_config_t wifi_config = {
    .sta = {
      .ssid = EXAMPLE_ESP_WIFI_SSID,
      .password = EXAMPLE_ESP_WIFI_PASS,
      /* Authmode threshold resets to WPA2 as default if password matches WPA2 standards (password len => 8).
        * If you want to connect the device to deprecated WEP/WPA networks, Please set the threshold value
        * to WIFI_AUTH_WEP/WIFI_AUTH_WPA_PSK and set the password with length and format matching to
        * WIFI_AUTH_WEP/WIFI_AUTH_WPA_PSK standards.
        */
      .threshold.authmode = ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD,
      .sae_pwe_h2e = ESP_WIFI_SAE_MODE,
      .sae_h2e_identifier = EXAMPLE_H2E_IDENTIFIER,
    },
  };

  // snprintf((char *)wifi_config.sta.ssid, 32, "%s", Setting_Param.Wifi_ssid);
  // snprintf((char *)wifi_config.sta.password, 64, "%s", Setting_Param.Wifi_password);

  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
  ESP_ERROR_CHECK(esp_wifi_start());

  ESP_LOGI(TAG, "wifi_init_sta finished.");

  /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
    * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
  EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                          WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                          pdFALSE,
                                          pdFALSE,
                                          portMAX_DELAY);

  /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
    * happened. */
  if (bits & WIFI_CONNECTED_BIT) ESP_LOGI(TAG, "connected to ap SSID:%s password:%s", wifi_config.sta.ssid, wifi_config.sta.password);
  else if (bits & WIFI_FAIL_BIT) ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s", wifi_config.sta.ssid, wifi_config.sta.password);
  else                           ESP_LOGE(TAG, "UNEXPECTED EVENT");
}