#ifndef NET_MNGR_H
#define NET_MNGR_H
#include <zephyr/types.h>
#include <stdbool.h>

typedef struct {
	const char* (*get_guid) ();
	bool (*is_network_ready) ();
	void (*restart) ();
} net_mngr_t;

extern const net_mngr_t net_mngr;

#endif
