/* Network Protocol Interfaces for Dependency Inversion */

#ifndef NETWORK_PROTOCOLS_H
#define NETWORK_PROTOCOLS_H

#include "ethernet.h"

// Abstract interface for network protocols
typedef struct network_protocol_vtable {
    int (*resolve_hostname)(const char *hostname, uint32_t *ip_addr);
    int (*send_request)(uint32_t dest_ip, uint16_t src_port, uint16_t dest_port,
                       const void *data, uint16_t data_length);
    int (*receive_response)(void *buffer, uint32_t max_size, uint32_t *received);
} network_protocol_vtable_t;

// DNS protocol interface
typedef struct dns_protocol {
    network_protocol_vtable_t vtable;
    uint32_t dns_server;
} dns_protocol_t;

// HTTP protocol interface
typedef struct http_protocol {
    network_protocol_vtable_t vtable;
    char user_agent[64];
} http_protocol_t;

// DHCP protocol interface
typedef struct dhcp_protocol {
    network_protocol_vtable_t vtable;
    uint32_t transaction_id;
} dhcp_protocol_t;

// TFTP protocol interface
typedef struct tftp_protocol {
    network_protocol_vtable_t vtable;
    uint16_t block_number;
} tftp_protocol_t;

// Protocol factory functions
dns_protocol_t *dns_protocol_create(uint32_t dns_server);
http_protocol_t *http_protocol_create(const char *user_agent);
dhcp_protocol_t *dhcp_protocol_create(uint32_t transaction_id);
tftp_protocol_t *tftp_protocol_create(void);

// Protocol destruction
void dns_protocol_destroy(dns_protocol_t *protocol);
void http_protocol_destroy(http_protocol_t *protocol);
void dhcp_protocol_destroy(dhcp_protocol_t *protocol);
void tftp_protocol_destroy(tftp_protocol_t *protocol);

#endif