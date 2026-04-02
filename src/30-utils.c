#include "main.h"

#define BLINK_GPIO 2
#define BLINK_DELAY_MS 500L
bool led_blink_enabled = true;

static void led_blink(void) {  
    
    // Configure the GPIO pin
    gpio_reset_pin(BLINK_GPIO);
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);

    // Blink loop
    while (true) {

        // Turn LED ON
        //printf("LED ON\n");
        if(!led_blink_enabled) {
            vTaskDelay(BLINK_DELAY_MS / portTICK_PERIOD_MS); // Delay 1 second
            continue;
        }   

        gpio_set_level(BLINK_GPIO, 1);
        vTaskDelay(BLINK_DELAY_MS / portTICK_PERIOD_MS); // Delay 1 second
        
        // Turn LED OFF
        //printf("LED OFF\n");
        gpio_set_level(BLINK_GPIO, 0);
        vTaskDelay(BLINK_DELAY_MS / portTICK_PERIOD_MS); // Delay 1 second
    }

void led_blink_init(void) {
    xTaskCreate(led_bink, "led_blink", 4096, NULL, 5, NULL);
    }
