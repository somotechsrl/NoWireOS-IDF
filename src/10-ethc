#include "esp_eth.h"
#include "esp_log.h"
#include "driver/gpio.h"

#define ETH_PHY_ADDR        1
#define ETH_MDC_GPIO        23
#define ETH_MDIO_GPIO       18
#define ETH_POWER_GPIO      -1 // Set to -1 if not used

static const char *TAG = "ethernet_init";

void ethernet_init(void)
{
#if defined(CONFIG_WAVESHARE_ETHERNET)
    esp_err_t ret;

    // Optional: Power up the PHY if needed
    if (ETH_POWER_GPIO >= 0) {
        gpio_pad_select_gpio(ETH_POWER_GPIO);
        gpio_set_direction(ETH_POWER_GPIO, GPIO_MODE_OUTPUT);
        gpio_set_level(ETH_POWER_GPIO, 1);
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    eth_mac_config_t mac_config = ETH_MAC_DEFAULT_CONFIG();
    eth_phy_config_t phy_config = ETH_PHY_DEFAULT_CONFIG();

    mac_config.smi_mdc_gpio_num = ETH_MDC_GPIO;
    mac_config.smi_mdio_gpio_num = ETH_MDIO_GPIO;

    esp_eth_mac_t *mac = esp_eth_mac_new_esp32(&mac_config);
    esp_eth_phy_t *phy = esp_eth_phy_new_lan8720(&phy_config);

    esp_eth_handle_t eth_handle = NULL;
    esp_eth_config_t config = ETH_DEFAULT_CONFIG(mac, phy);

    ret = esp_eth_driver_install(&config, &eth_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Ethernet driver install failed: %s", esp_err_to_name(ret));
        return;
    }

    ret = esp_eth_start(eth_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Ethernet start failed: %s", esp_err_to_name(ret));
        return;
    }

    ESP_LOGI(TAG, "Waveshare Ethernet initialized");
#else
    ESP_LOGI(TAG, "Waveshare Ethernet not enabled in config");
#endif
}