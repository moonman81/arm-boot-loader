/* Network Protocol Implementations with Dependency Inversion */

#include "network_protocols.h"
#include "ethernet.h"  // For network structures and functions

#ifndef NULL
#define NULL ((void *)0)
#endif

// Simple memory management for freestanding environment
static void *malloc(uint32_t size) {
    // Simple bump allocator - in real implementation would use proper heap
    static uint8_t heap[1024];
    static uint32_t heap_used = 0;

    if (heap_used + size > sizeof(heap)) return NULL;
    void *ptr = &heap[heap_used];
    heap_used += size;
    return ptr;
}

static void free(void *ptr) {
    // No-op for simple allocator
    (void)ptr;
}

static char *strcpy(char *dest, const char *src) {
    char *d = dest;
    while ((*d++ = *src++));
    return dest;
}

// DNS Protocol Implementation
static int dns_resolve_hostname(const char *hostname, uint32_t *ip_addr) {
    return network_dns_resolve(hostname, ip_addr);
}

static int dns_send_request(uint32_t dest_ip, uint16_t src_port, uint16_t dest_port,
                           const void *data, uint16_t data_length) {
    return network_send_udp(dest_ip, src_port, dest_port, data, data_length);
}

static int dns_receive_response(void *buffer, uint32_t max_size, uint32_t *received) {
    // DNS responses are handled asynchronously in the current implementation
    // For dependency inversion, this would need to be implemented
    return -1; // Not implemented yet
}

dns_protocol_t *dns_protocol_create(uint32_t dns_server) {
    dns_protocol_t *protocol = (dns_protocol_t *)malloc(sizeof(dns_protocol_t));
    if (!protocol) return NULL;

    protocol->dns_server = dns_server;
    protocol->vtable.resolve_hostname = dns_resolve_hostname;
    protocol->vtable.send_request = dns_send_request;
    protocol->vtable.receive_response = dns_receive_response;

    return protocol;
}

void dns_protocol_destroy(dns_protocol_t *protocol) {
    if (protocol) {
        free(protocol);
    }
}

// HTTP Protocol Implementation
static int http_resolve_hostname(const char *hostname, uint32_t *ip_addr) {
    return network_dns_resolve(hostname, ip_addr);
}

static int http_send_request(uint32_t dest_ip, uint16_t src_port, uint16_t dest_port,
                            const void *data, uint16_t data_length) {
    return network_send_udp(dest_ip, src_port, dest_port, data, data_length);
}

static int http_receive_response(void *buffer, uint32_t max_size, uint32_t *received) {
    // HTTP responses would need TCP implementation
    return -1; // Not implemented yet
}

http_protocol_t *http_protocol_create(const char *user_agent) {
    http_protocol_t *protocol = (http_protocol_t *)malloc(sizeof(http_protocol_t));
    if (!protocol) return NULL;

    if (user_agent) {
        strcpy(protocol->user_agent, user_agent);
    } else {
        strcpy(protocol->user_agent, "ARM-Bootloader/1.0");
    }

    protocol->vtable.resolve_hostname = http_resolve_hostname;
    protocol->vtable.send_request = http_send_request;
    protocol->vtable.receive_response = http_receive_response;

    return protocol;
}

void http_protocol_destroy(http_protocol_t *protocol) {
    if (protocol) {
        free(protocol);
    }
}

// DHCP Protocol Implementation
static int dhcp_resolve_hostname(const char *hostname, uint32_t *ip_addr) {
    // DHCP doesn't do hostname resolution
    return -1;
}

static int dhcp_send_request(uint32_t dest_ip, uint16_t src_port, uint16_t dest_port,
                            const void *data, uint16_t data_length) {
    return network_send_udp(dest_ip, src_port, dest_port, data, data_length);
}

static int dhcp_receive_response(void *buffer, uint32_t max_size, uint32_t *received) {
    // DHCP responses are handled in the DHCP functions
    return -1; // Not implemented yet
}

dhcp_protocol_t *dhcp_protocol_create(uint32_t transaction_id) {
    dhcp_protocol_t *protocol = (dhcp_protocol_t *)malloc(sizeof(dhcp_protocol_t));
    if (!protocol) return NULL;

    protocol->transaction_id = transaction_id;
    protocol->vtable.resolve_hostname = dhcp_resolve_hostname;
    protocol->vtable.send_request = dhcp_send_request;
    protocol->vtable.receive_response = dhcp_receive_response;

    return protocol;
}

void dhcp_protocol_destroy(dhcp_protocol_t *protocol) {
    if (protocol) {
        free(protocol);
    }
}

// TFTP Protocol Implementation
static int tftp_resolve_hostname(const char *hostname, uint32_t *ip_addr) {
    return network_dns_resolve(hostname, ip_addr);
}

static int tftp_send_request(uint32_t dest_ip, uint16_t src_port, uint16_t dest_port,
                            const void *data, uint16_t data_length) {
    return network_send_udp(dest_ip, src_port, dest_port, data, data_length);
}

static int tftp_receive_response(void *buffer, uint32_t max_size, uint32_t *received) {
    // TFTP responses are handled in the TFTP download function
    return -1; // Not implemented yet
}

tftp_protocol_t *tftp_protocol_create(void) {
    tftp_protocol_t *protocol = (tftp_protocol_t *)malloc(sizeof(tftp_protocol_t));
    if (!protocol) return NULL;

    protocol->block_number = 1;
    protocol->vtable.resolve_hostname = tftp_resolve_hostname;
    protocol->vtable.send_request = tftp_send_request;
    protocol->vtable.receive_response = tftp_receive_response;

    return protocol;
}

void tftp_protocol_destroy(tftp_protocol_t *protocol) {
    if (protocol) {
        free(protocol);
    }
}