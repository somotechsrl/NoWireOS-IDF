#include "main.h"

#define BLINK_GPIO 2
#define BLINK_DELAY_MS 500L

void led_blink(void) {  
    
    // Configure the GPIO pin
    gpio_reset_pin(BLINK_GPIO);
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);

    // Blink loop
    while (1) {
        // Turn LED ON
        //printf("LED ON\n");
        gpio_set_level(BLINK_GPIO, 1);
        vTaskDelay(BLINK_DELAY_MS / portTICK_PERIOD_MS); // Delay 1 second
        
        // Turn LED OFF
        //printf("LED OFF\n");
        gpio_set_level(BLINK_GPIO, 0);
        vTaskDelay(BLINK_DELAY_MS / portTICK_PERIOD_MS); // Delay 1 second
    }

}
