#ifndef MQTT_PUBLISHER_H
#define MQTT_PUBLISHER_H
#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

typedef void (*subscribe_cb_t)(const char *payload);
typedef	struct
{
	bool (*initialize)(void);
	bool (*publish)(char *message);
	bool (*set_subscribe_callback)(subscribe_cb_t subscribe_cb);
	bool (*process_input)(void);
} mqtt_publisher_t;

extern const mqtt_publisher_t mqtt_publisher;

#ifdef __cplusplus
}
#endif

#endif /* MQTT_PUBLISHER_H */

