/* Flattened Device Tree Parser Implementation */

#include "fdt.h"
#include "uart.h"

#ifndef NULL
#define NULL ((void *)0)
#endif

// Memory functions for freestanding environment
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

static int strcmp(const char *s1, const char *s2) {
    while (*s1 && *s2 && *s1 == *s2) { s1++; s2++; }
    return *s1 - *s2;
}

static int strncmp(const char *s1, const char *s2, uint32_t n) {
    while (n && *s1 && *s2 && *s1 == *s2) { s1++; s2++; n--; }
    return n ? (*s1 - *s2) : 0;
}

static uint32_t strlen(const char *s) {
    uint32_t len = 0;
    while (*s++) len++;
    return len;
}

// Byte swapping for big-endian FDT
static uint32_t fdt32_to_cpu(uint32_t x) {
    return ((x & 0xFF000000) >> 24) |
           ((x & 0x00FF0000) >> 8) |
           ((x & 0x0000FF00) << 8) |
           ((x & 0x000000FF) << 24);
}

static uint64_t fdt64_to_cpu(uint64_t x) {
    return ((uint64_t)fdt32_to_cpu(x & 0xFFFFFFFF) << 32) |
           fdt32_to_cpu(x >> 32);
}

static uint32_t cpu_to_fdt32(uint32_t x) {
    return fdt32_to_cpu(x); // Same operation
}

static uint64_t cpu_to_fdt64(uint64_t x) {
    return fdt64_to_cpu(x); // Same operation
}

// Alignment helpers
static inline uint32_t align_up(uint32_t x, uint32_t align) {
    return (x + align - 1) & ~(align - 1);
}

// Get pointer to FDT structure block
static const uint32_t *fdt_get_struct(const void *fdt, uint32_t offset) {
    const fdt_header_t *hdr = (const fdt_header_t *)fdt;
    return (const uint32_t *)((const char *)fdt + fdt32_to_cpu(hdr->off_dt_struct) + offset);
}

// Get pointer to FDT strings block
static const char *fdt_get_string(const void *fdt, uint32_t offset) {
    const fdt_header_t *hdr = (const fdt_header_t *)fdt;
    return (const char *)fdt + fdt32_to_cpu(hdr->off_dt_strings) + offset;
}

// Check FDT header validity
int fdt_check_header(const void *fdt) {
    const fdt_header_t *hdr = (const fdt_header_t *)fdt;

    if (fdt32_to_cpu(hdr->magic) != FDT_MAGIC) {
        return FDT_ERR_BADMAGIC;
    }

    uint32_t version = fdt32_to_cpu(hdr->version);
    if (version < 16 || version > 17) {
        return FDT_ERR_BADVERSION;
    }

    return 0;
}

// Get FDT header
int fdt_get_header(const void *fdt, fdt_header_t *header) {
    const fdt_header_t *hdr = (const fdt_header_t *)fdt;

    header->magic = fdt32_to_cpu(hdr->magic);
    header->totalsize = fdt32_to_cpu(hdr->totalsize);
    header->off_dt_struct = fdt32_to_cpu(hdr->off_dt_struct);
    header->off_dt_strings = fdt32_to_cpu(hdr->off_dt_strings);
    header->off_mem_rsvmap = fdt32_to_cpu(hdr->off_mem_rsvmap);
    header->version = fdt32_to_cpu(hdr->version);
    header->last_comp_version = fdt32_to_cpu(hdr->last_comp_version);
    header->boot_cpuid_phys = fdt32_to_cpu(hdr->boot_cpuid_phys);
    header->size_dt_strings = fdt32_to_cpu(hdr->size_dt_strings);
    header->size_dt_struct = fdt32_to_cpu(hdr->size_dt_struct);

    return 0;
}

// Get total size
uint32_t fdt_get_totalsize(const void *fdt) {
    const fdt_header_t *hdr = (const fdt_header_t *)fdt;
    return fdt32_to_cpu(hdr->totalsize);
}

// Find next node in tree
int fdt_next_node(const void *fdt, int offset, int *depth) {
    const fdt_header_t *hdr = (const fdt_header_t *)fdt;
    uint32_t struct_size = fdt32_to_cpu(hdr->size_dt_struct);
    const uint32_t *ptr = fdt_get_struct(fdt, offset);
    int current_depth = depth ? *depth : 0;

    while ((const char *)ptr < (const char *)fdt + fdt32_to_cpu(hdr->off_dt_struct) + struct_size) {
        uint32_t tag = fdt32_to_cpu(*ptr);

        switch (tag) {
            case FDT_BEGIN_NODE:
                if (offset > 0) {
                    current_depth++;
                    if (depth) *depth = current_depth;
                    return (const char *)ptr - (const char *)fdt_get_struct(fdt, 0);
                }
                // Skip node name
                ptr++;
                while (*(const char *)ptr) ptr = (const uint32_t *)((const char *)ptr + 1);
                ptr = (const uint32_t *)align_up((uint32_t)(const char *)ptr + 1, 4);
                offset = (const char *)ptr - (const char *)fdt_get_struct(fdt, 0);
                break;

            case FDT_END_NODE:
                current_depth--;
                if (current_depth < 0) return FDT_ERR_BADSTRUCTURE;
                ptr++;
                break;

            case FDT_PROP:
                ptr++; // Skip tag
                ptr += 2; // Skip len and nameoff
                {
                    uint32_t len = fdt32_to_cpu(*(ptr - 2));
                    ptr = (const uint32_t *)align_up((uint32_t)ptr + len, 4);
                }
                break;

            case FDT_NOP:
                ptr++;
                break;

            case FDT_END:
                return FDT_ERR_NOTFOUND;

            default:
                return FDT_ERR_BADSTRUCTURE;
        }
    }

    return FDT_ERR_NOTFOUND;
}

// Get property
const void *fdt_getprop(const void *fdt, int node_offset, const char *name, int *len) {
    const fdt_header_t *hdr = (const fdt_header_t *)fdt;
    const uint32_t *ptr = fdt_get_struct(fdt, node_offset);

    // Skip BEGIN_NODE tag
    uint32_t tag = fdt32_to_cpu(*ptr++);
    if (tag != FDT_BEGIN_NODE) return NULL;

    // Skip node name
    while (*(const char *)ptr) ptr = (const uint32_t *)((const char *)ptr + 1);
    ptr = (const uint32_t *)align_up((uint32_t)(const char *)ptr + 1, 4);

    // Search properties
    while (1) {
        tag = fdt32_to_cpu(*ptr++);

        if (tag == FDT_PROP) {
            uint32_t prop_len = fdt32_to_cpu(*ptr++);
            uint32_t nameoff = fdt32_to_cpu(*ptr++);
            const char *prop_name = fdt_get_string(fdt, nameoff);

            if (strcmp(prop_name, name) == 0) {
                if (len) *len = prop_len;
                return ptr;
            }

            ptr = (const uint32_t *)align_up((uint32_t)ptr + prop_len, 4);
        } else if (tag == FDT_BEGIN_NODE || tag == FDT_END_NODE) {
            break;
        } else if (tag == FDT_NOP) {
            continue;
        } else {
            break;
        }
    }

    return NULL;
}

// Get string property
const char *fdt_getprop_string(const void *fdt, int node_offset, const char *name) {
    int len;
    const char *str = (const char *)fdt_getprop(fdt, node_offset, name, &len);
    return (str && len > 0) ? str : NULL;
}

// Get u32 property
uint32_t fdt_getprop_u32(const void *fdt, int node_offset, const char *name) {
    int len;
    const uint32_t *val = (const uint32_t *)fdt_getprop(fdt, node_offset, name, &len);
    return (val && len == 4) ? fdt32_to_cpu(*val) : 0;
}

// Get u64 property
uint64_t fdt_getprop_u64(const void *fdt, int node_offset, const char *name) {
    int len;
    const uint64_t *val = (const uint64_t *)fdt_getprop(fdt, node_offset, name, &len);
    return (val && len == 8) ? fdt64_to_cpu(*val) : 0;
}

// Get node name
const char *fdt_get_name(const void *fdt, int node_offset, int *len) {
    const uint32_t *ptr = fdt_get_struct(fdt, node_offset);
    uint32_t tag = fdt32_to_cpu(*ptr++);

    if (tag != FDT_BEGIN_NODE) return NULL;

    const char *name = (const char *)ptr;
    if (len) {
        *len = 0;
        while (name[*len]) (*len)++;
    }

    return name;
}

// Find node by path
int fdt_path_offset(const void *fdt, const char *path) {
    if (!path || path[0] != '/') return FDT_ERR_BADPATH;

    if (path[1] == '\0') return 0; // Root node

    int offset = 0;
    int depth = 0;
    const char *path_ptr = path + 1; // Skip initial '/'

    while (*path_ptr) {
        // Find next '/' or end of string
        const char *next_slash = path_ptr;
        while (*next_slash && *next_slash != '/') next_slash++;

        uint32_t component_len = next_slash - path_ptr;

        // Find matching child node
        offset = fdt_next_node(fdt, offset, &depth);
        while (offset >= 0) {
            int name_len;
            const char *node_name = fdt_get_name(fdt, offset, &name_len);

            if (node_name && name_len == component_len &&
                strncmp(node_name, path_ptr, component_len) == 0) {
                break; // Found
            }

            offset = fdt_next_node(fdt, offset, &depth);
        }

        if (offset < 0) return FDT_ERR_NOTFOUND;

        // Move to next component
        path_ptr = next_slash;
        if (*path_ptr == '/') path_ptr++;
    }

    return offset;
}

// Get reg property (address and size)
int fdt_get_reg(const void *fdt, int node_offset, int index, uint64_t *addr, uint64_t *size) {
    int len;
    const uint32_t *reg = (const uint32_t *)fdt_getprop(fdt, node_offset, "reg", &len);
    if (!reg) return FDT_ERR_NOTFOUND;

    // Assume #address-cells=2 and #size-cells=1 for now (common for Pi)
    int cells_per_entry = 3; // 2 for address, 1 for size
    int offset_cells = index * cells_per_entry;

    if (offset_cells * 4 + 12 > len) return FDT_ERR_NOTFOUND;

    if (addr) {
        *addr = ((uint64_t)fdt32_to_cpu(reg[offset_cells]) << 32) |
                fdt32_to_cpu(reg[offset_cells + 1]);
    }

    if (size) {
        *size = fdt32_to_cpu(reg[offset_cells + 2]);
    }

    return 0;
}

// Dump FDT for debugging
void fdt_dump(const void *fdt) {
    fdt_header_t hdr;
    fdt_get_header(fdt, &hdr);

    uart_puts("FDT Header:\n");
    uart_puts("  Magic: 0x");
    // Would need hex print function
    uart_puts("\n  Total size: ");
    // Would need decimal print function
    uart_puts(" bytes\n");
}
