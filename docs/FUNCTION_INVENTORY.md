## Function Inventory

Automatically generated function list from codebase.

### main.c

- `kernel_handover`
- `main`

### uart.c

- `uart_init`
- `uart_putc`
- `uart_puts`
- `uart_available`
- `uart_getc`
- `uart_getc_timeout`
- `uart_gets`
- `uart_flush_rx`

### gpio.c

- `gpio_init`
- `gpio_set_function`
- `gpio_set_pull`
- `gpio_set`
- `gpio_clear`
- `gpio_read`
- `gpio_toggle`
- `gpio_enable_interrupt`
- `gpio_disable_interrupt`
- `gpio_clear_interrupt`

### timer.c

- `timer_init`
- `timer_get_counter`
- `timer_delay_us`
- `timer_delay_ms`
- `timer_get_ticks`
- `timer_arm_init`
- `timer_arm_set_load`
- `timer_arm_enable_interrupt`
- `timer_arm_disable_interrupt`
- `timer_arm_clear_interrupt`

### sd.c

- `sd_init`
- `sd_read_sector`
- `partition_detect`
- `partition_read_boot_sector`
- `fat_init`
- `fat_find_file`
- `fat_read_file`
- `sd_load_file`

### verification.c

- `verification_init`
- `verification_add_kripke_world`
- `verification_add_accessibility`
- `verification_check_necessity`
- `verification_add_scott_element`
- `verification_scott_less_equal`
- `verification_add_grothendieck_site`
- `verification_check_sheaf_condition`
- `verification_add_tarski_element`
- `verification_add_tarski_interpretation`
- `verification_check_tarski_satisfaction`
- `verification_get_tarski_interpretation`
- `verification_add_type_theory_type`
- `verification_add_type_theory_function`
- `verification_add_type_theory_dependent`
- `verification_check_type_inhabitation`
- `verification_check_function_type`
- `verification_add_homotopy_path`
- `verification_add_homotopy_equivalence`
- `verification_add_homotopy_group`
- `verification_check_path_homotopy`
- `verification_check_fundamental_group`
- `verification_add_grothendieck_category`
- `verification_add_category_object`
- `verification_add_category_morphism`
- `verification_add_grothendieck_functor`
- `verification_check_functoriality`
- `verification_check_category_axioms`
- `verification_add_unified_mapping`
- `verification_check_mathematical_consistency`
- `verification_add_universal_property`
- `verification_add_limit_colimit`
- `verification_add_topos`
- `verification_add_sheaf`
- `verification_add_grothendieck_nat_trans`
- `verification_check_universal_property`
- `verification_check_topos_properties`
- `verification_add_kripke_frame`
- `verification_add_kripke_world_to_frame`
- `verification_add_kripke_relation`
- `verification_add_kripke_model`
- `verification_add_modal_formula`
- `verification_check_modal_satisfaction`
- `verification_add_scott_domain`
- `verification_add_partial_order`
- `verification_add_directed_set`
- `verification_add_domain_element`
- `verification_add_fixed_point`
- `verification_add_continuous_function`
- `verification_check_domain_properties`
- `verification_check_fixed_point`
- `verification_add_dependent_type`
- `verification_add_homotopy_space`
- `verification_check_dependent_type`
- `verification_check_homotopy_space`
- `verification_add_fsa_state`
- `verification_add_fsa_transition`
- `verification_add_transition_condition`
- `verification_add_boot_stage`
- `verification_add_firmware_component`
- `verification_check_fsa_determinism`
- `verification_check_boot_completeness`
- `verification_initialize_standard_instances`
- `verification_run_comprehensive_check`

