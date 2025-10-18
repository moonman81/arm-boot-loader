/* Formal Verification Header for Bootloader */

#ifndef VERIFICATION_H
#define VERIFICATION_H

#include <stdint.h>

// Verification functions
int verification_init(void);
int verification_add_kripke_world(uint32_t world_id, const uint8_t *properties, uint32_t prop_size);
int verification_add_accessibility(uint32_t from_world, uint32_t to_world);
int verification_check_necessity(uint32_t world_id, uint8_t property_bit);
int verification_add_scott_element(uint8_t is_bottom, uint8_t level, const uint8_t *properties, uint32_t prop_size);
int verification_scott_less_equal(uint32_t elem1_idx, uint32_t elem2_idx);
int verification_add_grothendieck_site(uint32_t site_id, const uint8_t *local_data, uint32_t data_size);
int verification_check_sheaf_condition(uint32_t site_idx);
int verification_run_comprehensive_check(void);

#endif