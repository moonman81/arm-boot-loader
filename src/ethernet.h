/* Ethernet Controller Header for Raspberry Pi */

#ifndef ETHERNET_H
#define ETHERNET_H

#include <stdint.h>

// Ethernet GENET Controller registers (Pi 4)
#define ETH_BASE 0xFE300000

#define ETH_SYS_REV_CTRL     (ETH_BASE + 0x000)  // System Revision Control
#define ETH_SYS_PORT_CTRL    (ETH_BASE + 0x004)  // System Port Control
#define ETH_RBUF_FLUSH_CTRL  (ETH_BASE + 0x0B4)  // RBUF Flush Control
#define ETH_TBUF_FLUSH_CTRL  (ETH_BASE + 0x0B8)  // TBUF Flush Control

// UMAC registers
#define ETH_UMAC_CMD         (ETH_BASE + 0x808)  // UMAC Command
#define ETH_UMAC_MAC0        (ETH_BASE + 0x80C)  // MAC Address 0
#define ETH_UMAC_MAC1        (ETH_BASE + 0x810)  // MAC Address 1
#define ETH_UMAC_MAX_FRAME_LEN (ETH_BASE + 0x814) // Maximum Frame Length

// RBUF registers
#define ETH_RBUF_CTRL        (ETH_BASE + 0xC00)  // RBUF Control
#define ETH_RBUF_STATUS      (ETH_BASE + 0xC04)  // RBUF Status

// TBUF registers
#define ETH_TBUF_CTRL        (ETH_BASE + 0xD00)  // TBUF Control
#define ETH_TBUF_DATA        (ETH_BASE + 0xD04)  // TBUF Data

// HFB registers
#define ETH_HFB_CTRL         (ETH_BASE + 0xE00)  // HFB Control
#define ETH_HFB_FLT_ENABLE1  (ETH_BASE + 0xE04)  // HFB Filter Enable 1

// DMA registers
#define ETH_DMA_TX_RING0_ADDR    (ETH_BASE + 0x1000)  // TX Ring 0 Address
#define ETH_DMA_TX_RING0_SIZE    (ETH_BASE + 0x1004)  // TX Ring 0 Size
#define ETH_DMA_TX_RING0_CTRL    (ETH_BASE + 0x1008)  // TX Ring 0 Control
#define ETH_DMA_TX_RING0_STATUS  (ETH_BASE + 0x100C)  // TX Ring 0 Status

#define ETH_DMA_RX_RING0_ADDR    (ETH_BASE + 0x1100)  // RX Ring 0 Address
#define ETH_DMA_RX_RING0_SIZE    (ETH_BASE + 0x1104)  // RX Ring 0 Size
#define ETH_DMA_RX_RING0_CTRL    (ETH_BASE + 0x1108)  // RX Ring 0 Control
#define ETH_DMA_RX_RING0_STATUS  (ETH_BASE + 0x110C)  // RX Ring 0 Status

#define ETH_DMA_CTRL             (ETH_BASE + 0x1200)  // DMA Control
#define ETH_DMA_STATUS           (ETH_BASE + 0x1204)  // DMA Status
#define ETH_DMA_INTR_EN          (ETH_BASE + 0x1208)  // DMA Interrupt Enable

// Ethernet frame structure
typedef struct {
    uint8_t dest_mac[6];
    uint8_t src_mac[6];
    uint16_t ethertype;
    uint8_t payload[1500];
} ethernet_frame_t;

// Ethernet commands
#define ETH_CMD_TX_EN        (1 << 0)   // Transmit Enable
#define ETH_CMD_RX_EN        (1 << 1)   // Receive Enable
#define ETH_CMD_SPEED_100    (1 << 2)   // 100Mbps
#define ETH_CMD_SPEED_1000   (1 << 3)   // 1000Mbps
#define ETH_CMD_PROMISC      (1 << 4)   // Promiscuous Mode
#define ETH_CMD_PAD_EN       (1 << 5)   // Padding Enable
#define ETH_CMD_CRC_EN       (1 << 6)   // CRC Enable
#define ETH_CMD_HD_EN        (1 << 7)   // Half Duplex Enable

// Ethernet status
#define ETH_STATUS_LINK_UP   (1 << 0)
#define ETH_STATUS_FULL_DUPLEX (1 << 1)
#define ETH_STATUS_100MBPS   (1 << 2)
#define ETH_STATUS_1000MBPS  (1 << 3)

// DMA control
#define DMA_TX_EN            (1 << 0)  // TX DMA Enable
#define DMA_RX_EN            (1 << 1)  // RX DMA Enable
#define DMA_TX_RING_EN       (1 << 2)  // TX Ring Enable
#define DMA_RX_RING_EN       (1 << 3)  // RX Ring Enable

// DMA status
#define DMA_TX_RING_FULL     (1 << 0)
#define DMA_TX_RING_EMPTY    (1 << 1)
#define DMA_RX_RING_FULL     (1 << 2)
#define DMA_RX_RING_EMPTY    (1 << 3)

// DMA ring control
#define RING_CTRL_EN         (1 << 0)  // Ring Enable
#define RING_CTRL_INTR_EN    (1 << 1)  // Interrupt Enable

// Frame buffer descriptor
typedef struct {
    uint32_t addr;      // Buffer address
    uint32_t length;    // Buffer length and flags
} dma_desc_t;

void ethernet_init(void);
void ethernet_set_mac_address(const uint8_t *mac);
int ethernet_send_frame(const ethernet_frame_t *frame, uint16_t length);
int ethernet_receive_frame(ethernet_frame_t *frame, uint16_t *length);
uint32_t ethernet_get_status(void);
int ethernet_enable(void);
int ethernet_disable(void);

// Network protocol definitions
#define ETHERTYPE_IP    0x0800
#define ETHERTYPE_ARP   0x0806
#define ETHERTYPE_IPV6  0x86DD

// ARP definitions
#define ARP_HWTYPE_ETHERNET  0x0001
#define ARP_PROTO_IPV4       0x0800
#define ARP_OP_REQUEST       0x0001
#define ARP_OP_REPLY         0x0002

// ARP packet structure
typedef struct {
    uint16_t hw_type;
    uint16_t proto_type;
    uint8_t hw_addr_len;
    uint8_t proto_addr_len;
    uint16_t operation;
    uint8_t sender_hw_addr[6];
    uint32_t sender_proto_addr;
    uint8_t target_hw_addr[6];
    uint32_t target_proto_addr;
} __attribute__((packed)) arp_packet_t;

// IP protocol numbers
#define IP_PROTO_ICMP   1
#define IP_PROTO_UDP    17

// IPv6 protocol numbers
#define IPV6_PROTO_ICMPV6  58
#define IPV6_PROTO_UDP     17

// IPv6 ICMPv6 types
#define ICMPV6_TYPE_NEIGHBOR_SOLICITATION 135
#define ICMPV6_TYPE_NEIGHBOR_ADVERTISEMENT 136
#define ICMPV6_TYPE_ROUTER_SOLICITATION   133
#define ICMPV6_TYPE_ROUTER_ADVERTISEMENT  134

// IPv6 multicast addresses
#define IPV6_ALL_NODES_MULTICAST    {0xff, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01}
#define IPV6_ALL_ROUTERS_MULTICAST  {0xff, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02}

// IPv6 neighbor discovery structures
typedef struct {
    uint8_t type;
    uint8_t code;
    uint16_t checksum;
    uint32_t reserved;
    uint8_t target_addr[16];
    // Options follow
} icmpv6_neighbor_solicitation_t;

typedef struct {
    uint8_t type;
    uint8_t code;
    uint16_t checksum;
    uint8_t flags;
    uint8_t reserved[3];
    uint8_t target_addr[16];
    // Options follow
} icmpv6_neighbor_advertisement_t;

typedef struct {
    uint8_t type;
    uint8_t code;
    uint16_t checksum;
    uint32_t reserved;
    // Options follow
} __attribute__((packed)) icmpv6_router_solicitation_t;

typedef struct {
    uint8_t type;
    uint8_t code;
    uint16_t checksum;
    uint8_t hop_limit;
    uint8_t flags;
    uint16_t router_lifetime;
    uint32_t reachable_time;
    uint32_t retrans_timer;
    // Options follow
} __attribute__((packed)) icmpv6_router_advertisement_t;

// ND option types
#define ND_OPT_SOURCE_LINK_LAYER_ADDR 1
#define ND_OPT_TARGET_LINK_LAYER_ADDR 2
#define ND_OPT_PREFIX_INFO           3
#define ND_OPT_MTU                   5

typedef struct {
    uint8_t type;
    uint8_t length; // in units of 8 octets
    uint8_t addr[6];
} __attribute__((packed)) nd_option_link_layer_addr_t;

// UDP ports
#define UDP_PORT_DHCP_CLIENT  68
#define UDP_PORT_DHCP_SERVER  67
#define UDP_PORT_BOOTP_CLIENT 68
#define UDP_PORT_DNS          53
#define UDP_PORT_DHCPV6_CLIENT 546
#define UDP_PORT_DHCPV6_SERVER 547

// DNS definitions
#define DNS_TYPE_A     1
#define DNS_TYPE_AAAA  28
#define DNS_CLASS_IN   1

// DNS header structure
typedef struct dns_header {
    uint16_t id;
    uint16_t flags;
    uint16_t qdcount;
    uint16_t ancount;
    uint16_t nscount;
    uint16_t arcount;
} __attribute__((packed)) dns_header_t;

// DNS question structure
typedef struct dns_question {
    uint16_t qtype;
    uint16_t qclass;
} __attribute__((packed)) dns_question_t;

// DNS resource record structure
typedef struct dns_rr {
    uint16_t type;
    uint16_t class;
    uint32_t ttl;
    uint16_t rdlength;
} dns_rr_t;

// TCP ports
#define TCP_PORT_HTTP  80
#define TCP_PORT_HTTPS 443

// HTTP methods
#define HTTP_METHOD_GET  "GET"
#define HTTP_METHOD_POST "POST"

// HTTP status codes
#define HTTP_STATUS_OK       200
#define HTTP_STATUS_NOT_FOUND 404

// Basic HTTP request structure
typedef struct http_request {
    char method[8];
    char uri[256];
    char host[128];
    char user_agent[128];
} http_request_t;

// Basic HTTP response structure
typedef struct http_response {
    int status_code;
    char content_type[64];
    uint32_t content_length;
    char *body;
} http_response_t;
#define UDP_PORT_BOOTP_SERVER 67
#define UDP_PORT_TFTP         69

// Network configuration
typedef struct {
    uint8_t mac_addr[6];
    uint8_t ip_addr[4];
    uint8_t subnet_mask[4];
    uint8_t gateway[4];
    uint8_t dns_server[4];
    uint8_t boot_server[4];
    char boot_filename[128];
} network_config_t;

// IP header structure
typedef struct {
    uint8_t version_ihl;
    uint8_t tos;
    uint16_t total_length;
    uint16_t identification;
    uint16_t flags_fragment;
    uint8_t ttl;
    uint8_t protocol;
    uint16_t checksum;
    uint32_t src_ip;
    uint32_t dest_ip;
} __attribute__((packed)) ip_header_t;

// UDP header structure
typedef struct {
    uint16_t src_port;
    uint16_t dest_port;
    uint16_t length;
    uint16_t checksum;
} __attribute__((packed)) udp_header_t;

// IPv6 header structure
typedef struct {
    uint32_t version_tc_fl;  // Version (4 bits), Traffic Class (8), Flow Label (20)
    uint16_t payload_length;
    uint8_t next_header;
    uint8_t hop_limit;
    uint8_t src_addr[16];
    uint8_t dest_addr[16];
} __attribute__((packed)) ipv6_header_t;

// DHCP message structure
typedef struct {
    uint8_t op;
    uint8_t htype;
    uint8_t hlen;
    uint8_t hops;
    uint32_t xid;
    uint16_t secs;
    uint16_t flags;
    uint32_t ciaddr;
    uint32_t yiaddr;
    uint32_t siaddr;
    uint32_t giaddr;
    uint8_t chaddr[16];
    uint8_t sname[64];
    uint8_t file[128];
    uint32_t magic_cookie;
    uint8_t options[312];
} __attribute__((packed)) dhcp_message_t;

// DHCP options
#define DHCP_OPTION_SUBNET_MASK     1
#define DHCP_OPTION_ROUTER          3
#define DHCP_OPTION_DNS_SERVER      6
#define DHCP_OPTION_HOST_NAME       12
#define DHCP_OPTION_REQUESTED_IP    50
#define DHCP_OPTION_LEASE_TIME      51
#define DHCP_OPTION_MESSAGE_TYPE    53
#define DHCP_OPTION_SERVER_ID       54
#define DHCP_OPTION_PARAMETER_LIST  55
#define DHCP_OPTION_TFTP_SERVER     66
#define DHCP_OPTION_BOOTFILE        67
#define DHCP_OPTION_PXE_MENU        43
#define DHCP_OPTION_PXE_DISCOVERY   60
#define DHCP_OPTION_END             255

// PXE specific
#define PXE_CLIENT_ARCH_X86         0x0000
#define PXE_CLIENT_ARCH_EFI32       0x0006
#define PXE_CLIENT_ARCH_EFI64       0x0007
#define PXE_CLIENT_ARCH_EFI_ARM     0x000A
#define PXE_CLIENT_ARCH_EFI_ARM64   0x000B

// PXE DHCP vendor options
#define PXE_OPTION_DISCOVERY_CONTROL 6
#define PXE_OPTION_BOOT_SERVERS     8
#define PXE_OPTION_BOOT_MENU        9
#define PXE_OPTION_BOOT_ITEM        10

// DHCP message types
#define DHCP_DISCOVER   1
#define DHCP_OFFER      2
#define DHCP_REQUEST    3
#define DHCP_DECLINE    4
#define DHCP_ACK        5
#define DHCP_NAK        6
#define DHCP_RELEASE    7

// DHCPv6 message types (RFC 3315)
#define DHCPV6_SOLICIT          1
#define DHCPV6_ADVERTISE        2
#define DHCPV6_REQUEST          3
#define DHCPV6_CONFIRM          4
#define DHCPV6_RENEW            5
#define DHCPV6_REBIND           6
#define DHCPV6_REPLY            7
#define DHCPV6_RELEASE          8
#define DHCPV6_DECLINE          9
#define DHCPV6_RECONFIGURE      10
#define DHCPV6_INFORMATION_REQUEST 11
#define DHCPV6_RELAY_FORW       12
#define DHCPV6_RELAY_REPL       13

// DHCPv6 option codes
#define DHCPV6_OPT_CLIENTID         1
#define DHCPV6_OPT_SERVERID         2
#define DHCPV6_OPT_IA_NA            3
#define DHCPV6_OPT_IA_TA            4
#define DHCPV6_OPT_IAADDR           5
#define DHCPV6_OPT_ORO              6
#define DHCPV6_OPT_PREFERENCE       7
#define DHCPV6_OPT_ELAPSED_TIME     8
#define DHCPV6_OPT_RELAY_MSG        9
#define DHCPV6_OPT_AUTH             11
#define DHCPV6_OPT_UNICAST          12
#define DHCPV6_OPT_STATUS_CODE      13
#define DHCPV6_OPT_RAPID_COMMIT     14
#define DHCPV6_OPT_USER_CLASS       15
#define DHCPV6_OPT_VENDOR_CLASS     16
#define DHCPV6_OPT_VENDOR_OPTS      17
#define DHCPV6_OPT_INTERFACE_ID     18
#define DHCPV6_OPT_RECONF_MSG       19
#define DHCPV6_OPT_RECONF_ACCEPT    20
#define DHCPV6_OPT_BOOTFILE_URL     59  // PXE specific
#define DHCPV6_OPT_BOOTFILE_PARAM   60  // PXE specific

// PXE DHCPv6 vendor options
#define PXE_VENDOR_OPT_DISCOVERY_CONTROL 6
#define PXE_VENDOR_OPT_BOOT_SERVERS     8
#define PXE_VENDOR_OPT_BOOT_MENU        9
#define PXE_VENDOR_OPT_BOOT_ITEM        10

// DHCPv6 message structure
typedef struct {
    uint8_t msg_type;
    uint8_t transaction_id[3];
    uint8_t options[];  // Variable length options
} __attribute__((packed)) dhcpv6_message_t;

// DHCPv6 option header
typedef struct {
    uint16_t option_code;
    uint16_t option_length;
    uint8_t data[];
} __attribute__((packed)) dhcpv6_option_t;

// Network functions
int network_init(void);
int network_dhcp_discover(network_config_t *config);
int network_bootp_request(network_config_t *config);
int network_tftp_download(const char *filename, uint32_t server_ip, void *buffer, uint32_t max_size);
int network_process_packet(const ethernet_frame_t *frame, uint16_t length);

// DNS functions
int network_dns_resolve(const char *hostname, uint32_t *ip_addr);
int network_dns_resolve_ipv6(const char *hostname, uint8_t *ipv6_addr);

// HTTP functions
int network_http_get(const char *url, uint8_t *buffer, uint32_t max_size, uint32_t *received);

// PXE functions
int network_pxe_discover(network_config_t *config);
int network_pxe_boot(network_config_t *config);

// IPv6 functions
int network_ipv6_init(void);
int network_ipv6_send_udp(const uint8_t *dest_addr, uint16_t src_port, uint16_t dest_port,
                         const void *data, uint16_t data_length);
uint16_t network_ipv6_checksum(const void *data, uint32_t length, const uint8_t *src_addr, const uint8_t *dest_addr, uint8_t next_header);

// IPv6 PXE functions
int network_ipv6_pxe_discover(network_config_t *config);
int network_ipv6_pxe_boot(network_config_t *config);

// ARP functions
int network_arp_request(uint32_t target_ip, uint8_t *target_mac);
int network_arp_reply(uint32_t target_ip, const uint8_t *target_mac);
int network_process_arp_packet(const arp_packet_t *arp_packet);

// Utility functions
uint16_t network_checksum(const void *data, uint32_t length);
uint32_t network_ip_to_int(const uint8_t *ip);
void network_int_to_ip(uint32_t ip_int, uint8_t *ip);
int network_parse_ip(const char *ip_str, uint32_t *ip);

#endif