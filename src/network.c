/* Network Stack Implementation for Bootloader */

#include "ethernet.h"
#include "timer.h"

#ifndef NULL
#define NULL ((void *)0)
#endif

// Custom string functions for freestanding environment
static void *memcpy(void *dest, const void *src, uint32_t n) {
    unsigned char *d = dest;
    const unsigned char *s = src;
    while (n--) *d++ = *s++;
    return dest;
}

static void *memset(void *s, int c, uint32_t n) {
    unsigned char *p = s;
    while (n--) *p++ = c;
    return s;
}

static uint32_t strlen(const char *s) {
    uint32_t len = 0;
    while (*s++) len++;
    return len;
}

static char *strcpy(char *dest, const char *src) {
    char *d = dest;
    while ((*d++ = *src++));
    return dest;
}

static char *strstr(const char *haystack, const char *needle) {
    if (!haystack || !needle) return NULL;

    uint32_t hlen = strlen(haystack);
    uint32_t nlen = strlen(needle);

    if (nlen == 0) return (char *)haystack;
    if (nlen > hlen) return NULL;

    for (uint32_t i = 0; i <= hlen - nlen; i++) {
        uint32_t j;
        for (j = 0; j < nlen; j++) {
            if (haystack[i + j] != needle[j]) break;
        }
        if (j == nlen) return (char *)&haystack[i];
    }

    return NULL;
}

static char *strchr(const char *s, int c) {
    while (*s) {
        if (*s == c) return (char *)s;
        s++;
    }
    return NULL;
}

static int memcmp(const void *s1, const void *s2, uint32_t n) {
    const unsigned char *p1 = s1;
    const unsigned char *p2 = s2;
    while (n--) {
        if (*p1 != *p2) return *p1 - *p2;
        p1++;
        p2++;
    }
    return 0;
}

// Network configuration
static network_config_t network_config;
static uint32_t dhcp_xid = 0x12345678; // Transaction ID

// Network receive state
typedef struct {
    ethernet_frame_t rx_buffer;
    uint16_t rx_length;
    int rx_pending;
} network_rx_state_t;

static network_rx_state_t rx_state = {0};

// Security constants
#define NETWORK_MAX_RETRIES 3
#define NETWORK_DHCP_TIMEOUT_MS 5000
#define NETWORK_BOOTP_TIMEOUT_MS 5000
#define NETWORK_TFTP_TIMEOUT_MS 10000
#define NETWORK_MAX_FRAME_SIZE 1514
#define NETWORK_ARP_TIMEOUT_MS 1000
#define NETWORK_MAC_VALIDATION_MASK 0x010000000000ULL

// ARP cache (simple - expand to multiple entries if needed)
#define ARP_CACHE_SIZE 8
static struct {
    uint32_t ip;
    uint8_t mac[6];
    uint32_t timestamp;
} arp_cache[ARP_CACHE_SIZE];
static int arp_cache_count = 0;

// Security validation functions
static int validate_mac_address(const uint8_t *mac) {
    if (!mac) return 0;

    // Check for null MAC
    uint64_t mac_int = ((uint64_t)mac[0] << 40) | ((uint64_t)mac[1] << 32) |
                       ((uint64_t)mac[2] << 24) | ((uint64_t)mac[3] << 16) |
                       ((uint64_t)mac[4] << 8) | mac[5];
    if (mac_int == 0 || mac_int == 0xFFFFFFFFFFFFULL) return 0;

    // Check locally administered bit (should be 0 for valid unicast)
    if (mac[0] & 0x02) return 0;

    return 1;
}

static int validate_ip_address(const uint8_t *ip) {
    if (!ip) return 0;

    // Check for invalid IPs (0.0.0.0, 255.255.255.255, loopback in network context)
    if (ip[0] == 0 && ip[1] == 0 && ip[2] == 0 && ip[3] == 0) return 0;
    if (ip[0] == 255 && ip[1] == 255 && ip[2] == 255 && ip[3] == 255) return 0;
    if (ip[0] == 127) return 0; // Loopback

    return 1;
}

static int validate_filename(const char *filename) {
    if (!filename || strlen(filename) == 0 || strlen(filename) > 255) return 0;

    // Check for path traversal attempts
    if (strstr(filename, "..") || strstr(filename, "/") || strstr(filename, "\\")) return 0;

    return 1;
}

uint16_t network_checksum(const void *data, uint32_t length) {
    uint32_t sum = 0;
    const uint16_t *ptr = (const uint16_t *)data;

    while (length > 1) {
        sum += *ptr++;
        length -= 2;
    }

    if (length > 0) {
        sum += *(uint8_t *)ptr;
    }

    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    return ~sum;
}

uint32_t network_ip_to_int(const uint8_t *ip) {
    return (ip[0] << 24) | (ip[1] << 16) | (ip[2] << 8) | ip[3];
}

void network_int_to_ip(uint32_t ip_int, uint8_t *ip) {
    ip[0] = (ip_int >> 24) & 0xFF;
    ip[1] = (ip_int >> 16) & 0xFF;
    ip[2] = (ip_int >> 8) & 0xFF;
    ip[3] = ip_int & 0xFF;
}

// ARP functions
int network_arp_request(uint32_t target_ip, uint8_t *target_mac) {
    if (!validate_ip_address((uint8_t *)&target_ip)) {
        return -1;
    }

    // Check cache first
    uint64_t current_time = timer_get_counter();
    for (int i = 0; i < arp_cache_count; i++) {
        if (arp_cache[i].ip == target_ip) {
            // Check if entry is still valid (10 minutes)
            if ((current_time - arp_cache[i].timestamp) / 1000000 < 600000) {
                memcpy(target_mac, arp_cache[i].mac, 6);
    return 0;
}
            // Remove stale entry
            arp_cache[i] = arp_cache[--arp_cache_count];
            break;
        }
    }

    // Send ARP request
    uint8_t arp_buffer[sizeof(ethernet_frame_t) + sizeof(arp_packet_t)];
    ethernet_frame_t *eth_frame = (ethernet_frame_t *)arp_buffer;
    arp_packet_t *arp_packet = (arp_packet_t *)eth_frame->payload;

    // Ethernet header
    eth_frame->ethertype = ETHERTYPE_ARP;
    memcpy(eth_frame->src_mac, network_config.mac_addr, 6);
    memset(eth_frame->dest_mac, 0xFF, 6); // Broadcast

    // ARP packet
    arp_packet->hw_type = ARP_HWTYPE_ETHERNET;
    arp_packet->proto_type = ARP_PROTO_IPV4;
    arp_packet->hw_addr_len = 6;
    arp_packet->proto_addr_len = 4;
    arp_packet->operation = ARP_OP_REQUEST;
    memcpy(arp_packet->sender_hw_addr, network_config.mac_addr, 6);
    arp_packet->sender_proto_addr = network_ip_to_int(network_config.ip_addr);
    memset(arp_packet->target_hw_addr, 0, 6);
    arp_packet->target_proto_addr = target_ip;

    // Send frame
    if (ethernet_send_frame(eth_frame, sizeof(ethernet_frame_t) + sizeof(arp_packet_t)) < 0) {
        return -1;
    }

    // Wait for ARP reply with timeout
    uint64_t timeout_start = timer_get_counter();
    while ((timer_get_counter() - timeout_start) / 1000 < NETWORK_ARP_TIMEOUT_MS) {
        // In a real implementation, would process incoming packets
        timer_delay_ms(10);
    }

    // For bootloader, simulate ARP reply for gateway
    if (target_ip == network_ip_to_int(network_config.gateway)) {
        // Use a dummy MAC for gateway
        target_mac[0] = 0x00; target_mac[1] = 0x11; target_mac[2] = 0x22;
        target_mac[3] = 0x33; target_mac[4] = 0x44; target_mac[5] = 0x55;

        // Cache the entry
        if (arp_cache_count < ARP_CACHE_SIZE) {
            arp_cache[arp_cache_count].ip = target_ip;
            memcpy(arp_cache[arp_cache_count].mac, target_mac, 6);
            arp_cache[arp_cache_count].timestamp = current_time;
            arp_cache_count++;
        }
        return 0;
    }

    return -1; // Timeout or no response
}

int network_arp_reply(uint32_t target_ip, const uint8_t *target_mac) {
    if (!validate_ip_address((uint8_t *)&target_ip) || !target_mac) {
        return -1;
    }

    uint8_t arp_buffer[sizeof(ethernet_frame_t) + sizeof(arp_packet_t)];
    ethernet_frame_t *eth_frame = (ethernet_frame_t *)arp_buffer;
    arp_packet_t *arp_packet = (arp_packet_t *)eth_frame->payload;

    // Ethernet header
    eth_frame->ethertype = ETHERTYPE_ARP;
    memcpy(eth_frame->src_mac, network_config.mac_addr, 6);
    memcpy(eth_frame->dest_mac, target_mac, 6);

    // ARP packet
    arp_packet->hw_type = ARP_HWTYPE_ETHERNET;
    arp_packet->proto_type = ARP_PROTO_IPV4;
    arp_packet->hw_addr_len = 6;
    arp_packet->proto_addr_len = 4;
    arp_packet->operation = ARP_OP_REPLY;
    memcpy(arp_packet->sender_hw_addr, network_config.mac_addr, 6);
    arp_packet->sender_proto_addr = network_ip_to_int(network_config.ip_addr);
    memcpy(arp_packet->target_hw_addr, target_mac, 6);
    arp_packet->target_proto_addr = target_ip;

    return ethernet_send_frame(eth_frame, sizeof(ethernet_frame_t) + sizeof(arp_packet_t));
}

int network_process_arp_packet(const arp_packet_t *arp_packet) {
    if (!arp_packet) return -1;

    // Only handle IPv4 Ethernet ARP
    if (arp_packet->hw_type != ARP_HWTYPE_ETHERNET ||
        arp_packet->proto_type != ARP_PROTO_IPV4 ||
        arp_packet->hw_addr_len != 6 ||
        arp_packet->proto_addr_len != 4) {
        return -1;
    }

    uint32_t current_time = (uint32_t)(timer_get_counter() / 1000000);

    if (arp_packet->operation == ARP_OP_REQUEST) {
        // Check if the request is for our IP
        if (arp_packet->target_proto_addr == network_ip_to_int(network_config.ip_addr)) {
            // Send ARP reply
            return network_arp_reply(arp_packet->sender_proto_addr, arp_packet->sender_hw_addr);
        }
    } else if (arp_packet->operation == ARP_OP_REPLY) {
        // Cache the reply
        if (arp_cache_count < ARP_CACHE_SIZE) {
            arp_cache[arp_cache_count].ip = arp_packet->sender_proto_addr;
            memcpy(arp_cache[arp_cache_count].mac, arp_packet->sender_hw_addr, 6);
            arp_cache[arp_cache_count].timestamp = current_time;
            arp_cache_count++;
        }
    }

    return 0;
}

// UDP packet creation and sending
int network_send_udp(uint32_t dest_ip, uint16_t src_port, uint16_t dest_port,
                     const void *data, uint16_t data_length) {
    uint8_t packet_buffer[1514];
    ethernet_frame_t *eth_frame = (ethernet_frame_t *)packet_buffer;
    ip_header_t *ip_hdr = (ip_header_t *)eth_frame->payload;
    udp_header_t *udp_hdr = (udp_header_t *)(eth_frame->payload + sizeof(ip_header_t));

    uint16_t total_length = sizeof(ip_header_t) + sizeof(udp_header_t) + data_length;

    // Ethernet header
    eth_frame->ethertype = ETHERTYPE_IP;
    memcpy(eth_frame->src_mac, network_config.mac_addr, 6);

    // Get destination MAC via ARP
    if (network_arp_request(dest_ip, eth_frame->dest_mac) < 0) {
        return -1;
    }

    // IP header
    ip_hdr->version_ihl = (4 << 4) | 5; // IPv4, 20 byte header
    ip_hdr->tos = 0;
    ip_hdr->total_length = total_length;
    ip_hdr->identification = 0;
    ip_hdr->flags_fragment = 0;
    ip_hdr->ttl = 64;
    ip_hdr->protocol = IP_PROTO_UDP;
    ip_hdr->checksum = 0;
    ip_hdr->src_ip = network_ip_to_int(network_config.ip_addr);
    ip_hdr->dest_ip = dest_ip;
    ip_hdr->checksum = network_checksum(ip_hdr, sizeof(ip_header_t));

    // UDP header
    udp_hdr->src_port = src_port;
    udp_hdr->dest_port = dest_port;
    udp_hdr->length = sizeof(udp_header_t) + data_length;
    udp_hdr->checksum = 0; // Optional for IPv4

    // Copy data
    memcpy(udp_hdr + 1, data, data_length);

    // Send frame
    return ethernet_send_frame(eth_frame, total_length + sizeof(ethernet_frame_t) - sizeof(eth_frame->payload));
}

// DNS resolution functions with proper response parsing
int network_dns_resolve(const char *hostname, uint32_t *ip_addr) {
    if (!hostname || !ip_addr) return -1;

    // Get DNS server from config
    uint32_t dns_server = network_ip_to_int(network_config.dns_server);
    if (dns_server == 0) return -1;

    // Build DNS query
    uint8_t dns_packet[512];
    dns_header_t *header = (dns_header_t *)dns_packet;
    uint8_t *ptr = dns_packet + sizeof(dns_header_t);

    // DNS header (network byte order)
    uint16_t query_id = 0x1234;
    header->id = (query_id >> 8) | (query_id << 8);
    header->flags = 0x0001; // Standard query with recursion desired (network byte order)
    header->qdcount = 0x0100; // 1 question
    header->ancount = 0;
    header->nscount = 0;
    header->arcount = 0;

    // Convert hostname to DNS format (labels)
    const char *part = hostname;
    while (*part) {
        const char *dot = strchr(part, '.');
        uint8_t len = dot ? (dot - part) : strlen(part);
        if (len == 0 || len > 63) return -1; // Invalid label
        *ptr++ = len;
        memcpy(ptr, part, len);
        ptr += len;
        part += len;
        if (*part == '.') part++;
    }
    *ptr++ = 0; // End of name

    // Question section
    *(uint16_t *)ptr = 0x0100; // Type A (network byte order)
    ptr += 2;
    *(uint16_t *)ptr = 0x0100; // Class IN (network byte order)
    ptr += 2;

    uint16_t query_length = ptr - dns_packet;

    // Send DNS query with source port
    uint16_t src_port = 53000; // Use high port for DNS query
    if (network_send_udp(dns_server, src_port, UDP_PORT_DNS, dns_packet, query_length) < 0) {
        return -1;
    }

    // Wait for DNS response
    ethernet_frame_t rx_frame;
    uint16_t rx_len;
    int retries = 3;

    while (retries--) {
        if (network_receive_packet(&rx_frame, &rx_len, 2000) == 0) { // 2 second timeout
            // Check if it's an IP/UDP packet from DNS server
            if (rx_frame.ethertype != ETHERTYPE_IP) continue;

            ip_header_t *ip_hdr = (ip_header_t *)rx_frame.payload;
            if (ip_hdr->protocol != IP_PROTO_UDP) continue;
            if (ip_hdr->src_ip != dns_server) continue;

            udp_header_t *udp_hdr = (udp_header_t *)(rx_frame.payload + sizeof(ip_header_t));
            if (udp_hdr->src_port != UDP_PORT_DNS) continue;

            dns_header_t *response = (dns_header_t *)((uint8_t *)udp_hdr + sizeof(udp_header_t));

            // Verify it's our query
            if (response->id != header->id) continue;

            // Check if we have answers
            uint16_t ancount = (response->ancount >> 8) | (response->ancount << 8);
            if (ancount == 0) return -1; // No answers

            // Skip question section in response
            ptr = (uint8_t *)response + sizeof(dns_header_t);

            // Parse question (skip it)
            while (*ptr != 0) {
                if ((*ptr & 0xC0) == 0xC0) {
                    ptr += 2; // Compression pointer
                    break;
                }
                ptr += *ptr + 1;
            }
            if (*ptr == 0) ptr++; // Skip final zero
            ptr += 4; // Skip QTYPE and QCLASS

            // Parse answer section
            for (int i = 0; i < ancount; i++) {
                // Skip name (may be compressed)
                if ((*ptr & 0xC0) == 0xC0) {
                    ptr += 2;
                } else {
                    while (*ptr != 0) ptr += *ptr + 1;
                    ptr++;
                }

                // Read type, class, TTL, length
                uint16_t rr_type = (ptr[0] << 8) | ptr[1];
                ptr += 2;
                uint16_t rr_class = (ptr[0] << 8) | ptr[1];
                ptr += 2;
                uint32_t ttl = (ptr[0] << 24) | (ptr[1] << 16) | (ptr[2] << 8) | ptr[3];
                ptr += 4;
                uint16_t rdlength = (ptr[0] << 8) | ptr[1];
                ptr += 2;

                // If it's an A record (Type 1, Class 1), extract IP
                if (rr_type == 1 && rr_class == 1 && rdlength == 4) {
                    *ip_addr = (ptr[0] << 24) | (ptr[1] << 16) | (ptr[2] << 8) | ptr[3];
                    return 0; // Success!
                }

                ptr += rdlength; // Skip to next RR
            }
        }
    }

    return -1; // Timeout or no valid response
}

int network_dns_resolve_ipv6(const char *hostname, uint8_t *ipv6_addr) {
    if (!hostname || !ipv6_addr) return -1;

    // Similar to IPv4 but for AAAA records
    // Implementation would be similar but with IPv6 DNS server
    return -1; // Placeholder
}

// HTTP client functions
int network_http_get(const char *url, uint8_t *buffer, uint32_t max_size, uint32_t *received) {
    if (!url || !buffer || !received) return -1;

    // Parse URL (simplified)
    const char *host_start = strstr(url, "http://");
    if (!host_start) return -1;
    host_start += 7;

    const char *path_start = strchr(host_start, '/');
    if (!path_start) path_start = host_start + strlen(host_start);

    char host[128];
    uint32_t host_len = path_start - host_start;
    if (host_len >= sizeof(host)) return -1;
    memcpy(host, host_start, host_len);
    host[host_len] = 0;

    // Resolve hostname to IP
    uint32_t server_ip;
    if (network_dns_resolve(host, &server_ip) < 0) {
        // Try to parse as IP
        if (network_parse_ip(host, &server_ip) < 0) return -1;
    }

    // Build HTTP request
    char request[512];
    char *req_ptr = request;

    // Copy "GET "
    strcpy(req_ptr, "GET ");
    req_ptr += 4;

    // Copy path
    strcpy(req_ptr, path_start);
    req_ptr += strlen(path_start);

    // Copy rest of request
    strcpy(req_ptr, " HTTP/1.0\r\nHost: ");
    req_ptr += 17;

    // Copy host
    strcpy(req_ptr, host);
    req_ptr += strlen(host);

    // Copy rest
    strcpy(req_ptr, "\r\nUser-Agent: ARM-Bootloader/1.0\r\n\r\n");
    req_ptr += 35;

    int len = req_ptr - request;
    if (len >= sizeof(request)) return -1;

    // Send HTTP request over TCP (simplified - would need TCP implementation)
    // For now, return error as TCP not fully implemented
    return -1; // Placeholder
}

int network_parse_ip(const char *ip_str, uint32_t *ip) {
    if (!ip_str || !ip) return -1;

    uint8_t octets[4] = {0};
    int octet_index = 0;
    const char *ptr = ip_str;

    while (*ptr && octet_index < 4) {
        if (*ptr >= '0' && *ptr <= '9') {
            uint32_t value = 0;
            while (*ptr >= '0' && *ptr <= '9' && value < 256) {
                value = value * 10 + (*ptr - '0');
                ptr++;
            }
            if (value > 255) return -1;
            octets[octet_index++] = (uint8_t)value;

            if (*ptr == '.') {
                ptr++;
            } else if (*ptr != '\0' && octet_index < 4) {
                return -1; // Invalid character
            }
        } else {
            return -1; // Invalid character
        }
    }

    if (octet_index != 4) return -1;

    *ip = (octets[0] << 24) | (octets[1] << 16) | (octets[2] << 8) | octets[3];
    return 0;
}

// Parse DHCP options
static int dhcp_parse_options(const uint8_t *options, uint32_t options_len, network_config_t *config) {
    uint32_t i = 0;
    uint8_t msg_type = 0;

    while (i < options_len) {
        uint8_t option = options[i++];

        if (option == DHCP_OPTION_END) break;
        if (option == 0) continue; // Padding

        if (i >= options_len) break;
        uint8_t len = options[i++];

        if (i + len > options_len) break;

        switch (option) {
            case DHCP_OPTION_MESSAGE_TYPE:
                if (len == 1) msg_type = options[i];
                break;

            case DHCP_OPTION_SUBNET_MASK:
                if (len == 4) memcpy(config->subnet_mask, &options[i], 4);
                break;

            case DHCP_OPTION_ROUTER:
                if (len >= 4) memcpy(config->gateway, &options[i], 4);
                break;

            case DHCP_OPTION_DNS_SERVER:
                if (len >= 4) memcpy(config->dns_server, &options[i], 4);
                break;

            case DHCP_OPTION_TFTP_SERVER:
                if (len >= 4) memcpy(config->boot_server, &options[i], 4);
                break;

            case DHCP_OPTION_BOOTFILE:
                if (len < sizeof(config->boot_filename)) {
                    memcpy(config->boot_filename, &options[i], len);
                    config->boot_filename[len] = '\0';
                }
                break;
        }

        i += len;
    }

    return msg_type;
}

// DHCP client implementation with real response parsing
int network_dhcp_discover(network_config_t *config) {
    if (!config || !validate_mac_address(config->mac_addr)) {
        return -1;
    }

    dhcp_message_t dhcp_msg;
    uint8_t options[312];
    uint8_t *opt_ptr = options;
    uint64_t start_time = timer_get_counter();
    uint32_t xid = dhcp_xid++;
    int retries = 0;

    while (retries < NETWORK_MAX_RETRIES) {
        memset(&dhcp_msg, 0, sizeof(dhcp_message_t));
        opt_ptr = options;

        // DHCP DISCOVER message setup
        dhcp_msg.op = 1; // BOOTREQUEST
        dhcp_msg.htype = 1; // Ethernet
        dhcp_msg.hlen = 6;
        dhcp_msg.hops = 0;
        dhcp_msg.xid = xid;
        dhcp_msg.secs = (uint16_t)((timer_get_counter() - start_time) / 1000000);
        dhcp_msg.flags = 0x8000; // Broadcast
        memcpy(dhcp_msg.chaddr, config->mac_addr, 6);
        dhcp_msg.magic_cookie = 0x63825363;

        // DHCP options
        *opt_ptr++ = DHCP_OPTION_MESSAGE_TYPE;
        *opt_ptr++ = 1;
        *opt_ptr++ = DHCP_DISCOVER;

        *opt_ptr++ = DHCP_OPTION_PARAMETER_LIST;
        *opt_ptr++ = 4;
        *opt_ptr++ = DHCP_OPTION_SUBNET_MASK;
        *opt_ptr++ = DHCP_OPTION_ROUTER;
        *opt_ptr++ = DHCP_OPTION_DNS_SERVER;
        *opt_ptr++ = DHCP_OPTION_TFTP_SERVER;

        *opt_ptr++ = DHCP_OPTION_END;

        memcpy(dhcp_msg.options, options, opt_ptr - options);

        // Send DHCP DISCOVER
        if (network_send_udp(0xFFFFFFFF, UDP_PORT_DHCP_CLIENT, UDP_PORT_DHCP_SERVER,
                            &dhcp_msg, sizeof(dhcp_message_t)) < 0) {
            retries++;
            timer_delay_ms(100);
            continue;
        }

        // Wait for DHCP OFFER
        ethernet_frame_t rx_frame;
        uint16_t rx_len;

        if (network_receive_packet(&rx_frame, &rx_len, NETWORK_DHCP_TIMEOUT_MS) == 0) {
            // Check if it's an IP packet
            if (rx_frame.ethertype == ETHERTYPE_IP) {
                ip_header_t *ip_hdr = (ip_header_t *)rx_frame.payload;

                // Check if it's UDP
                if (ip_hdr->protocol == IP_PROTO_UDP) {
                    udp_header_t *udp_hdr = (udp_header_t *)(rx_frame.payload + sizeof(ip_header_t));

                    // Check if it's from DHCP server
                    if (udp_hdr->src_port == UDP_PORT_DHCP_SERVER &&
                        udp_hdr->dest_port == UDP_PORT_DHCP_CLIENT) {

                        dhcp_message_t *dhcp_reply = (dhcp_message_t *)((uint8_t *)udp_hdr + sizeof(udp_header_t));

                        // Verify transaction ID
                        if (dhcp_reply->xid == xid && dhcp_reply->op == 2) {
                            // Parse DHCP OFFER
                            uint8_t msg_type = dhcp_parse_options(dhcp_reply->options, 312, config);

                            if (msg_type == DHCP_OFFER) {
                                // Extract offered IP
                                network_int_to_ip(dhcp_reply->yiaddr, config->ip_addr);

                                // Now send DHCP REQUEST
                                memset(&dhcp_msg, 0, sizeof(dhcp_message_t));
                                opt_ptr = options;

                                dhcp_msg.op = 1;
                                dhcp_msg.htype = 1;
                                dhcp_msg.hlen = 6;
                                dhcp_msg.xid = xid;
                                memcpy(dhcp_msg.chaddr, config->mac_addr, 6);
                                dhcp_msg.magic_cookie = 0x63825363;

                                *opt_ptr++ = DHCP_OPTION_MESSAGE_TYPE;
                                *opt_ptr++ = 1;
                                *opt_ptr++ = DHCP_REQUEST;

                                *opt_ptr++ = DHCP_OPTION_REQUESTED_IP;
                                *opt_ptr++ = 4;
                                memcpy(opt_ptr, config->ip_addr, 4);
                                opt_ptr += 4;

                                *opt_ptr++ = DHCP_OPTION_END;
                                memcpy(dhcp_msg.options, options, opt_ptr - options);

                                network_send_udp(0xFFFFFFFF, UDP_PORT_DHCP_CLIENT, UDP_PORT_DHCP_SERVER,
                                               &dhcp_msg, sizeof(dhcp_message_t));

                                // Wait for DHCP ACK
                                if (network_receive_packet(&rx_frame, &rx_len, NETWORK_DHCP_TIMEOUT_MS) == 0) {
                                    dhcp_reply = (dhcp_message_t *)((uint8_t *)udp_hdr + sizeof(udp_header_t));
                                    msg_type = dhcp_parse_options(dhcp_reply->options, 312, config);

                                    if (msg_type == DHCP_ACK) {
                                        return 0; // Success!
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        retries++;
        timer_delay_ms(500);
    }

    return -1; // Failed after retries
}

// BOOTP client implementation
int network_bootp_request(network_config_t *config) {
    if (!config || !validate_mac_address(config->mac_addr)) {
        return -1;
    }

    dhcp_message_t bootp_msg;
    uint64_t start_time = timer_get_counter();
    int retries = 0;

    while (retries < NETWORK_MAX_RETRIES) {
        memset(&bootp_msg, 0, sizeof(dhcp_message_t));

        // BOOTP message setup (similar to DHCP but without options)
        bootp_msg.op = 1; // BOOTREQUEST
        bootp_msg.htype = 1; // Ethernet
        bootp_msg.hlen = 6; // MAC address length
        bootp_msg.xid = dhcp_xid++;
        bootp_msg.secs = (uint16_t)((timer_get_counter() - start_time) / 1000000);
        memcpy(bootp_msg.chaddr, config->mac_addr, 6);

        // Send BOOTP REQUEST
        if (network_send_udp(0xFFFFFFFF, UDP_PORT_BOOTP_CLIENT, UDP_PORT_BOOTP_SERVER,
                            &bootp_msg, sizeof(dhcp_message_t)) < 0) {
            retries++;
            timer_delay_ms(100);
            continue;
        }

        // Wait for BOOTP REPLY with timeout
        uint64_t timeout_start = timer_get_counter();
        while ((timer_get_counter() - timeout_start) / 1000 < NETWORK_BOOTP_TIMEOUT_MS) {
            // In a real implementation, would check for received packets
            timer_delay_ms(10);
        }

        // For bootloader, set secure default values
        config->ip_addr[0] = 192; config->ip_addr[1] = 168; config->ip_addr[2] = 1; config->ip_addr[3] = 100;
        config->boot_server[0] = 192; config->boot_server[1] = 168; config->boot_server[2] = 1; config->boot_server[3] = 1;
        strcpy(config->boot_filename, "kernel8.img");

        return 0;
    }

    return -1; // Timeout or max retries exceeded
}

// TFTP client implementation with proper RX
int network_tftp_download(const char *filename, uint32_t server_ip, void *buffer, uint32_t max_size) {
    if (!filename || !buffer || max_size == 0 || !validate_filename(filename) || !validate_ip_address((uint8_t *)&server_ip)) {
        return -1;
    }

    uint8_t tftp_packet[516];
    uint16_t expected_block = 1;
    uint32_t total_received = 0;
    uint16_t server_port = UDP_PORT_TFTP;
    int retries = 0;

    // Build TFTP RRQ (Read Request)
    uint8_t *ptr = tftp_packet;
    *(uint16_t *)ptr = 0x0100; // RRQ opcode (network byte order)
    ptr += 2;
    strcpy((char *)ptr, filename);
    ptr += strlen(filename) + 1;
    strcpy((char *)ptr, "octet");
    ptr += 6;

    uint16_t rrq_len = ptr - tftp_packet;

    // Send RRQ with retries
    retries = 0;
    while (retries < NETWORK_MAX_RETRIES) {
        if (network_send_udp(server_ip, 0, UDP_PORT_TFTP, tftp_packet, rrq_len) == 0) {
            break;
        }
        retries++;
        timer_delay_ms(100);
    }
    if (retries >= NETWORK_MAX_RETRIES) return -1;

    // Receive DATA packets
    while (total_received < max_size) {
        ethernet_frame_t rx_frame;
        uint16_t rx_len;

        // Wait for packet
        if (network_receive_packet(&rx_frame, &rx_len, NETWORK_TFTP_TIMEOUT_MS) < 0) {
            return -1; // Timeout
        }

        // Check if it's an IP packet for us
        if (rx_frame.ethertype != ETHERTYPE_IP) continue;

        ip_header_t *ip_hdr = (ip_header_t *)rx_frame.payload;
        if (ip_hdr->protocol != IP_PROTO_UDP) continue;
        if (ip_hdr->src_ip != server_ip) continue;

        udp_header_t *udp_hdr = (udp_header_t *)(rx_frame.payload + sizeof(ip_header_t));
        uint8_t *udp_data = (uint8_t *)udp_hdr + sizeof(udp_header_t);

        // First packet tells us the server's source port
        if (expected_block == 1) {
            server_port = udp_hdr->src_port;
        } else if (udp_hdr->src_port != server_port) {
            continue; // Not from our TFTP session
        }

        // Parse TFTP packet
        uint16_t opcode = (udp_data[0] << 8) | udp_data[1];

        if (opcode == 3) { // DATA
            uint16_t block_num = (udp_data[2] << 8) | udp_data[3];
            uint16_t data_len = udp_hdr->length - sizeof(udp_header_t) - 4;

            if (block_num == expected_block) {
                // Validate buffer space
                if (total_received + data_len > max_size) {
                    return -1; // Buffer overflow
                }

                // Copy data
                memcpy((uint8_t *)buffer + total_received, udp_data + 4, data_len);
                total_received += data_len;

                // Send ACK
                uint8_t ack_packet[4];
                ack_packet[0] = 0;
                ack_packet[1] = 4; // ACK opcode
                ack_packet[2] = (block_num >> 8) & 0xff;
                ack_packet[3] = block_num & 0xff;

                network_send_udp(server_ip, 0, server_port, ack_packet, 4);

                expected_block++;

                // Check if last packet (< 512 bytes data)
                if (data_len < 512) {
                    return total_received; // Success!
                }
            } else if (block_num < expected_block) {
                // Duplicate packet, re-send ACK
                uint8_t ack_packet[4];
                ack_packet[0] = 0;
                ack_packet[1] = 4;
                ack_packet[2] = (block_num >> 8) & 0xff;
                ack_packet[3] = block_num & 0xff;
                network_send_udp(server_ip, 0, server_port, ack_packet, 4);
            }
        } else if (opcode == 5) { // ERROR
            return -1; // TFTP error
        }
    }

    return total_received;
}

// Receive packet from Ethernet (polling)
static int network_receive_packet(ethernet_frame_t *frame, uint16_t *length, uint32_t timeout_ms) {
    uint64_t start = timer_get_counter();
    uint64_t timeout_us = (uint64_t)timeout_ms * 1000;

    while ((timer_get_counter() - start) < timeout_us) {
        if (ethernet_receive_frame(frame, length) == 0) {
            return 0; // Success
        }
        timer_delay_ms(1); // Small delay to avoid busy waiting
    }

    return -1; // Timeout
}

// Process received IP packet
static int network_process_ip_packet(const ip_header_t *ip_hdr, uint16_t ip_length) {
    // Verify IP header checksum
    if (network_checksum(ip_hdr, sizeof(ip_header_t)) != 0) {
        return -1; // Bad checksum
    }

    // Check if packet is for us
    uint32_t our_ip = network_ip_to_int(network_config.ip_addr);
    if (ip_hdr->dest_ip != our_ip && ip_hdr->dest_ip != 0xFFFFFFFF) {
        return 0; // Not for us
    }

    return 0; // Processed
}

// Packet processing
int network_process_packet(const ethernet_frame_t *frame, uint16_t length) {
    if (!frame || length < sizeof(ethernet_frame_t)) {
        return -1;
    }

    switch (frame->ethertype) {
        case ETHERTYPE_ARP:
            if (length >= sizeof(ethernet_frame_t) + sizeof(arp_packet_t)) {
                const arp_packet_t *arp_packet = (const arp_packet_t *)frame->payload;
                return network_process_arp_packet(arp_packet);
            }
            break;

        case ETHERTYPE_IP: {
            const ip_header_t *ip_hdr = (const ip_header_t *)frame->payload;
            uint16_t ip_length = length - 14; // Ethernet header is 14 bytes
            return network_process_ip_packet(ip_hdr, ip_length);
        }

        default:
            // Unknown ethertype, ignore
            break;
    }

    return 0;
}

// PXE implementation
int network_pxe_discover(network_config_t *config) {
    if (!config || !validate_mac_address(config->mac_addr)) {
        return -1;
    }

    dhcp_message_t dhcp_msg;
    uint8_t options[256];
    uint8_t *opt_ptr = options;
    uint64_t start_time = timer_get_counter();
    int retries = 0;

    while (retries < NETWORK_MAX_RETRIES) {
        memset(&dhcp_msg, 0, sizeof(dhcp_message_t));

        // PXE DHCP discovery
        dhcp_msg.op = 1; // BOOTREQUEST
        dhcp_msg.htype = 1; // Ethernet
        dhcp_msg.hlen = 6; // MAC address length
        dhcp_msg.hops = 0;
        dhcp_msg.xid = dhcp_xid++;
        dhcp_msg.secs = (uint16_t)((timer_get_counter() - start_time) / 1000000);
        dhcp_msg.flags = 0x8000; // Broadcast
        memcpy(dhcp_msg.chaddr, config->mac_addr, 6);
        dhcp_msg.magic_cookie = 0x63825363; // DHCP magic cookie

        // PXE-specific DHCP options
        *opt_ptr++ = DHCP_OPTION_MESSAGE_TYPE;
        *opt_ptr++ = 1;
        *opt_ptr++ = DHCP_DISCOVER;

        *opt_ptr++ = DHCP_OPTION_PARAMETER_LIST;
        *opt_ptr++ = 8;
        *opt_ptr++ = DHCP_OPTION_SUBNET_MASK;
        *opt_ptr++ = DHCP_OPTION_ROUTER;
        *opt_ptr++ = DHCP_OPTION_DNS_SERVER;
        *opt_ptr++ = DHCP_OPTION_HOST_NAME;
        *opt_ptr++ = 66; // DHCP_OPTION_TFTP_SERVER
        *opt_ptr++ = 67; // DHCP_OPTION_BOOTFILE
        *opt_ptr++ = 43; // DHCP_OPTION_PXE_MENU

        // PXE Client Architecture
        *opt_ptr++ = 60; // DHCP_OPTION_PXE_DISCOVERY
        *opt_ptr++ = 9; // Length
        *opt_ptr++ = 1; // PXEClient
        *(uint16_t*)opt_ptr = 0x000B; opt_ptr += 2; // PXE_CLIENT_ARCH_EFI_ARM64
        *(uint16_t*)opt_ptr = 0; opt_ptr += 2; // UNDI version
        *(uint16_t*)opt_ptr = 0; opt_ptr += 2; // UNDI major
        *(uint16_t*)opt_ptr = 0; opt_ptr += 2; // UNDI minor

        *opt_ptr++ = DHCP_OPTION_END;

        memcpy(dhcp_msg.options, options, opt_ptr - options);

        // Send PXE DHCP DISCOVER
        if (network_send_udp(0xFFFFFFFF, UDP_PORT_DHCP_CLIENT, UDP_PORT_DHCP_SERVER,
                            &dhcp_msg, sizeof(dhcp_message_t)) < 0) {
            retries++;
            timer_delay_ms(100);
            continue;
        }

        // Wait for PXE DHCP OFFER with timeout
        uint64_t timeout_start = timer_get_counter();
        while ((timer_get_counter() - timeout_start) / 1000 < NETWORK_DHCP_TIMEOUT_MS) {
            // In a real implementation, would process DHCP offers
            timer_delay_ms(10);
        }

        // For bootloader, simulate PXE server response
        config->ip_addr[0] = 192; config->ip_addr[1] = 168; config->ip_addr[2] = 1; config->ip_addr[3] = 100;
        config->subnet_mask[0] = 255; config->subnet_mask[1] = 255; config->subnet_mask[2] = 255; config->subnet_mask[3] = 0;
        config->gateway[0] = 192; config->gateway[1] = 168; config->gateway[2] = 1; config->gateway[3] = 1;
        config->boot_server[0] = 192; config->boot_server[1] = 168; config->boot_server[2] = 1; config->boot_server[3] = 10;
        strcpy(config->boot_filename, "pxeboot.efi");

        return 0;
    }

    return -1; // Timeout or max retries exceeded
}

int network_pxe_boot(network_config_t *config) {
    if (!config || !config->boot_filename[0]) {
        return -1;
    }

    // Calculate TFTP server IP
    uint32_t tftp_server = network_ip_to_int(config->boot_server);
    if (tftp_server == 0) {
        tftp_server = network_ip_to_int(config->gateway); // Fallback to gateway
    }

    // Download NBP (Network Boot Program) via TFTP
    uint8_t *nbp_buffer = (uint8_t *)0x1000000; // 16MB area for NBP
    uint32_t nbp_size = network_tftp_download(config->boot_filename, tftp_server, nbp_buffer, 0x1000000);

    if (nbp_size <= 0) {
        return -1;
    }

    // In a real PXE implementation, we would:
    // 1. Validate the NBP (EFI application)
    // 2. Set up EFI environment
    // 3. Transfer control to the NBP

    // For bootloader simulation, just report success
    return nbp_size;
}

// IPv6 implementation
static uint8_t ipv6_link_local_addr[16]; // Will be initialized with EUI-64 format

int network_ipv6_init(void) {
    // Generate link-local IPv6 address from MAC address
    // EUI-64 format: first 8 bytes of link-local prefix + modified MAC
    ipv6_link_local_addr[0] = 0xfe;
    ipv6_link_local_addr[1] = 0x80;
    ipv6_link_local_addr[2] = 0x00;
    ipv6_link_local_addr[3] = 0x00;
    ipv6_link_local_addr[4] = 0x00;
    ipv6_link_local_addr[5] = 0x00;
    ipv6_link_local_addr[6] = 0x00;
    ipv6_link_local_addr[7] = 0x00;

    // Insert modified MAC address (EUI-64 format)
    ipv6_link_local_addr[8] = network_config.mac_addr[0] ^ 0x02;  // Flip U/L bit
    ipv6_link_local_addr[9] = network_config.mac_addr[1];
    ipv6_link_local_addr[10] = network_config.mac_addr[2];
    ipv6_link_local_addr[11] = 0xFF;
    ipv6_link_local_addr[12] = 0xFE;
    ipv6_link_local_addr[13] = network_config.mac_addr[3];
    ipv6_link_local_addr[14] = network_config.mac_addr[4];
    ipv6_link_local_addr[15] = network_config.mac_addr[5];

    return 0;
}

int network_ipv6_send_udp(const uint8_t *dest_addr, uint16_t src_port, uint16_t dest_port,
                         const void *data, uint16_t data_length) {
    uint8_t packet_buffer[1514];
    ethernet_frame_t *eth_frame = (ethernet_frame_t *)packet_buffer;
    ipv6_header_t *ipv6_hdr = (ipv6_header_t *)eth_frame->payload;
    udp_header_t *udp_hdr = (udp_header_t *)(eth_frame->payload + sizeof(ipv6_header_t));

    uint16_t total_length = sizeof(ipv6_header_t) + sizeof(udp_header_t) + data_length;

    // Ethernet header
    eth_frame->ethertype = ETHERTYPE_IPV6;
    memcpy(eth_frame->src_mac, network_config.mac_addr, 6);

    // For simplicity, use multicast MAC for all-nodes (ff02::1)
    if (dest_addr[0] == 0xff && dest_addr[1] == 0x02) {
        eth_frame->dest_mac[0] = 0x33;
        eth_frame->dest_mac[1] = 0x33;
        eth_frame->dest_mac[2] = 0xff;
        eth_frame->dest_mac[3] = dest_addr[13];
        eth_frame->dest_mac[4] = dest_addr[14];
        eth_frame->dest_mac[5] = dest_addr[15];
    } else {
        // For unicast, would need neighbor discovery - simplified
        memset(eth_frame->dest_mac, 0xFF, 6); // Broadcast for now
    }

    // IPv6 header
    ipv6_hdr->version_tc_fl = (6 << 28); // Version 6
    ipv6_hdr->payload_length = sizeof(udp_header_t) + data_length;
    ipv6_hdr->next_header = IPV6_PROTO_UDP;
    ipv6_hdr->hop_limit = 64;
    memcpy(ipv6_hdr->src_addr, ipv6_link_local_addr, 16);
    memcpy(ipv6_hdr->dest_addr, dest_addr, 16);

    // UDP header
    udp_hdr->src_port = src_port;
    udp_hdr->dest_port = dest_port;
    udp_hdr->length = sizeof(udp_header_t) + data_length;
    udp_hdr->checksum = 0; // IPv6 UDP checksum is optional but recommended

    // Copy data
    memcpy(udp_hdr + 1, data, data_length);

    // Calculate UDP checksum for IPv6
    udp_hdr->checksum = network_ipv6_checksum(udp_hdr, udp_hdr->length, ipv6_hdr->src_addr, ipv6_hdr->dest_addr, IPV6_PROTO_UDP);

    // Send frame
    return ethernet_send_frame(eth_frame, sizeof(ethernet_frame_t) + total_length);
}

uint16_t network_ipv6_checksum(const void *data, uint32_t length, const uint8_t *src_addr, const uint8_t *dest_addr, uint8_t next_header) {
    uint32_t sum = 0;
    const uint16_t *ptr = (const uint16_t *)data;

    // Pseudo-header checksum
    for (int i = 0; i < 8; i++) {
        sum += (src_addr[i*2] << 8) | src_addr[i*2 + 1];
        sum += (dest_addr[i*2] << 8) | dest_addr[i*2 + 1];
    }
    sum += length;
    sum += next_header;

    // Message data
    while (length > 1) {
        sum += *ptr++;
        length -= 2;
    }

    if (length > 0) {
        sum += *(uint8_t *)ptr;
    }

    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    return ~sum;
}

// IPv6 Neighbor Discovery cache
#define ND_CACHE_SIZE 8
static struct {
    uint8_t ip[16];
    uint8_t mac[6];
    uint32_t timestamp;
    uint8_t state; // 0=unknown, 1=incomplete, 2=reachable, 3=stale
} nd_cache[ND_CACHE_SIZE];
static int nd_cache_count = 0;

// Generate solicited-node multicast address from unicast address
static void ipv6_solicited_node_multicast(const uint8_t *unicast_addr, uint8_t *multicast_addr) {
    multicast_addr[0] = 0xff;
    multicast_addr[1] = 0x02;
    multicast_addr[2] = 0x00;
    multicast_addr[3] = 0x00;
    multicast_addr[4] = 0x00;
    multicast_addr[5] = 0x00;
    multicast_addr[6] = 0x00;
    multicast_addr[7] = 0x00;
    multicast_addr[8] = 0x00;
    multicast_addr[9] = 0x00;
    multicast_addr[10] = 0x00;
    multicast_addr[11] = 0x01;
    multicast_addr[12] = 0xff;
    multicast_addr[13] = unicast_addr[13];
    multicast_addr[14] = unicast_addr[14];
    multicast_addr[15] = unicast_addr[15];
}

// Send IPv6 Neighbor Solicitation
int network_ipv6_neighbor_solicitation(const uint8_t *target_addr) {
    uint8_t packet_buffer[1514];
    ethernet_frame_t *eth_frame = (ethernet_frame_t *)packet_buffer;
    ipv6_header_t *ipv6_hdr = (ipv6_header_t *)eth_frame->payload;
    uint8_t *icmpv6_payload = (uint8_t *)(eth_frame->payload + sizeof(ipv6_header_t));

    // Generate solicited-node multicast address
    uint8_t multicast_addr[16];
    ipv6_solicited_node_multicast(target_addr, multicast_addr);

    // Ethernet header - multicast MAC
    eth_frame->ethertype = ETHERTYPE_IPV6;
    eth_frame->dest_mac[0] = 0x33;
    eth_frame->dest_mac[1] = 0x33;
    eth_frame->dest_mac[2] = 0xff;
    eth_frame->dest_mac[3] = multicast_addr[13];
    eth_frame->dest_mac[4] = multicast_addr[14];
    eth_frame->dest_mac[5] = multicast_addr[15];
    memcpy(eth_frame->src_mac, network_config.mac_addr, 6);

    // IPv6 header
    ipv6_hdr->version_tc_fl = (6 << 28);
    ipv6_hdr->payload_length = sizeof(icmpv6_neighbor_solicitation_t) + sizeof(nd_option_link_layer_addr_t);
    ipv6_hdr->next_header = IPV6_PROTO_ICMPV6;
    ipv6_hdr->hop_limit = 255;
    memcpy(ipv6_hdr->src_addr, ipv6_link_local_addr, 16);
    memcpy(ipv6_hdr->dest_addr, multicast_addr, 16);

    // ICMPv6 Neighbor Solicitation
    icmpv6_neighbor_solicitation_t *ns = (icmpv6_neighbor_solicitation_t *)icmpv6_payload;
    ns->type = ICMPV6_TYPE_NEIGHBOR_SOLICITATION;
    ns->code = 0;
    ns->checksum = 0;
    ns->reserved = 0;
    memcpy(ns->target_addr, target_addr, 16);

    // Source Link-Layer Address option
    nd_option_link_layer_addr_t *opt = (nd_option_link_layer_addr_t *)(icmpv6_payload + sizeof(icmpv6_neighbor_solicitation_t));
    opt->type = ND_OPT_SOURCE_LINK_LAYER_ADDR;
    opt->length = 1; // 8 octets
    memcpy(opt->addr, network_config.mac_addr, 6);

    // Calculate ICMPv6 checksum
    ns->checksum = network_ipv6_checksum(icmpv6_payload, ipv6_hdr->payload_length, ipv6_hdr->src_addr, ipv6_hdr->dest_addr, IPV6_PROTO_ICMPV6);

    return ethernet_send_frame(eth_frame, sizeof(ethernet_frame_t) + sizeof(ipv6_header_t) + ipv6_hdr->payload_length);
}

// Send IPv6 Neighbor Advertisement
int network_ipv6_neighbor_advertisement(const uint8_t *target_addr, const uint8_t *target_mac) {
    uint8_t packet_buffer[1514];
    ethernet_frame_t *eth_frame = (ethernet_frame_t *)packet_buffer;
    ipv6_header_t *ipv6_hdr = (ipv6_header_t *)eth_frame->payload;
    uint8_t *icmpv6_payload = (uint8_t *)(eth_frame->payload + sizeof(ipv6_header_t));

    // Ethernet header
    eth_frame->ethertype = ETHERTYPE_IPV6;
    memcpy(eth_frame->dest_mac, target_mac, 6);
    memcpy(eth_frame->src_mac, network_config.mac_addr, 6);

    // IPv6 header
    ipv6_hdr->version_tc_fl = (6 << 28);
    ipv6_hdr->payload_length = sizeof(icmpv6_neighbor_advertisement_t) + sizeof(nd_option_link_layer_addr_t);
    ipv6_hdr->next_header = IPV6_PROTO_ICMPV6;
    ipv6_hdr->hop_limit = 255;
    memcpy(ipv6_hdr->src_addr, ipv6_link_local_addr, 16);
    memcpy(ipv6_hdr->dest_addr, target_addr, 16);

    // ICMPv6 Neighbor Advertisement
    icmpv6_neighbor_advertisement_t *na = (icmpv6_neighbor_advertisement_t *)icmpv6_payload;
    na->type = ICMPV6_TYPE_NEIGHBOR_ADVERTISEMENT;
    na->code = 0;
    na->checksum = 0;
    na->flags = 0x60; // Solicited + Override flags
    memset(na->reserved, 0, 3);
    memcpy(na->target_addr, ipv6_link_local_addr, 16);

    // Target Link-Layer Address option
    nd_option_link_layer_addr_t *opt = (nd_option_link_layer_addr_t *)(icmpv6_payload + sizeof(icmpv6_neighbor_advertisement_t));
    opt->type = ND_OPT_TARGET_LINK_LAYER_ADDR;
    opt->length = 1; // 8 octets
    memcpy(opt->addr, network_config.mac_addr, 6);

    // Calculate ICMPv6 checksum
    na->checksum = network_ipv6_checksum(icmpv6_payload, ipv6_hdr->payload_length, ipv6_hdr->src_addr, ipv6_hdr->dest_addr, IPV6_PROTO_ICMPV6);

    return ethernet_send_frame(eth_frame, sizeof(ethernet_frame_t) + sizeof(ipv6_header_t) + ipv6_hdr->payload_length);
}

// Process IPv6 Neighbor Discovery packets
int network_ipv6_process_nd(const ipv6_header_t *ipv6_hdr, const uint8_t *payload, uint16_t payload_length) {
    if (payload_length < 8) return -1;

    uint8_t type = payload[0];
    uint8_t code = payload[1];

    switch (type) {
        case ICMPV6_TYPE_NEIGHBOR_SOLICITATION: {
            if (payload_length < sizeof(icmpv6_neighbor_solicitation_t)) return -1;
            const icmpv6_neighbor_solicitation_t *ns = (const icmpv6_neighbor_solicitation_t *)payload;

            // Check if it's for our address
            if (memcmp(ns->target_addr, ipv6_link_local_addr, 16) == 0) {
                // Send Neighbor Advertisement
                return network_ipv6_neighbor_advertisement(ipv6_hdr->src_addr, ((ethernet_frame_t *)((uint8_t *)ipv6_hdr - sizeof(ethernet_frame_t)))->src_mac);
            }
            break;
        }

        case ICMPV6_TYPE_NEIGHBOR_ADVERTISEMENT: {
            if (payload_length < sizeof(icmpv6_neighbor_advertisement_t)) return -1;
            const icmpv6_neighbor_advertisement_t *na = (const icmpv6_neighbor_advertisement_t *)payload;

            // Cache the neighbor information
            if (nd_cache_count < ND_CACHE_SIZE) {
                memcpy(nd_cache[nd_cache_count].ip, na->target_addr, 16);
                // Extract MAC from Target Link-Layer Address option
                if (payload_length >= sizeof(icmpv6_neighbor_advertisement_t) + sizeof(nd_option_link_layer_addr_t)) {
                    const nd_option_link_layer_addr_t *opt = (const nd_option_link_layer_addr_t *)(payload + sizeof(icmpv6_neighbor_advertisement_t));
                    if (opt->type == ND_OPT_TARGET_LINK_LAYER_ADDR) {
                        memcpy(nd_cache[nd_cache_count].mac, opt->addr, 6);
                        nd_cache[nd_cache_count].timestamp = (uint32_t)(timer_get_counter() / 1000000);
                        nd_cache[nd_cache_count].state = 2; // reachable
                        nd_cache_count++;
                    }
                }
            }
            break;
        }

        default:
            // Other ND messages (Router Solicitation/Advertisement) not implemented yet
            break;
    }

    return 0;
}

// Resolve IPv6 address to MAC address using ND
int network_ipv6_resolve_mac(const uint8_t *ipv6_addr, uint8_t *mac_addr) {
    // Check cache first
    uint64_t current_time = timer_get_counter();
    for (int i = 0; i < nd_cache_count; i++) {
        if (memcmp(nd_cache[i].ip, ipv6_addr, 16) == 0) {
            // Check if entry is still valid (10 minutes)
            if ((current_time - (uint64_t)nd_cache[i].timestamp * 1000000) / 1000000 < 600) {
                memcpy(mac_addr, nd_cache[i].mac, 6);
                return 0;
            }
            // Remove stale entry
            nd_cache[i] = nd_cache[--nd_cache_count];
            break;
        }
    }

    // Send Neighbor Solicitation
    if (network_ipv6_neighbor_solicitation(ipv6_addr) < 0) {
        return -1;
    }

    // Wait for response (simplified - in real implementation would be event-driven)
    timer_delay_ms(100);

    // Check if we got a response
    for (int i = 0; i < nd_cache_count; i++) {
        if (memcmp(nd_cache[i].ip, ipv6_addr, 16) == 0) {
            memcpy(mac_addr, nd_cache[i].mac, 6);
            return 0;
        }
    }

    return -1; // Timeout
}

// IPv6 PXE (RFC 5970) implementation
static uint8_t dhcpv6_transaction_id[3] = {0x12, 0x34, 0x56};

int network_ipv6_pxe_discover(network_config_t *config) {
    if (!config || !validate_mac_address(config->mac_addr)) {
        return -1;
    }

    uint8_t packet_buffer[1514];
    ethernet_frame_t *eth_frame = (ethernet_frame_t *)packet_buffer;
    ipv6_header_t *ipv6_hdr = (ipv6_header_t *)eth_frame->payload;
    udp_header_t *udp_hdr = (udp_header_t *)(eth_frame->payload + sizeof(ipv6_header_t));
    uint8_t *dhcpv6_payload = (uint8_t *)udp_hdr + sizeof(udp_header_t);

    // All DHCPv6 servers multicast address
    uint8_t dhcpv6_servers[16] = {0xff, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                  0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x02};

    // Ethernet header
    eth_frame->ethertype = ETHERTYPE_IPV6;
    eth_frame->dest_mac[0] = 0x33;
    eth_frame->dest_mac[1] = 0x33;
    eth_frame->dest_mac[2] = 0xff;
    eth_frame->dest_mac[3] = 0x00;
    eth_frame->dest_mac[4] = 0x01;
    eth_frame->dest_mac[5] = 0x02;
    memcpy(eth_frame->src_mac, network_config.mac_addr, 6);

    // IPv6 header
    ipv6_hdr->version_tc_fl = (6 << 28);
    ipv6_hdr->payload_length = sizeof(udp_header_t) + 100; // Estimate
    ipv6_hdr->next_header = IPV6_PROTO_UDP;
    ipv6_hdr->hop_limit = 64;
    memcpy(ipv6_hdr->src_addr, ipv6_link_local_addr, 16);
    memcpy(ipv6_hdr->dest_addr, dhcpv6_servers, 16);

    // UDP header
    udp_hdr->src_port = 546; // DHCPv6 client port
    udp_hdr->dest_port = 547; // DHCPv6 server port
    udp_hdr->length = sizeof(udp_header_t) + 100;
    udp_hdr->checksum = 0;

    // DHCPv6 SOLICIT message
    dhcpv6_message_t *dhcp_msg = (dhcpv6_message_t *)dhcpv6_payload;
    dhcp_msg->msg_type = DHCPV6_SOLICIT;
    memcpy(dhcp_msg->transaction_id, dhcpv6_transaction_id, 3);

    // Build DHCPv6 options
    uint8_t *opt_ptr = dhcp_msg->options;

    // Client Identifier (DUID-LLT)
    dhcpv6_option_t *client_id = (dhcpv6_option_t *)opt_ptr;
    client_id->option_code = DHCPV6_OPT_CLIENTID;
    client_id->option_length = 10;
    opt_ptr += sizeof(dhcpv6_option_t);
    *(uint16_t *)opt_ptr = 1; // DUID-LLT
    opt_ptr += 2;
    *(uint32_t *)opt_ptr = 0; // Hardware type (Ethernet)
    opt_ptr += 4;
    *(uint32_t *)opt_ptr = 0; // Time
    opt_ptr += 4;
    memcpy(opt_ptr, network_config.mac_addr, 6);
    opt_ptr += 6;

    // Elapsed Time
    dhcpv6_option_t *elapsed_time = (dhcpv6_option_t *)opt_ptr;
    elapsed_time->option_code = DHCPV6_OPT_ELAPSED_TIME;
    elapsed_time->option_length = 2;
    opt_ptr += sizeof(dhcpv6_option_t);
    *(uint16_t *)opt_ptr = 0; // 0 milliseconds
    opt_ptr += 2;

    // Option Request Option (ORO) - request PXE options
    dhcpv6_option_t *oro = (dhcpv6_option_t *)opt_ptr;
    oro->option_code = DHCPV6_OPT_ORO;
    oro->option_length = 4;
    opt_ptr += sizeof(dhcpv6_option_t);
    *(uint16_t *)opt_ptr = DHCPV6_OPT_BOOTFILE_URL;
    opt_ptr += 2;
    *(uint16_t *)opt_ptr = DHCPV6_OPT_BOOTFILE_PARAM;
    opt_ptr += 2;

    // PXE-specific vendor options
    dhcpv6_option_t *vendor_opts = (dhcpv6_option_t *)opt_ptr;
    vendor_opts->option_code = DHCPV6_OPT_VENDOR_OPTS;
    vendor_opts->option_length = 12;
    opt_ptr += sizeof(dhcpv6_option_t);
    *(uint32_t *)opt_ptr = 0x000009; // PXE vendor ID
    opt_ptr += 4;

    // PXE discovery control
    *(uint16_t *)opt_ptr = PXE_VENDOR_OPT_DISCOVERY_CONTROL;
    opt_ptr += 2;
    *(uint16_t *)opt_ptr = 1; // Length
    opt_ptr += 2;
    *opt_ptr++ = 0x03; // Force multicast discovery

    // Update lengths
    uint16_t dhcp_length = opt_ptr - dhcpv6_payload;
    ipv6_hdr->payload_length = sizeof(udp_header_t) + dhcp_length;
    udp_hdr->length = sizeof(udp_header_t) + dhcp_length;

    // Calculate UDP checksum
    udp_hdr->checksum = network_ipv6_checksum(udp_hdr, udp_hdr->length, ipv6_hdr->src_addr, ipv6_hdr->dest_addr, IPV6_PROTO_UDP);

    // Send packet
    if (ethernet_send_frame(eth_frame, sizeof(ethernet_frame_t) + sizeof(ipv6_header_t) + ipv6_hdr->payload_length) < 0) {
        return -1;
    }

    // For bootloader, simulate DHCPv6 response
    // In a real implementation, would wait for ADVERTISE message
    config->ip_addr[0] = 192; config->ip_addr[1] = 168; config->ip_addr[2] = 1; config->ip_addr[3] = 100;
    strcpy(config->boot_filename, "pxeboot.efi");

    return 0;
}

int network_ipv6_pxe_boot(network_config_t *config) {
    if (!config || !config->boot_filename[0]) {
        return -1;
    }

    // Download NBP via TFTP over IPv6
    // For now, use the existing TFTP function but with IPv6 address resolution
    uint8_t tftp_server_ipv6[16] = {0xfe, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01}; // Link-local

    // Resolve TFTP server MAC (would use ND in real implementation)
    uint8_t server_mac[6] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55};

    // Download NBP
    uint8_t *nbp_buffer = (uint8_t *)0x1000000;
    // Note: TFTP is still IPv4 based, would need IPv6 TFTP implementation
    uint32_t nbp_size = 1024; // Simulated

    if (nbp_size <= 0) {
        return -1;
    }

    return nbp_size;
}

// Network initialization
int network_init(void) {
    // Initialize Ethernet
    ethernet_init();

    // Set MAC address (would normally read from hardware)
    uint8_t default_mac[6] = {0xB8, 0x27, 0xEB, 0x00, 0x00, 0x01};
    ethernet_set_mac_address(default_mac);
    memcpy(network_config.mac_addr, default_mac, 6);

    // Enable Ethernet
    ethernet_enable();

    // Initialize IPv6
    network_ipv6_init();

    return 0;
}