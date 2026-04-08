#ifndef __PROTO_20_WIFI_PROVISION_C__
#define __PROTO_20_WIFI_PROVISION_C__
//Extracted Prototyes
// ****************************
// src/20-wifi-provision.c prototypes
// ****************************
void wifi_provision_save_config(void);
void wifi_provision_load_config(void);
void wifi_provision_connect(void);
httpd_handle_t wifi_provision_start_webserver(void);
void wifi_provision_init(void);
#endif
