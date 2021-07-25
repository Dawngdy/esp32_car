/* BSD Socket API Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "tcpip_adapter.h"
#include "protocol_examples_common.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>
#include "driver/gpio.h"

#ifdef CONFIG_EXAMPLE_IPV4
//#define HOST_IP_ADDR CONFIG_EXAMPLE_IPV4_ADDR
#define HOST_IP_ADDR "192.168.2.100"   //服务器IP
//#define HOST_IP_ADDR "192.168.43.1"
//电机控制引脚
#define IN1_GPIO 26
#define IN2_GPIO 25
#define IN3_GPIO 33
#define IN4_GPIO 32
#define IN5_GPIO 19
#define IN6_GPIO 18
#define IN7_GPIO 17
#define IN8_GPIO 16
//小车控制指令
char *Car_Ctral_US = "US";
char *Car_Ctral_LS = "LS";
char *Car_Ctral_RS = "RS";
char *Car_Ctral_DS = "DS";
char *Car_Ctral_SP = "SP";

/*char *Car_Ctral_UL = "UL";
char *Car_Ctral_UR = "UR";
char *Car_Ctral_DL = "DL";
char *Car_Ctral_DR = "DR";*/

#else
#define HOST_IP_ADDR CONFIG_EXAMPLE_IPV6_ADDR
#endif

//#define PORT CONFIG_EXAMPLE_PORT
#define PORT 8080   //端口号


static const char *TAG = "example";
static const char *payload = "ESP_ACK";

static void car_config_init(void)
{
     /* 1、首先配置IO口，哪个口？什么模式？（GPIO_26,输出模式） Set the GPIO as a push/pull output */
    gpio_pad_select_gpio(IN1_GPIO);
    gpio_set_direction(IN1_GPIO, GPIO_MODE_DEF_OUTPUT);

    gpio_pad_select_gpio(IN2_GPIO);
    gpio_set_direction(IN2_GPIO, GPIO_MODE_DEF_OUTPUT);

    gpio_pad_select_gpio(IN3_GPIO);
    gpio_set_direction(IN3_GPIO, GPIO_MODE_DEF_OUTPUT);

    gpio_pad_select_gpio(IN4_GPIO);
    gpio_set_direction(IN4_GPIO, GPIO_MODE_DEF_OUTPUT);

    gpio_pad_select_gpio(IN5_GPIO);
    gpio_set_direction(IN5_GPIO, GPIO_MODE_DEF_OUTPUT);

    gpio_pad_select_gpio(IN6_GPIO);
    gpio_set_direction(IN6_GPIO, GPIO_MODE_DEF_OUTPUT);

    gpio_pad_select_gpio(IN7_GPIO);
    gpio_set_direction(IN7_GPIO, GPIO_MODE_DEF_OUTPUT);

    gpio_pad_select_gpio(IN8_GPIO);
    gpio_set_direction(IN8_GPIO, GPIO_MODE_DEF_OUTPUT);

}
//小车控制
static void car_contral(char* C_buffer)
{

     //命令识别   char *Car_Ctral_UL = "US";  前进
     if (strncmp( C_buffer, Car_Ctral_US, 2) == 0) {
      gpio_set_level(IN1_GPIO, 1);
      gpio_set_level(IN2_GPIO, 0);
      gpio_set_level(IN3_GPIO, 1);
      gpio_set_level(IN4_GPIO, 0);
      gpio_set_level(IN5_GPIO, 1);
      gpio_set_level(IN6_GPIO, 0);
      gpio_set_level(IN7_GPIO, 1);
      gpio_set_level(IN8_GPIO, 0);
      //vTaskDelay(1000 / portTICK_PERIOD_MS);
     }
     //char *Car_Ctral_LS = "DS";       后退
     if (strncmp( C_buffer, Car_Ctral_DS, 2) == 0) {
      gpio_set_level(IN1_GPIO, 0);
      gpio_set_level(IN2_GPIO, 1);
      gpio_set_level(IN3_GPIO, 0);
      gpio_set_level(IN4_GPIO, 1);
      gpio_set_level(IN5_GPIO, 0);
      gpio_set_level(IN6_GPIO, 1);
      gpio_set_level(IN7_GPIO, 0);
      gpio_set_level(IN8_GPIO, 1);
      //vTaskDelay(1000 / portTICK_PERIOD_MS);
     }
     //char *Car_Ctral_LS = "LS";       左转
     if (strncmp( C_buffer, Car_Ctral_LS, 2) == 0) {
      gpio_set_level(IN1_GPIO, 1);
      gpio_set_level(IN2_GPIO, 0);
      gpio_set_level(IN3_GPIO, 0);
      gpio_set_level(IN4_GPIO, 1);
      gpio_set_level(IN5_GPIO, 0);
      gpio_set_level(IN6_GPIO, 1);
      gpio_set_level(IN7_GPIO, 1);
      gpio_set_level(IN8_GPIO, 0);
      //vTaskDelay(1000 / portTICK_PERIOD_MS);
     }
     //char *Car_Ctral_LS = "RS";       右转
     if (strncmp( C_buffer, Car_Ctral_RS, 2) == 0) {
      gpio_set_level(IN1_GPIO, 0);
      gpio_set_level(IN2_GPIO, 1);
      gpio_set_level(IN3_GPIO, 1);
      gpio_set_level(IN4_GPIO, 0);
      gpio_set_level(IN5_GPIO, 1);
      gpio_set_level(IN6_GPIO, 0);
      gpio_set_level(IN7_GPIO, 0);
      gpio_set_level(IN8_GPIO, 1);
     // vTaskDelay(1000 / portTICK_PERIOD_MS);
     }
     //char *Car_Ctral_SP = "SP";       停止
     if (strncmp( C_buffer, Car_Ctral_SP, 2) == 0) {
      gpio_set_level(IN1_GPIO, 0);
      gpio_set_level(IN2_GPIO, 0);
      gpio_set_level(IN3_GPIO, 0);
      gpio_set_level(IN4_GPIO, 0);
      gpio_set_level(IN5_GPIO, 0);
      gpio_set_level(IN6_GPIO, 0);
      gpio_set_level(IN7_GPIO, 0);
      gpio_set_level(IN8_GPIO, 0);
     // vTaskDelay(1000 / portTICK_PERIOD_MS);
     }
}
//tcp_client_task
static void tcp_client_task(void *pvParameters)
{
    char rx_buffer[128];
    char addr_str[128];
    int addr_family;
    int ip_protocol;
//#ifdef CAR_CONTRAL_OPEN
    car_config_init();
//#endif

    while (1) {

#ifdef CONFIG_EXAMPLE_IPV4
        struct sockaddr_in dest_addr;
        dest_addr.sin_addr.s_addr = inet_addr(HOST_IP_ADDR);
        dest_addr.sin_family = AF_INET;
        dest_addr.sin_port = htons(PORT);
        addr_family = AF_INET;
        ip_protocol = IPPROTO_IP;
        inet_ntoa_r(dest_addr.sin_addr, addr_str, sizeof(addr_str) - 1);
#else // IPV6
        struct sockaddr_in6 dest_addr;
        inet6_aton(HOST_IP_ADDR, &dest_addr.sin6_addr);
        dest_addr.sin6_family = AF_INET6;
        dest_addr.sin6_port = htons(PORT);
        addr_family = AF_INET6;
        ip_protocol = IPPROTO_IPV6;
        inet6_ntoa_r(dest_addr.sin6_addr, addr_str, sizeof(addr_str) - 1);
#endif

        int sock =  socket(addr_family, SOCK_STREAM, ip_protocol);
        if (sock < 0) {
            ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
            break;
        }
        ESP_LOGI(TAG, "Socket created, connecting to %s:%d", HOST_IP_ADDR, PORT);

        int err = connect(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
        if (err != 0) {
            ESP_LOGE(TAG, "Socket unable to connect: errno %d", errno);
            break;
        }
        ESP_LOGI(TAG, "Successfully connected");

        while (1) {
            int err = send(sock, payload, strlen(payload), 0);
            if (err < 0) {
                ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
                break;
            }

            int len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
            // Error occurred during receiving
            if (len < 0) {
                ESP_LOGE(TAG, "recv failed: errno %d", errno);
                break;
            }
            // Data received
            else {
                rx_buffer[len] = 0; // Null-terminate whatever we received and treat like a string

           // #ifdef CAR_CONTRAL_OPEN
             car_contral(rx_buffer);
            //#endif
              ESP_LOGI(TAG, "Received %d bytes from %s:", len, addr_str);
              ESP_LOGI(TAG, "%s", rx_buffer);
            }

            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }

        if (sock != -1) {
            ESP_LOGE(TAG, "Shutting down socket and restarting...");
            shutdown(sock, 0);
            close(sock);
        }
    }
    vTaskDelete(NULL);
}

void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    tcpip_adapter_init(); //初始化基础的TCP / IP堆栈
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
     * Read "Establishing Wi-Fi or Ethernet Connection" section in
     * examples/protocols/README.md for more information about this function.
     */
    ESP_ERROR_CHECK(example_connect());

    xTaskCreate(tcp_client_task, "tcp_client", 4096, NULL, 5, NULL);
}
