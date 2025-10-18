/* Flattened Device Tree Parser and Manipulator */

#ifndef FDT_H
#define FDT_H

#include <stdint.h>

// FDT header structure
typedef struct {
    uint32_t magic;              // 0xd00dfeed
    uint32_t totalsize;          // Total size of DTB
    uint32_t off_dt_struct;      // Offset to structure block
    uint32_t off_dt_strings;     // Offset to strings block
    uint32_t off_mem_rsvmap;     // Offset to memory reserve map
    uint32_t version;            // FDT version
    uint32_t last_comp_version;  // Last compatible version
    uint32_t boot_cpuid_phys;    // Boot CPU ID
    uint32_t size_dt_strings;    // Size of strings block
    uint32_t size_dt_struct;     // Size of structure block
} fdt_header_t;

// FDT tokens
#define FDT_BEGIN_NODE  0x00000001
#define FDT_END_NODE    0x00000002
#define FDT_PROP        0x00000003
#define FDT_NOP         0x00000004
#define FDT_END         0x00000009

// FDT magic number
#define FDT_MAGIC       0xd00dfeed

// FDT property structure
typedef struct {
    uint32_t len;
    uint32_t nameoff;
} fdt_prop_t;

// FDT traversal callback
typedef int (*fdt_node_callback_t)(void *fdt, int node_offset, const char *name, void *context);
typedef int (*fdt_prop_callback_t)(void *fdt, int node_offset, const char *name,
                                    const void *data, uint32_t len, void *context);

// Core FDT functions
int fdt_check_header(const void *fdt);
int fdt_get_header(const void *fdt, fdt_header_t *header);
uint32_t fdt_get_totalsize(const void *fdt);

// Node navigation
int fdt_next_node(const void *fdt, int offset, int *depth);
int fdt_first_subnode(const void *fdt, int parent_offset);
int fdt_next_subnode(const void *fdt, int offset);
int fdt_parent_offset(const void *fdt, int offset);

// Node lookup
int fdt_path_offset(const void *fdt, const char *path);
int fdt_node_offset_by_compatible(const void *fdt, int start_offset, const char *compatible);
int fdt_node_offset_by_phandle(const void *fdt, uint32_t phandle);

// Node information
const char *fdt_get_name(const void *fdt, int node_offset, int *len);
int fdt_get_depth(const void *fdt, int node_offset);

// Property access
const void *fdt_getprop(const void *fdt, int node_offset, const char *name, int *len);
const char *fdt_getprop_string(const void *fdt, int node_offset, const char *name);
uint32_t fdt_getprop_u32(const void *fdt, int node_offset, const char *name);
uint64_t fdt_getprop_u64(const void *fdt, int node_offset, const char *name);

// Property modification (requires writable FDT)
int fdt_setprop(void *fdt, int node_offset, const char *name, const void *val, int len);
int fdt_setprop_u32(void *fdt, int node_offset, const char *name, uint32_t val);
int fdt_setprop_u64(void *fdt, int node_offset, const char *name, uint64_t val);
int fdt_setprop_string(void *fdt, int node_offset, const char *name, const char *str);
int fdt_delprop(void *fdt, int node_offset, const char *name);

// Node modification
int fdt_add_subnode(void *fdt, int parent_offset, const char *name);
int fdt_del_node(void *fdt, int node_offset);

// Memory reservation
int fdt_num_mem_rsv(const void *fdt);
int fdt_get_mem_rsv(const void *fdt, int n, uint64_t *address, uint64_t *size);
int fdt_add_mem_rsv(void *fdt, uint64_t address, uint64_t size);
int fdt_del_mem_rsv(void *fdt, int n);

// Utility functions
int fdt_traverse(const void *fdt, fdt_node_callback_t node_cb, fdt_prop_callback_t prop_cb, void *context);
void fdt_dump(const void *fdt);

// Size calculation
int fdt_open_into(const void *fdt, void *buf, int bufsize);
int fdt_pack(void *fdt);

// Common property helpers
int fdt_get_reg(const void *fdt, int node_offset, int index, uint64_t *addr, uint64_t *size);
int fdt_get_interrupts(const void *fdt, int node_offset, uint32_t *interrupts, int max_count);
const char *fdt_get_alias(const void *fdt, const char *alias_name);

// Error codes
#define FDT_ERR_NOTFOUND       -1   // Node/property not found
#define FDT_ERR_BADMAGIC       -2   // Bad magic number
#define FDT_ERR_BADVERSION     -3   // Unsupported version
#define FDT_ERR_BADSTRUCTURE   -4   // Malformed structure
#define FDT_ERR_TRUNCATED      -5   // Truncated DTB
#define FDT_ERR_NOSPACE        -6   // Insufficient space
#define FDT_ERR_BADOFFSET      -7   // Invalid offset
#define FDT_ERR_BADPATH        -8   // Invalid path
#define FDT_ERR_BADPHANDLE     -9   // Invalid phandle
#define FDT_ERR_BADSTATE       -10  // Bad state

#endif
