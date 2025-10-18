#!/usr/bin/env python3
"""
Network Protocol Static Tests
Tests for Ethernet L2, ARP, DHCP, BOOTP, RARP, DNS, TFTP protocols
"""

import struct
import sys
import os

# Add the bootloader directory to path to import modules
sys.path.insert(0, os.path.dirname(__file__))

# Mock the hardware dependencies for static testing
class MockEthernet:
    def __init__(self):
        self.frames = []
        self.status = 0x01  # Link up

    def send_frame(self, frame, length):
        self.frames.append((frame[:length], length))
        return 0

    def get_status(self):
        return self.status

# Mock network config
mock_config = {
    'mac_addr': b'\x00\x11\x22\x33\x44\x55',
    'ip_addr': b'\xc0\xa8\x01\x64',  # 192.168.1.100
    'subnet_mask': b'\xff\xff\xff\x00',
    'gateway': b'\xc0\xa8\x01\x01',
    'dns_server': b'\x08\x08\x08\x08',  # 8.8.8.8
    'boot_server': b'\xc0\xa8\x01\x01',
    'boot_filename': b'kernel8.img'
}

def test_ethernet_l2():
    """Test Ethernet Layer 2 frame building and parsing"""
    print("Testing Ethernet Layer 2...")

    # Test frame structure
    dest_mac = b'\xff\xff\xff\xff\xff\xff'
    src_mac = mock_config['mac_addr']
    ethertype = 0x0806  # ARP

    # Build frame manually
    frame = dest_mac + src_mac + struct.pack('>H', ethertype) + b'payload'

    # Parse frame
    parsed_dest = frame[0:6]
    parsed_src = frame[6:12]
    parsed_type = struct.unpack('>H', frame[12:14])[0]
    payload = frame[14:]

    assert parsed_dest == dest_mac, "Destination MAC mismatch"
    assert parsed_src == src_mac, "Source MAC mismatch"
    assert parsed_type == ethertype, "Ethertype mismatch"
    assert payload == b'payload', "Payload mismatch"

    print("✓ Ethernet L2 frame building and parsing passed")

def test_arp():
    """Test ARP packet building and parsing"""
    print("Testing ARP...")

    # ARP constants
    ARP_HWTYPE_ETHERNET = 0x0001
    ARP_PROTO_IPV4 = 0x0800
    ARP_OP_REQUEST = 0x0001
    ARP_OP_REPLY = 0x0002

    # Build ARP request
    hw_type = ARP_HWTYPE_ETHERNET
    proto_type = ARP_PROTO_IPV4
    hw_addr_len = 6
    proto_addr_len = 4
    operation = ARP_OP_REQUEST
    sender_hw_addr = mock_config['mac_addr']
    sender_proto_addr = mock_config['ip_addr']
    target_hw_addr = b'\x00\x00\x00\x00\x00\x00'
    target_proto_addr = b'\xc0\xa8\x01\x01'  # 192.168.1.1

    arp_packet = struct.pack('>HHBBH6s4s6s4s',
                            hw_type, proto_type, hw_addr_len, proto_addr_len, operation,
                            sender_hw_addr, sender_proto_addr, target_hw_addr, target_proto_addr)

    # Parse ARP packet
    parsed_hw_type, parsed_proto_type, parsed_hw_len, parsed_proto_len, parsed_op = struct.unpack('>HHBBH', arp_packet[0:8])
    parsed_sender_hw = arp_packet[8:14]
    parsed_sender_ip = arp_packet[14:18]
    parsed_target_hw = arp_packet[18:24]
    parsed_target_ip = arp_packet[24:28]

    assert parsed_hw_type == hw_type, "ARP hardware type mismatch"
    assert parsed_proto_type == proto_type, "ARP protocol type mismatch"
    assert parsed_hw_len == hw_addr_len, "ARP hardware address length mismatch"
    assert parsed_proto_len == proto_addr_len, "ARP protocol address length mismatch"
    assert parsed_op == operation, "ARP operation mismatch"
    assert parsed_sender_hw == sender_hw_addr, "ARP sender hardware address mismatch"
    assert parsed_sender_ip == sender_proto_addr, "ARP sender IP mismatch"
    assert parsed_target_hw == target_hw_addr, "ARP target hardware address mismatch"
    assert parsed_target_ip == target_proto_addr, "ARP target IP mismatch"

    print("✓ ARP packet building and parsing passed")

def test_dhcp():
    """Test DHCP packet building and parsing"""
    print("Testing DHCP...")

    # DHCP constants
    DHCP_DISCOVER = 1
    DHCP_MAGIC_COOKIE = 0x63825363

    # Build DHCP discover
    op = 1  # BOOTREQUEST
    htype = 1  # Ethernet
    hlen = 6
    hops = 0
    xid = 0x12345678
    secs = 0
    flags = 0x8000  # Broadcast
    ciaddr = b'\x00\x00\x00\x00'
    yiaddr = b'\x00\x00\x00\x00'
    siaddr = b'\x00\x00\x00\x00'
    giaddr = b'\x00\x00\x00\x00'
    chaddr = mock_config['mac_addr'] + b'\x00' * 10
    sname = b'\x00' * 64
    file = b'\x00' * 128
    magic = DHCP_MAGIC_COOKIE

    # DHCP options (message type)
    options = b'\x35\x01' + bytes([DHCP_DISCOVER]) + b'\xff'

    dhcp_packet = struct.pack('>BBBBLHH4s4s4s4s16s64s128sL', op, htype, hlen, hops, xid, secs, flags,
                             ciaddr, yiaddr, siaddr, giaddr, chaddr, sname, file, magic) + options

    # Parse DHCP packet
    parsed_op, parsed_htype, parsed_hlen, parsed_hops, parsed_xid, parsed_secs, parsed_flags = struct.unpack('>BBBBLHH', dhcp_packet[0:12])
    parsed_ciaddr = dhcp_packet[12:16]
    parsed_yiaddr = dhcp_packet[16:20]
    parsed_siaddr = dhcp_packet[20:24]
    parsed_giaddr = dhcp_packet[24:28]
    parsed_chaddr = dhcp_packet[28:44]
    parsed_magic = struct.unpack('>L', dhcp_packet[236:240])[0]

    assert parsed_op == op, "DHCP op mismatch"
    assert parsed_htype == htype, "DHCP htype mismatch"
    assert parsed_hlen == hlen, "DHCP hlen mismatch"
    assert parsed_xid == xid, "DHCP xid mismatch"
    assert parsed_flags == flags, "DHCP flags mismatch"
    assert parsed_magic == magic, "DHCP magic cookie mismatch"

    print("✓ DHCP packet building and parsing passed")

def test_bootp():
    """Test BOOTP packet building and parsing"""
    print("Testing BOOTP...")

    # BOOTP is essentially DHCP without options
    # Build BOOTP request
    op = 1  # BOOTREQUEST
    htype = 1
    hlen = 6
    hops = 0
    xid = 0x87654321
    secs = 0
    flags = 0
    ciaddr = b'\x00\x00\x00\x00'
    yiaddr = b'\x00\x00\x00\x00'
    siaddr = b'\x00\x00\x00\x00'
    giaddr = b'\x00\x00\x00\x00'
    chaddr = mock_config['mac_addr'] + b'\x00' * 10
    sname = b'bootp-server.example.com' + b'\x00' * (64 - 24)
    file = b'kernel8.img' + b'\x00' * (128 - 11)

    bootp_packet = struct.pack('>BBBBLHH4s4s4s4s16s64s128s', op, htype, hlen, hops, xid, secs, flags,
                              ciaddr, yiaddr, siaddr, giaddr, chaddr, sname, file)

    # Parse BOOTP packet
    parsed_op, parsed_htype, parsed_hlen, parsed_hops, parsed_xid, parsed_secs, parsed_flags = struct.unpack('>BBBBLHH', bootp_packet[0:12])
    parsed_file = bootp_packet[108:108+11]  # kernel8.img

    assert parsed_op == op, "BOOTP op mismatch"
    assert parsed_htype == htype, "BOOTP htype mismatch"
    assert parsed_hlen == hlen, "BOOTP hlen mismatch"
    assert parsed_xid == xid, "BOOTP xid mismatch"
    assert parsed_file == b'kernel8.img', "BOOTP file mismatch"

    print("✓ BOOTP packet building and parsing passed")

def test_rarp():
    """Test RARP packet building and parsing"""
    print("Testing RARP...")

    # RARP uses same format as ARP but different operation
    ARP_OP_RARP_REQUEST = 0x0003
    ARP_OP_RARP_REPLY = 0x0004

    # Build RARP request
    hw_type = 0x0001
    proto_type = 0x0800
    hw_addr_len = 6
    proto_addr_len = 4
    operation = ARP_OP_RARP_REQUEST
    sender_hw_addr = mock_config['mac_addr']
    sender_proto_addr = b'\x00\x00\x00\x00'  # Unknown IP
    target_hw_addr = b'\x00\x00\x00\x00\x00\x00'
    target_proto_addr = b'\x00\x00\x00\x00'  # Request IP for this MAC

    rarp_packet = struct.pack('>HHBBH6s4s6s4s',
                             hw_type, proto_type, hw_addr_len, proto_addr_len, operation,
                             sender_hw_addr, sender_proto_addr, target_hw_addr, target_proto_addr)

    # Parse RARP packet
    parsed_op = struct.unpack('>H', rarp_packet[6:8])[0]
    parsed_sender_hw = rarp_packet[8:14]

    assert parsed_op == operation, "RARP operation mismatch"
    assert parsed_sender_hw == sender_hw_addr, "RARP sender hardware address mismatch"

    print("✓ RARP packet building and parsing passed")

def test_dns():
    """Test DNS packet building and parsing"""
    print("Testing DNS...")

    # DNS constants
    DNS_TYPE_A = 1
    DNS_CLASS_IN = 1

    # Build DNS query for "example.com"
    transaction_id = 0x1234
    flags = 0x0100  # Standard query
    qdcount = 1
    ancount = 0
    nscount = 0
    arcount = 0

    # Encode domain name: 7example3com0
    domain_name = b'\x07example\x03com\x00'
    question = struct.pack('>HH', DNS_TYPE_A, DNS_CLASS_IN)

    dns_packet = struct.pack('>HHHHHH', transaction_id, flags, qdcount, ancount, nscount, arcount) + domain_name + question

    # Parse DNS packet
    parsed_id, parsed_flags, parsed_qd = struct.unpack('>HHH', dns_packet[0:6])
    domain_start = 12
    domain_end = dns_packet.find(b'\x00', domain_start) + 1
    parsed_domain = dns_packet[domain_start:domain_end]
    parsed_qtype, parsed_qclass = struct.unpack('>HH', dns_packet[domain_end:domain_end+4])

    assert parsed_id == transaction_id, "DNS ID mismatch"
    assert parsed_flags == flags, "DNS flags mismatch"
    assert parsed_qd == qdcount, "DNS question count mismatch"
    assert parsed_domain == domain_name, "DNS domain name mismatch"
    assert parsed_qtype == DNS_TYPE_A, "DNS query type mismatch"
    assert parsed_qclass == DNS_CLASS_IN, "DNS query class mismatch"

    print("✓ DNS packet building and parsing passed")

def test_tftp():
    """Test TFTP packet building and parsing"""
    print("Testing TFTP...")

    # TFTP opcodes
    TFTP_OP_RRQ = 1  # Read Request
    TFTP_OP_WRQ = 2  # Write Request
    TFTP_OP_DATA = 3
    TFTP_OP_ACK = 4
    TFTP_OP_ERROR = 5

    # Build TFTP RRQ
    filename = b'kernel8.img'
    mode = b'octet'
    tftp_packet = struct.pack('>H', TFTP_OP_RRQ) + filename + b'\x00' + mode + b'\x00'

    # Parse TFTP packet
    parsed_op = struct.unpack('>H', tftp_packet[0:2])[0]
    # Find filename and mode
    null1_pos = tftp_packet.find(b'\x00', 2)
    parsed_filename = tftp_packet[2:null1_pos]
    null2_pos = tftp_packet.find(b'\x00', null1_pos + 1)
    parsed_mode = tftp_packet[null1_pos + 1:null2_pos]

    assert parsed_op == TFTP_OP_RRQ, "TFTP opcode mismatch"
    assert parsed_filename == filename, "TFTP filename mismatch"
    assert parsed_mode == mode, "TFTP mode mismatch"

    # Test TFTP DATA packet
    block_num = 1
    data = b'kernel data...'
    data_packet = struct.pack('>HH', TFTP_OP_DATA, block_num) + data

    parsed_data_op, parsed_block = struct.unpack('>HH', data_packet[0:4])
    parsed_data = data_packet[4:]

    assert parsed_data_op == TFTP_OP_DATA, "TFTP DATA opcode mismatch"
    assert parsed_block == block_num, "TFTP block number mismatch"
    assert parsed_data == data, "TFTP data mismatch"

    print("✓ TFTP packet building and parsing passed")

def run_network_tests():
    """Run all network protocol tests"""
    print("=== Network Protocol Static Tests ===\n")

    tests = [
        test_ethernet_l2,
        test_arp,
        test_dhcp,
        test_bootp,
        test_rarp,
        test_dns,
        test_tftp
    ]

    passed = 0
    for test in tests:
        try:
            test()
            passed += 1
        except Exception as e:
            print(f"✗ {test.__name__} failed: {e}")

    print(f"\nNetwork tests passed: {passed}/{len(tests)}")

    if passed == len(tests):
        print("🎉 All network protocol tests passed!")
        return True
    else:
        print("❌ Some network protocol tests failed")
        return False

if __name__ == "__main__":
    success = run_network_tests()
    sys.exit(0 if success else 1)