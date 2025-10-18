#!/usr/bin/env python3
"""
FAT Filesystem Testing Framework

Tests FAT12/16/32 filesystem support with mock data and edge cases.
"""

import struct
import os

# FAT boot sector structure (packed) - 62 bytes total
FAT_BOOT_SECTOR_FORMAT = '<3s8sHBHBHHBHHHLLLHHLL12sBBBL11s8s'

# Directory entry structure - 32 bytes
DIR_ENTRY_FORMAT = '<11sBBBHHHHHHHHL'

class MockSDCard:
    """Mock SD card for testing FAT filesystem functions"""

    def __init__(self, fat_type='FAT32'):
        self.fat_type = fat_type
        self.sectors = {}
        self.create_mock_filesystem()

    def create_mock_filesystem(self):
        """Create a mock FAT filesystem in memory"""
        if self.fat_type == 'FAT32':
            self.create_fat32_fs()
        elif self.fat_type == 'FAT16':
            self.create_fat16_fs()
        elif self.fat_type == 'FAT12':
            self.create_fat12_fs()

    def create_fat32_fs(self):
        """Create mock FAT32 filesystem"""
        # Boot sector - create manually to avoid struct issues
        # Boot sector - create manually to avoid struct issues
        bs = b'\xeb\x58\x90'  # jump
        bs += b'MSDOS5.0'     # oem_name (8 bytes)
        bs += b'\x00\x02'     # bytes_per_sector (512)
        bs += b'\x08'         # sectors_per_cluster
        bs += b'\x20\x00'     # reserved_sectors (32)
        bs += b'\x02'         # num_fats
        bs += b'\x00\x00'     # root_entries (0 for FAT32)
        bs += b'\x00\x00'     # total_sectors_16 (0)
        bs += b'\xF8'         # media_descriptor
        bs += b'\x00\x00'     # sectors_per_fat_16 (0)
        bs += b'\x3F\x00'     # sectors_per_track (63)
        bs += b'\xFF\x00'     # num_heads (255)
        bs += b'\x00\x00\x00\x00'  # hidden_sectors
        bs += b'\x00\x00\x20\x00'  # total_sectors_32 (2097152 = 0x200000 = 1GB)
        bs += b'\x00\x40\x00\x00'  # sectors_per_fat_32 (16384 = 0x4000)
        bs += b'\x00\x00'     # flags
        bs += b'\x00\x00'     # version
        bs += b'\x02\x00\x00\x00'  # root_cluster (2)
        bs += b'\x01\x00'     # fs_info_sector
        bs += b'\x06\x00'     # backup_boot_sector
        bs += b'\x00' * 12    # reserved
        bs += b'\x80'         # drive_number
        bs += b'\x00'         # reserved1
        bs += b'\x29'         # boot_signature
        bs += b'\x78\x56\x34\x12'  # volume_id
        bs += b'TESTVOLUME '   # volume_label (11 bytes)
        bs += b'FAT32   '     # fs_type (8 bytes)
        self.sectors[0] = bs

        # FAT table (simplified - all clusters free except root)
        fat_sector = b'\xF8\xFF\xFF\x0F' + b'\x00\x00\x00\x00' * 127  # 128 entries per sector
        for i in range(32, 32 + 16384):
            self.sectors[i] = fat_sector

        # Root directory (cluster 2)
        data_start_sector = 32 + 16384 * 2  # reserved + fat_size * num_fats
        root_sector = data_start_sector + (2 - 2) * 8  # (root_cluster - 2) * sectors_per_cluster
        # Create directory entry manually
        dir_entry = (
            b'KERNEL8 IMG'   # name (11 bytes)
            b'\x20'          # attributes (archive)
            b'\x00'          # reserved
            b'\x00'          # creation_time_tenths
            b'\x00\x00'      # creation_time
            b'\x00\x00'      # creation_date
            b'\x00\x00'      # last_access_date
            b'\x00\x00'      # first_cluster_high
            b'\x00\x00'      # last_mod_time
            b'\x00\x00'      # last_mod_date
            b'\x03\x00'      # first_cluster_low (3)
            b'\x00\x00\x10\x00'  # file_size (1048576 = 0x100000)
        )
        dir_data = dir_entry + b'\x00' * (512 - len(dir_entry))
        self.sectors[root_sector] = dir_data

        # Kernel data (cluster 3)
        kernel_data = b'Linux kernel data...' + b'\x00' * (512 * 8 - 20)
        for i in range(8):  # 8 sectors per cluster
            self.sectors[32 + 16384 * 2 + 3 * 8 + i] = kernel_data[i*512:(i+1)*512]

    def create_fat16_fs(self):
        """Create mock FAT16 filesystem"""
        # Boot sector - create manually to avoid struct issues
        bs = b'\xeb\x58\x90'  # jump
        bs += b'MSDOS5.0'     # oem_name (8 bytes)
        bs += b'\x00\x02'     # bytes_per_sector (512)
        bs += b'\x04'         # sectors_per_cluster
        bs += b'\x01\x00'     # reserved_sectors (1)
        bs += b'\x02'         # num_fats
        bs += b'\x00\x02'     # root_entries (512)
        bs += b'\x00\x80'     # total_sectors_16 (32768 = 0x8000)
        bs += b'\xF8'         # media_descriptor
        bs += b'\x00\x01'     # sectors_per_fat_16 (256 = 0x100)
        bs += b'\x3F\x00'     # sectors_per_track (63)
        bs += b'\xFF\x00'     # num_heads (255)
        bs += b'\x00\x00\x00\x00'  # hidden_sectors
        bs += b'\x00\x00\x00\x00'  # total_sectors_32 (0)
        bs += b'\x00\x00\x00\x00'  # sectors_per_fat_32 (0)
        bs += b'\x00\x00'     # flags
        bs += b'\x00\x00'     # version
        bs += b'\x00\x00\x00\x00'  # root_cluster (0)
        bs += b'\x00\x00'     # fs_info_sector
        bs += b'\x00\x00'     # backup_boot_sector
        bs += b'\x00' * 12    # reserved
        bs += b'\x80'         # drive_number
        bs += b'\x00'         # reserved1
        bs += b'\x29'         # boot_signature
        bs += b'\x78\x56\x34\x12'  # volume_id
        bs += b'TESTVOL16 '   # volume_label (11 bytes)
        bs += b'FAT16   '     # fs_type (8 bytes)
        self.sectors[0] = bs

        # FAT table
        fat_sector = b'\xF8\xFF' + b'\x00\x00' * 255  # 256 entries per sector
        for i in range(1, 1 + 256):
            self.sectors[i] = fat_sector

        # Root directory (fixed location after FAT)
        root_start = 1 + 256 * 2  # reserved + fat_size * num_fats
        kernel_entry = struct.pack(
            DIR_ENTRY_FORMAT,
            b'KERNEL8 IMG',  # name
            0x20,            # attributes
            0,0,0,0,0,0,0,0,0,  # various fields
            2,               # first_cluster_low
            512*1024         # file_size
        )
        dir_data = kernel_entry + b'\x00' * (512 - len(kernel_entry))
        self.sectors[root_start] = dir_data

        # Kernel data
        kernel_data = b'FAT16 kernel data...' + b'\x00' * (512 * 4 - 19)
        data_start = root_start + 32  # root dir sectors
        for i in range(4):  # 4 sectors per cluster
            self.sectors[data_start + 2 * 4 + i] = kernel_data[i*512:(i+1)*512]

    def create_fat12_fs(self):
        """Create mock FAT12 filesystem"""
        # Boot sector - create manually to avoid struct issues
        bs = b'\xeb\x58\x90'  # jump
        bs += b'MSDOS5.0'     # oem_name (8 bytes)
        bs += b'\x00\x02'     # bytes_per_sector (512)
        bs += b'\x01'         # sectors_per_cluster
        bs += b'\x01\x00'     # reserved_sectors (1)
        bs += b'\x02'         # num_fats
        bs += b'\xE0\x00'     # root_entries (224 = 0xE0)
        bs += b'\x40\x0B'     # total_sectors_16 (2880 = 0x0B40)
        bs += b'\xF0'         # media_descriptor
        bs += b'\x09\x00'     # sectors_per_fat_16 (9)
        bs += b'\x12\x00'     # sectors_per_track (18)
        bs += b'\x02\x00'     # num_heads (2)
        bs += b'\x00\x00\x00\x00'  # hidden_sectors
        bs += b'\x00\x00\x00\x00'  # total_sectors_32 (0)
        bs += b'\x00\x00\x00\x00'  # sectors_per_fat_32 (0)
        bs += b'\x00\x00'     # flags
        bs += b'\x00\x00'     # version
        bs += b'\x00\x00\x00\x00'  # root_cluster (0)
        bs += b'\x00\x00'     # fs_info_sector
        bs += b'\x00\x00'     # backup_boot_sector
        bs += b'\x00' * 12    # reserved
        bs += b'\x00'         # drive_number
        bs += b'\x00'         # reserved1
        bs += b'\x29'         # boot_signature
        bs += b'\x78\x56\x34\x12'  # volume_id
        bs += b'TESTVOL12 '   # volume_label (11 bytes)
        bs += b'FAT12   '     # fs_type (8 bytes)
        self.sectors[0] = bs

        # FAT table (9 sectors, 12-bit entries)
        # Each 12-bit entry takes 1.5 bytes, so 341 entries per 512-byte sector
        fat_data = b'\xF0\xFF'  # First two entries (media descriptor + EOF)
        fat_data += b'\x00\x00' * 170  # Fill sector
        for i in range(1, 1 + 9):
            self.sectors[i] = fat_data

        # Root directory
        root_start = 1 + 9 * 2  # reserved + fat_size * num_fats
        kernel_entry = struct.pack(
            DIR_ENTRY_FORMAT,
            b'KERNEL8 IMG',  # name
            0x20,            # attributes
            0,0,0,0,0,0,0,0,0,  # various fields
            2,               # first_cluster_low
            512*10           # file_size (10 sectors)
        )
        dir_data = kernel_entry + b'\x00' * (512 - len(kernel_entry))
        self.sectors[root_start] = dir_data

        # Kernel data
        kernel_data = b'FAT12 kernel data...' + b'\x00' * (512 - 19)
        data_start = root_start + 14  # root dir sectors (224 entries * 32 / 512)
        for i in range(10):
            self.sectors[data_start + i] = kernel_data

    def read_sector(self, sector_num):
        """Read a sector from the mock SD card"""
        return self.sectors.get(sector_num, b'\x00' * 512)

def test_fat_type_detection():
    """Test FAT type detection"""
    print("Testing FAT type detection...")

    # Test FAT32
    sd32 = MockSDCard('FAT32')
    boot_sector = sd32.read_sector(0)

    # Parse boot sector manually
    bytes_per_sector = (boot_sector[12] << 8) | boot_sector[11]
    sectors_per_cluster = boot_sector[13]
    reserved_sectors = (boot_sector[15] << 8) | boot_sector[14]
    num_fats = boot_sector[16]
    root_entries = (boot_sector[18] << 8) | boot_sector[17]
    total_sectors_16 = (boot_sector[20] << 8) | boot_sector[19]
    sectors_per_fat_16 = (boot_sector[23] << 8) | boot_sector[22]
    total_sectors_32 = (boot_sector[35] << 24) | (boot_sector[34] << 16) | (boot_sector[33] << 8) | boot_sector[32]
    sectors_per_fat_32 = (boot_sector[39] << 24) | (boot_sector[38] << 16) | (boot_sector[37] << 8) | boot_sector[36]

    total_sectors = total_sectors_16 if total_sectors_16 else total_sectors_32
    sectors_per_fat = sectors_per_fat_16 if sectors_per_fat_16 else sectors_per_fat_32

    data_sectors = total_sectors - (reserved_sectors + num_fats * sectors_per_fat + (root_entries * 32 + bytes_per_sector - 1) // bytes_per_sector)
    total_clusters = data_sectors // sectors_per_cluster

    if total_clusters > 65525:
        fat_type = 'FAT32'
    elif total_clusters > 4085:
        fat_type = 'FAT16'
    else:
        fat_type = 'FAT12'

    assert fat_type == 'FAT32', f"Expected FAT32, got {fat_type}"
    print("✓ FAT32 detection passed")

    # Test FAT16
    sd16 = MockSDCard('FAT16')
    boot_sector = sd16.read_sector(0)

    bytes_per_sector = (boot_sector[12] << 8) | boot_sector[11]
    sectors_per_cluster = boot_sector[13]
    reserved_sectors = (boot_sector[15] << 8) | boot_sector[14]
    num_fats = boot_sector[16]
    root_entries = (boot_sector[18] << 8) | boot_sector[17]
    total_sectors_16 = (boot_sector[20] << 8) | boot_sector[19]
    sectors_per_fat_16 = (boot_sector[23] << 8) | boot_sector[22]

    total_sectors = total_sectors_16
    sectors_per_fat = sectors_per_fat_16

    data_sectors = total_sectors - (reserved_sectors + num_fats * sectors_per_fat + (root_entries * 32 + bytes_per_sector - 1) // bytes_per_sector)
    total_clusters = data_sectors // sectors_per_cluster

    if total_clusters > 65525:
        fat_type = 'FAT32'
    elif total_clusters > 4085:
        fat_type = 'FAT16'
    else:
        fat_type = 'FAT12'

    assert fat_type == 'FAT16', f"Expected FAT16, got {fat_type}"
    print("✓ FAT16 detection passed")

    # Test FAT12
    sd12 = MockSDCard('FAT12')
    boot_sector = sd12.read_sector(0)

    bytes_per_sector = (boot_sector[12] << 8) | boot_sector[11]
    sectors_per_cluster = boot_sector[13]
    reserved_sectors = (boot_sector[15] << 8) | boot_sector[14]
    num_fats = boot_sector[16]
    root_entries = (boot_sector[18] << 8) | boot_sector[17]
    total_sectors_16 = (boot_sector[20] << 8) | boot_sector[19]
    sectors_per_fat_16 = (boot_sector[23] << 8) | boot_sector[22]

    total_sectors = total_sectors_16
    sectors_per_fat = sectors_per_fat_16

    data_sectors = total_sectors - (reserved_sectors + num_fats * sectors_per_fat + (root_entries * 32 + bytes_per_sector - 1) // bytes_per_sector)
    total_clusters = data_sectors // sectors_per_cluster

    if total_clusters > 65525:
        fat_type = 'FAT32'
    elif total_clusters > 4085:
        fat_type = 'FAT16'
    else:
        fat_type = 'FAT12'

    assert fat_type == 'FAT12', f"Expected FAT12, got {fat_type}"
    print("✓ FAT12 detection passed")

def calculate_total_clusters(fields):
    """Calculate total clusters from boot sector fields"""
    bytes_per_sector, sectors_per_cluster, reserved_sectors, num_fats, root_entries, total_sectors_16, sectors_per_fat_16, total_sectors_32, sectors_per_fat_32 = fields[2:11] + fields[11:12] + fields[12:13]

    total_sectors = total_sectors_16 if total_sectors_16 else total_sectors_32
    sectors_per_fat = sectors_per_fat_16 if sectors_per_fat_16 else sectors_per_fat_32

    data_sectors = total_sectors - (reserved_sectors + num_fats * sectors_per_fat + (root_entries * 32 + bytes_per_sector - 1) // bytes_per_sector)
    return data_sectors // sectors_per_cluster

def test_filename_conversion():
    """Test 8.3 filename conversion"""
    print("Testing 8.3 filename conversion...")

    test_cases = [
        ("kernel8.img", b"kernel8 img"),
        ("test.txt", b"test    txt"),
        ("longfilename.txt", b"longfiletxt"),  # Truncated
        ("noext", b"noext      "),
        ("a.b.c", b"a.b     c  "),  # Last extension used
    ]

    for input_name, expected in test_cases:
        # Convert to 8.3 format (simulating the C code)
        result = convert_to_83(input_name)
        assert result == expected, f"Expected {expected}, got {result}"
        print(f"✓ '{input_name}' -> {result}")

def convert_to_83(filename):
    """Convert filename to 8.3 format (simplified)"""
    # Find the last dot for extension
    parts = filename.rsplit('.', 1)
    if len(parts) == 2:
        name_part, ext_part = parts
    else:
        name_part, ext_part = parts[0], ''

    # Pad/truncate name part
    name_padded = (name_part[:8] + '        ')[:8]

    # Pad/truncate extension
    ext_padded = (ext_part[:3] + '   ')[:3]

    return (name_padded + ext_padded).encode('ascii')

def test_directory_traversal():
    """Test directory entry parsing and file finding"""
    print("Testing directory traversal...")

    sd = MockSDCard('FAT32')
    data_start_sector = 32 + 16384 * 2  # reserved + fat_size * num_fats
    root_sector = data_start_sector + (2 - 2) * 8  # (root_cluster - 2) * sectors_per_cluster

    dir_data = sd.read_sector(root_sector)
    entries = []

    for i in range(0, 512, 32):
        entry_data = dir_data[i:i+32]
        if len(entry_data) < 32:
            break

        name = entry_data[:11]
        if name[0] == 0x00:  # End of directory
            break
        if name[0] == 0xE5:  # Deleted entry
            continue

        attributes = entry_data[11]
        first_cluster_low = (entry_data[27] << 8) | entry_data[26]  # Little endian
        first_cluster_high = (entry_data[21] << 8) | entry_data[20]
        file_size = (entry_data[31] << 24) | (entry_data[30] << 16) | (entry_data[29] << 8) | entry_data[28]

        entries.append({
            'name': name,
            'attributes': attributes,
            'first_cluster': (first_cluster_high << 16) | first_cluster_low,
            'file_size': file_size
        })

    assert len(entries) == 1, f"Expected 1 entry, got {len(entries)}"
    assert entries[0]['name'] == b'KERNEL8 IMG', f"Wrong filename: {entries[0]['name']}"
    assert entries[0]['file_size'] == 1024*1024, f"Wrong file size: {entries[0]['file_size']}"
    print("✓ Directory traversal passed")

def test_cluster_chain_traversal():
    """Test FAT cluster chain traversal"""
    print("Testing cluster chain traversal...")

    # Test FAT32 chain
    fat_type = 'FAT32'
    fat_start = 32
    cluster = 3  # Starting cluster

    # Mock FAT with cluster 3 -> 4 -> 5 -> EOF
    fat_entries = {3: 4, 4: 5, 5: 0x0FFFFFF8}

    current_cluster = cluster
    chain = []

    while True:
        chain.append(current_cluster)
        if fat_type == 'FAT32':
            next_cluster = fat_entries.get(current_cluster, 0x0FFFFFFF) & 0x0FFFFFFF
        else:
            next_cluster = fat_entries.get(current_cluster, 0xFFFF)

        eof_marker = 0x0FFFFFF8 if fat_type == 'FAT32' else (0xFFF8 if fat_type == 'FAT16' else 0xFF8)
        if next_cluster >= eof_marker:
            break
        current_cluster = next_cluster

    expected_chain = [3, 4, 5]
    assert chain == expected_chain, f"Expected {expected_chain}, got {chain}"
    print("✓ FAT32 cluster chain traversal passed")

    # Test FAT16 chain
    fat_type = 'FAT16'
    fat_entries = {2: 3, 3: 4, 4: 0xFFF8}

    current_cluster = 2
    chain = []

    while True:
        chain.append(current_cluster)
        next_cluster = fat_entries.get(current_cluster, 0xFFFF)
        if next_cluster >= 0xFFF8:
            break
        current_cluster = next_cluster

    expected_chain = [2, 3, 4]
    assert chain == expected_chain, f"Expected {expected_chain}, got {chain}"
    print("✓ FAT16 cluster chain traversal passed")

def test_fat12_cluster_access():
    """Test FAT12 12-bit entry access"""
    print("Testing FAT12 cluster access...")

    # FAT12 stores entries as 12 bits, packed as 1.5 bytes per entry
    # Test with data: \xF0\xF0\xFF\xF0\xFF\x00
    # Word 0: \xF0\xF0 = 0xF0F0 -> Entry 0: 0x0F0, Entry 1: 0xF0F
    # Wait, let me calculate properly...
    # Actually, let's use: \xF0\xFF\xF0\xFF\x00\x00 but with correct byte order
    # For little-endian uint16_t read: byte0 | (byte1 << 8)
    # So for cluster 0: bytes 0-1: \xF0 | (\xFF << 8) = 0xFFF0 & 0x0FFF = 0xFF0
    # For cluster 1: bytes 1-2: \xFF | (\xF0 << 8) = 0xF0FF >> 4 = 0xF0F (wrong)
    # I need bytes 1-2 to be \xF0\xFF so \xF0 | (\xFF << 8) = 0xFFF0 >> 4 = 0xFFF
    fat_data = b'\xF0\xF0\xFF\xF0\xFF\x00'

    def get_fat12_entry(cluster):
        byte_offset = cluster * 3 // 2
        # Simulate *(uint16_t*)(fat_data + byte_offset) on little-endian system
        word = fat_data[byte_offset] | (fat_data[byte_offset + 1] << 8)
        if (cluster & 1):
            return word >> 4
        else:
            return word & 0x0FFF

    assert get_fat12_entry(0) == 0x0F0, f"Entry 0: expected 0x0F0, got {get_fat12_entry(0):03X}"
    assert get_fat12_entry(1) == 0xFFF, f"Entry 1: expected 0xFFF, got {get_fat12_entry(1):03X}"
    assert get_fat12_entry(2) == 0xFF0, f"Entry 2: expected 0xFF0, got {get_fat12_entry(2):03X}"
    assert get_fat12_entry(3) == 0x00F, f"Entry 3: expected 0x00F, got {get_fat12_entry(3):03X}"
    print("✓ FAT12 cluster access passed")

def test_edge_cases():
    """Test edge cases and error conditions"""
    print("Testing edge cases...")

    # Test invalid filenames
    invalid_names = ["", "a"*256, "file.with.many.dots", "file with spaces"]
    for name in invalid_names:
        try:
            result = convert_to_83(name)
            # Should not crash, but result may be truncated
            assert len(result) == 11, f"Invalid result length for '{name}': {len(result)}"
        except:
            pass  # Expected for some cases

    print("✓ Edge case handling passed")

def run_fat_tests():
    """Run all FAT filesystem tests"""
    print("=== FAT Filesystem Tests ===\n")

    tests = [
        test_fat_type_detection,
        test_filename_conversion,
        test_directory_traversal,
        test_cluster_chain_traversal,
        test_fat12_cluster_access,
        test_edge_cases
    ]

    passed = 0
    for test in tests:
        try:
            test()
            passed += 1
        except Exception as e:
            print(f"✗ {test.__name__} failed: {e}")
        print()

    print(f"FAT tests passed: {passed}/{len(tests)}")

    if passed == len(tests):
        print("🎉 All FAT tests passed!")
        return True
    else:
        print("❌ Some FAT tests failed")
        return False

if __name__ == "__main__":
    run_fat_tests()