/* Device Tree Blob Creation for Raspberry Pi */

#include "dtb.h"
#include "uart.h"

// Custom memcpy for freestanding
static void *memcpy(void *dest, const void *src, uint32_t n) {
    uint8_t *d = dest;
    const uint8_t *s = src;
    while (n--) *d++ = *s++;
    return dest;
}

// Simple DTB creation for Raspberry Pi 4
// This is a minimal implementation - real DTBs are more complex

#define DTB_HEADER_SIZE 40
#define FDT_BEGIN_NODE 0x00000001
#define FDT_END_NODE   0x00000002
#define FDT_PROP       0x00000003
#define FDT_END        0x00000009

typedef struct {
    uint32_t magic;
    uint32_t totalsize;
    uint32_t off_dt_struct;
    uint32_t off_dt_strings;
    uint32_t off_mem_rsvmap;
    uint32_t version;
    uint32_t last_comp_version;
    uint32_t boot_cpuid_phys;
    uint32_t size_dt_strings;
    uint32_t size_dt_struct;
} fdt_header_t;

static void write_be32(uint8_t *buf, uint32_t val) {
    buf[0] = (val >> 24) & 0xFF;
    buf[1] = (val >> 16) & 0xFF;
    buf[2] = (val >> 8) & 0xFF;
    buf[3] = val & 0xFF;
}

static void write_be64(uint8_t *buf, uint64_t val) {
    write_be32(buf, (uint32_t)(val >> 32));
    write_be32(buf + 4, (uint32_t)val);
}

unsigned long dtb_create(unsigned long addr) {
    uint8_t *dtb = (uint8_t*)addr;
    uint32_t offset = 0;

    uart_puts("Creating minimal DTB...\n");

    // DTB header
    fdt_header_t *header = (fdt_header_t*)dtb;
    header->magic = 0xd00dfeed; // Big endian
    header->totalsize = 1024; // Placeholder
    header->off_dt_struct = DTB_HEADER_SIZE;
    header->off_dt_strings = 512; // Placeholder
    header->off_mem_rsvmap = DTB_HEADER_SIZE - 16;
    header->version = 17;
    header->last_comp_version = 16;
    header->boot_cpuid_phys = 0;
    header->size_dt_strings = 256;
    header->size_dt_struct = 256;

    offset = DTB_HEADER_SIZE;

    // Memory reservation map (empty)
    write_be64(dtb + offset, 0); offset += 8;
    write_be64(dtb + offset, 0); offset += 8;

    // Device tree structure
    write_be32(dtb + offset, FDT_BEGIN_NODE); offset += 4;
    // Root node name (null terminated, padded to 4 bytes)
    *(uint32_t*)(dtb + offset) = 0; offset += 4;

    // model property
    write_be32(dtb + offset, FDT_PROP); offset += 4;
    write_be32(dtb + offset, 20); offset += 4; // Length
    write_be32(dtb + offset, 0); offset += 4; // Name offset
    memcpy(dtb + offset, "Raspberry Pi 4 Model B", 20); offset += 20;

    // compatible property
    write_be32(dtb + offset, FDT_PROP); offset += 4;
    write_be32(dtb + offset, 24); offset += 4;
    write_be32(dtb + offset, 20); offset += 4; // Name offset
    memcpy(dtb + offset, "raspberrypi,4-model-b\0broadcom,bcm2711", 24); offset += 24;

    // memory node
    write_be32(dtb + offset, FDT_BEGIN_NODE); offset += 4;
    memcpy(dtb + offset, "memory", 7); offset += 8; // Null padded

    // reg property for memory
    write_be32(dtb + offset, FDT_PROP); offset += 4;
    write_be32(dtb + offset, 16); offset += 4; // 2 cells * 8 bytes
    write_be32(dtb + offset, 40); offset += 4; // Name offset
    write_be64(dtb + offset, 0); offset += 8; // Start address
    write_be64(dtb + offset, 0x40000000); offset += 8; // Size (1GB)

    write_be32(dtb + offset, FDT_END_NODE); offset += 4;

    // cpus node
    write_be32(dtb + offset, FDT_BEGIN_NODE); offset += 4;
    memcpy(dtb + offset, "cpus", 5); offset += 8;

    // cpu@0 node
    write_be32(dtb + offset, FDT_BEGIN_NODE); offset += 4;
    memcpy(dtb + offset, "cpu@0", 6); offset += 8;

    // compatible
    write_be32(dtb + offset, FDT_PROP); offset += 4;
    write_be32(dtb + offset, 12); offset += 4;
    write_be32(dtb + offset, 64); offset += 4;
    memcpy(dtb + offset, "arm,cortex-a72", 12); offset += 12;

    write_be32(dtb + offset, FDT_END_NODE); offset += 4;

    write_be32(dtb + offset, FDT_END_NODE); offset += 4;

    write_be32(dtb + offset, FDT_END_NODE); offset += 4;
    write_be32(dtb + offset, FDT_END); offset += 4;

    // Strings section
    uint32_t strings_offset = 512;
    memcpy(dtb + strings_offset, "model", 6); strings_offset += 6;
    memcpy(dtb + strings_offset, "compatible", 11); strings_offset += 12; // padded
    memcpy(dtb + strings_offset, "reg", 4); strings_offset += 4;

    // Update header sizes
    header->size_dt_struct = offset - DTB_HEADER_SIZE;
    header->size_dt_strings = strings_offset - 512;
    header->totalsize = strings_offset;

    uart_puts("DTB created\n");
    return addr;
}