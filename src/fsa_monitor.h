/* FSA Monitor and Interlocks Header for Bootloader */

#ifndef FSA_MONITOR_H
#define FSA_MONITOR_H

#include <stdint.h>

// FSA States - Hyper-modular with intermediate states for alternative configurations
typedef enum {
    // Power-on and early init
    STATE_POWER_ON,
    STATE_EARLY_HW_INIT,

    // Bootcode phase with intermediates
    STATE_BOOTCODE_SOURCE_SELECT,
    STATE_BOOTCODE_LOADING,
    STATE_BOOTCODE_VALIDATION,
    STATE_BOOTCODE_EXEC,
    STATE_BOOTCODE_CONFIG_PARSE,

    // Hardware initialization phases
    STATE_CORE_DRIVER_INIT,
    STATE_BSP_DRIVER_INIT,
    STATE_HW_VALIDATION,

    // Configuration phase
    STATE_CONFIG_LOADING,
    STATE_CONFIG_PARSING,
    STATE_CONFIG_VALIDATION,
    STATE_CONFIG_APPLICATION,

    // Start.elf phase with alternatives
    STATE_STARTELF_SOURCE_SELECT,
    STATE_STARTELF_LOADING,
    STATE_STARTELF_VALIDATION,
    STATE_STARTELF_EXEC,

    // Kernel phase with multiple sources and intermediates
    STATE_KERNEL_SOURCE_SELECT,
    STATE_KERNEL_LOADING,
    STATE_KERNEL_VALIDATION,
    STATE_INITRD_LOADING,
    STATE_DTB_LOADING,
    STATE_KERNEL_PARAMS_SETUP,
    STATE_KERNEL_EXEC,

    // Alternative boot paths
    STATE_NETWORK_BOOT_INIT,
    STATE_PXE_BOOT_EXEC,
    STATE_USB_BOOT_INIT,
    STATE_FAILSAFE_BOOT_INIT,
    STATE_RECOVERY_BOOT_INIT,

    // Modular component loading
    STATE_MODULE_DEPENDENCY_RESOLVE,
    STATE_MODULE_LOADING,
    STATE_MODULE_VALIDATION,

    // Security and trust states (Kripke modal necessity)
    STATE_SECURITY_ATTESTATION,
    STATE_FIRMWARE_MEASUREMENT,
    STATE_BOOT_POLICY_VALIDATION,
    STATE_TRUSTED_EXECUTION_INIT,

    // Configuration coherence (Grothendieck topology)
    STATE_CONFIGURATION_COHERENCE_CHECK,
    STATE_DEPENDENCY_GRAPH_ANALYSIS,

    // Verification states (Tarski model theory)
    STATE_SEMANTIC_VALIDATION,
    STATE_CONSISTENCY_CHECK,

    // Final states
    STATE_SUCCESS,
    STATE_FAILURE,
    STATE_HALT
} boot_state_t;

// State transition validation
typedef enum {
    TRANSITION_VALID,
    TRANSITION_INVALID,
    TRANSITION_BLOCKED
} transition_status_t;

// Safety interlock types
typedef enum {
    INTERLOCK_NONE,
    INTERLOCK_HARDWARE_NOT_READY,
    INTERLOCK_MEMORY_CORRUPTION,
    INTERLOCK_TIMEOUT,
    INTERLOCK_SECURITY_VIOLATION,
    INTERLOCK_RESOURCE_EXHAUSTED
} interlock_type_t;

// State monitoring data
typedef struct {
    boot_state_t current_state;
    boot_state_t previous_state;
    uint64_t state_entry_time;
    uint32_t state_timeout_ms;
    uint32_t retry_count;
    interlock_type_t active_interlock;
    uint8_t safety_flags;
} state_monitor_t;

// State history for debugging
#define STATE_HISTORY_SIZE 16
typedef struct {
    boot_state_t state;
    uint64_t timestamp;
    transition_status_t transition_result;
    interlock_type_t interlock;
} state_history_entry_t;

// FSA monitoring functions
void fsa_monitor_init(void);
transition_status_t fsa_validate_transition(boot_state_t from_state, boot_state_t to_state);
int fsa_check_interlocks(boot_state_t target_state);
void fsa_update_state(boot_state_t new_state);
void fsa_monitor_tick(void);
void fsa_log_transition(boot_state_t from, boot_state_t to, transition_status_t status);
void fsa_handle_timeout(void);
void fsa_activate_interlock(interlock_type_t type);
void fsa_clear_interlock(void);

// State timeout configuration - comprehensive for all intermediate states
#define TIMEOUT_POWER_ON              1000   // 1 second
#define TIMEOUT_EARLY_HW_INIT         2000   // 2 seconds
#define TIMEOUT_BOOTCODE_SOURCE_SELECT 1000  // 1 second
#define TIMEOUT_BOOTCODE_LOADING      5000   // 5 seconds
#define TIMEOUT_BOOTCODE_VALIDATION   2000   // 2 seconds
#define TIMEOUT_BOOTCODE_EXEC         3000   // 3 seconds
#define TIMEOUT_BOOTCODE_CONFIG_PARSE 2000   // 2 seconds
#define TIMEOUT_CORE_DRIVER_INIT      3000   // 3 seconds
#define TIMEOUT_BSP_DRIVER_INIT       5000   // 5 seconds
#define TIMEOUT_HW_VALIDATION         2000   // 2 seconds
#define TIMEOUT_CONFIG_LOADING        3000   // 3 seconds
#define TIMEOUT_CONFIG_PARSING        2000   // 2 seconds
#define TIMEOUT_CONFIG_VALIDATION     1000   // 1 second
#define TIMEOUT_CONFIG_APPLICATION    1000   // 1 second
#define TIMEOUT_STARTELF_SOURCE_SELECT 1000  // 1 second
#define TIMEOUT_STARTELF_LOADING      5000   // 5 seconds
#define TIMEOUT_STARTELF_VALIDATION   2000   // 2 seconds
#define TIMEOUT_STARTELF_EXEC         10000  // 10 seconds
#define TIMEOUT_KERNEL_SOURCE_SELECT  1000   // 1 second
#define TIMEOUT_KERNEL_LOADING        10000  // 10 seconds
#define TIMEOUT_KERNEL_VALIDATION     2000   // 2 seconds
#define TIMEOUT_INITRD_LOADING        5000   // 5 seconds
#define TIMEOUT_DTB_LOADING           3000   // 3 seconds
#define TIMEOUT_KERNEL_PARAMS_SETUP   1000   // 1 second
#define TIMEOUT_KERNEL_EXEC           5000   // 5 seconds
#define TIMEOUT_NETWORK_BOOT_INIT     5000   // 5 seconds
#define TIMEOUT_USB_BOOT_INIT         3000   // 3 seconds
#define TIMEOUT_FAILSAFE_BOOT_INIT    2000   // 2 seconds
#define TIMEOUT_RECOVERY_BOOT_INIT    2000   // 2 seconds
#define TIMEOUT_MODULE_DEPENDENCY_RESOLVE 2000  // 2 seconds
#define TIMEOUT_MODULE_LOADING        5000   // 5 seconds
#define TIMEOUT_MODULE_VALIDATION     2000   // 2 seconds
#define TIMEOUT_SECURITY_ATTESTATION  3000   // 3 seconds
#define TIMEOUT_FIRMWARE_MEASUREMENT  2000   // 2 seconds
#define TIMEOUT_BOOT_POLICY_VALIDATION 1500  // 1.5 seconds
#define TIMEOUT_TRUSTED_EXECUTION_INIT 2500  // 2.5 seconds
#define TIMEOUT_CONFIGURATION_COHERENCE_CHECK 2000  // 2 seconds
#define TIMEOUT_DEPENDENCY_GRAPH_ANALYSIS 3000  // 3 seconds
#define TIMEOUT_SEMANTIC_VALIDATION  2000   // 2 seconds
#define TIMEOUT_CONSISTENCY_CHECK    1500   // 1.5 seconds

// Safety flags
#define SAFETY_FLAG_HARDWARE_READY    (1 << 0)
#define SAFETY_FLAG_MEMORY_INTEGRITY  (1 << 1)
#define SAFETY_FLAG_SECURITY_VALID    (1 << 2)
#define SAFETY_FLAG_RESOURCES_OK      (1 << 3)

// Recovery actions
typedef enum {
    RECOVERY_NONE,
    RECOVERY_RETRY,
    RECOVERY_RESET,
    RECOVERY_FAILSAFE,
    RECOVERY_HALT
} recovery_action_t;

recovery_action_t fsa_determine_recovery(boot_state_t failed_state, interlock_type_t interlock);
void fsa_execute_recovery(recovery_action_t action);

// State history functions
void fsa_record_history(boot_state_t state, transition_status_t status, interlock_type_t interlock);
void fsa_dump_history(void);
state_history_entry_t* fsa_get_history(uint8_t index);

// Monitoring statistics
typedef struct {
    uint32_t total_transitions;
    uint32_t valid_transitions;
    uint32_t invalid_transitions;
    uint32_t blocked_transitions;
    uint32_t timeouts;
    uint32_t interlocks_triggered;
    uint32_t recoveries_attempted;
    uint32_t recoveries_successful;
} fsa_statistics_t;

fsa_statistics_t* fsa_get_statistics(void);

// Internal safety check function
void fsa_perform_safety_checks(void);

// External interface for main.c
extern state_monitor_t fsa_monitor;

#endif