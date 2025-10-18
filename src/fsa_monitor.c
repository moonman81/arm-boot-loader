/* FSA Monitor and Interlocks Implementation for Bootloader */

#include "fsa_monitor.h"
#include "uart.h"
#include "timer.h"
#include "gpio.h"

#define NULL ((void*)0)

// Forward declarations
void fsa_perform_safety_checks(void);

// Global monitor state
state_monitor_t fsa_monitor;

// State history
static state_history_entry_t state_history[STATE_HISTORY_SIZE];
static uint8_t history_index = 0;

// Statistics
static fsa_statistics_t fsa_stats = {0};

// State timeout configuration - comprehensive for all states
static const uint32_t state_timeouts[] = {
    [STATE_POWER_ON] = TIMEOUT_POWER_ON,
    [STATE_EARLY_HW_INIT] = TIMEOUT_EARLY_HW_INIT,
    [STATE_BOOTCODE_SOURCE_SELECT] = TIMEOUT_BOOTCODE_SOURCE_SELECT,
    [STATE_BOOTCODE_LOADING] = TIMEOUT_BOOTCODE_LOADING,
    [STATE_BOOTCODE_VALIDATION] = TIMEOUT_BOOTCODE_VALIDATION,
    [STATE_BOOTCODE_EXEC] = TIMEOUT_BOOTCODE_EXEC,
    [STATE_BOOTCODE_CONFIG_PARSE] = TIMEOUT_BOOTCODE_CONFIG_PARSE,
    [STATE_CORE_DRIVER_INIT] = TIMEOUT_CORE_DRIVER_INIT,
    [STATE_BSP_DRIVER_INIT] = TIMEOUT_BSP_DRIVER_INIT,
    [STATE_HW_VALIDATION] = TIMEOUT_HW_VALIDATION,
    [STATE_CONFIG_LOADING] = TIMEOUT_CONFIG_LOADING,
    [STATE_CONFIG_PARSING] = TIMEOUT_CONFIG_PARSING,
    [STATE_CONFIG_VALIDATION] = TIMEOUT_CONFIG_VALIDATION,
    [STATE_CONFIG_APPLICATION] = TIMEOUT_CONFIG_APPLICATION,
    [STATE_STARTELF_SOURCE_SELECT] = TIMEOUT_STARTELF_SOURCE_SELECT,
    [STATE_STARTELF_LOADING] = TIMEOUT_STARTELF_LOADING,
    [STATE_STARTELF_VALIDATION] = TIMEOUT_STARTELF_VALIDATION,
    [STATE_STARTELF_EXEC] = TIMEOUT_STARTELF_EXEC,
    [STATE_KERNEL_SOURCE_SELECT] = TIMEOUT_KERNEL_SOURCE_SELECT,
    [STATE_KERNEL_LOADING] = TIMEOUT_KERNEL_LOADING,
    [STATE_KERNEL_VALIDATION] = TIMEOUT_KERNEL_VALIDATION,
    [STATE_INITRD_LOADING] = TIMEOUT_INITRD_LOADING,
    [STATE_DTB_LOADING] = TIMEOUT_DTB_LOADING,
    [STATE_KERNEL_PARAMS_SETUP] = TIMEOUT_KERNEL_PARAMS_SETUP,
    [STATE_KERNEL_EXEC] = TIMEOUT_KERNEL_EXEC,
    [STATE_NETWORK_BOOT_INIT] = TIMEOUT_NETWORK_BOOT_INIT,
    [STATE_PXE_BOOT_EXEC] = TIMEOUT_NETWORK_BOOT_INIT, // Same timeout as network boot
    [STATE_USB_BOOT_INIT] = TIMEOUT_USB_BOOT_INIT,
    [STATE_FAILSAFE_BOOT_INIT] = TIMEOUT_FAILSAFE_BOOT_INIT,
    [STATE_RECOVERY_BOOT_INIT] = TIMEOUT_RECOVERY_BOOT_INIT,
    [STATE_MODULE_DEPENDENCY_RESOLVE] = TIMEOUT_MODULE_DEPENDENCY_RESOLVE,
    [STATE_MODULE_LOADING] = TIMEOUT_MODULE_LOADING,
    [STATE_MODULE_VALIDATION] = TIMEOUT_MODULE_VALIDATION,
    [STATE_SECURITY_ATTESTATION] = TIMEOUT_SECURITY_ATTESTATION,
    [STATE_FIRMWARE_MEASUREMENT] = TIMEOUT_FIRMWARE_MEASUREMENT,
    [STATE_BOOT_POLICY_VALIDATION] = TIMEOUT_BOOT_POLICY_VALIDATION,
    [STATE_TRUSTED_EXECUTION_INIT] = TIMEOUT_TRUSTED_EXECUTION_INIT,
    [STATE_CONFIGURATION_COHERENCE_CHECK] = TIMEOUT_CONFIGURATION_COHERENCE_CHECK,
    [STATE_DEPENDENCY_GRAPH_ANALYSIS] = TIMEOUT_DEPENDENCY_GRAPH_ANALYSIS,
    [STATE_SEMANTIC_VALIDATION] = TIMEOUT_SEMANTIC_VALIDATION,
    [STATE_CONSISTENCY_CHECK] = TIMEOUT_CONSISTENCY_CHECK,
    [STATE_SUCCESS] = 0,  // No timeout for success
    [STATE_FAILURE] = 0,  // No timeout for failure
    [STATE_HALT] = 0      // No timeout for halt
};

// Valid state transitions - dynamic validation for hyper-flexible bootloader
// Instead of a large static matrix, use transition rules for better maintainability
static int is_valid_transition(boot_state_t from, boot_state_t to) {
    // Power-on sequence
    if (from == STATE_POWER_ON) {
        return (to == STATE_EARLY_HW_INIT);
    }

    // Early hardware to bootcode
    if (from == STATE_EARLY_HW_INIT) {
        return (to == STATE_BOOTCODE_SOURCE_SELECT || to == STATE_FAILURE);
    }

    // Bootcode phase
    if (from == STATE_BOOTCODE_SOURCE_SELECT) {
        return (to == STATE_BOOTCODE_LOADING || to == STATE_NETWORK_BOOT_INIT || to == STATE_USB_BOOT_INIT);
    }
    if (from == STATE_BOOTCODE_LOADING) {
        return (to == STATE_BOOTCODE_VALIDATION || to == STATE_FAILURE);
    }
    if (from == STATE_BOOTCODE_VALIDATION) {
        return (to == STATE_BOOTCODE_EXEC || to == STATE_FAILURE);
    }
    if (from == STATE_BOOTCODE_EXEC) {
        return (to == STATE_BOOTCODE_CONFIG_PARSE || to == STATE_CORE_DRIVER_INIT);
    }
    if (from == STATE_BOOTCODE_CONFIG_PARSE) {
        return (to == STATE_CORE_DRIVER_INIT);
    }

    // Hardware initialization
    if (from == STATE_CORE_DRIVER_INIT) {
        return (to == STATE_BSP_DRIVER_INIT || to == STATE_FAILURE);
    }
    if (from == STATE_BSP_DRIVER_INIT) {
        return (to == STATE_HW_VALIDATION || to == STATE_FAILURE);
    }
    if (from == STATE_HW_VALIDATION) {
        return (to == STATE_CONFIG_LOADING || to == STATE_FAILURE);
    }

    // Configuration phase
    if (from == STATE_CONFIG_LOADING) {
        return (to == STATE_CONFIG_PARSING || to == STATE_STARTELF_SOURCE_SELECT);
    }
    if (from == STATE_CONFIG_PARSING) {
        return (to == STATE_CONFIG_VALIDATION);
    }
    if (from == STATE_CONFIG_VALIDATION) {
        return (to == STATE_CONFIG_APPLICATION);
    }
    if (from == STATE_CONFIG_APPLICATION) {
        return (to == STATE_STARTELF_SOURCE_SELECT);
    }

    // Start.elf phase
    if (from == STATE_STARTELF_SOURCE_SELECT) {
        return (to == STATE_STARTELF_LOADING || to == STATE_NETWORK_BOOT_INIT || to == STATE_USB_BOOT_INIT);
    }
    if (from == STATE_STARTELF_LOADING) {
        return (to == STATE_STARTELF_VALIDATION || to == STATE_FAILURE);
    }
    if (from == STATE_STARTELF_VALIDATION) {
        return (to == STATE_STARTELF_EXEC || to == STATE_FAILURE);
    }
    if (from == STATE_STARTELF_EXEC) {
        return (to == STATE_KERNEL_SOURCE_SELECT);
    }

    // Kernel phase
    if (from == STATE_KERNEL_SOURCE_SELECT) {
        return (to == STATE_KERNEL_LOADING || to == STATE_NETWORK_BOOT_INIT || to == STATE_USB_BOOT_INIT || to == STATE_MODULE_DEPENDENCY_RESOLVE);
    }
    if (from == STATE_KERNEL_LOADING) {
        return (to == STATE_KERNEL_VALIDATION || to == STATE_FAILURE);
    }
    if (from == STATE_KERNEL_VALIDATION) {
        return (to == STATE_INITRD_LOADING || to == STATE_DTB_LOADING || to == STATE_KERNEL_PARAMS_SETUP);
    }
    if (from == STATE_INITRD_LOADING) {
        return (to == STATE_DTB_LOADING);
    }
    if (from == STATE_DTB_LOADING) {
        return (to == STATE_KERNEL_PARAMS_SETUP);
    }
    if (from == STATE_KERNEL_PARAMS_SETUP) {
        return (to == STATE_KERNEL_EXEC);
    }
    if (from == STATE_KERNEL_EXEC) {
        return (to == STATE_SUCCESS || to == STATE_FAILURE);
    }

    // Alternative boot paths
    if (from == STATE_NETWORK_BOOT_INIT) {
        return (to == STATE_KERNEL_LOADING || to == STATE_STARTELF_LOADING || to == STATE_BOOTCODE_LOADING || to == STATE_PXE_BOOT_EXEC);
    }
    if (from == STATE_PXE_BOOT_EXEC) {
        return (to == STATE_SUCCESS || to == STATE_FAILURE);
    }
    if (from == STATE_USB_BOOT_INIT) {
        return (to == STATE_KERNEL_LOADING || to == STATE_STARTELF_LOADING || to == STATE_BOOTCODE_LOADING);
    }
    if (from == STATE_FAILSAFE_BOOT_INIT) {
        return (to == STATE_KERNEL_LOADING || to == STATE_STARTELF_LOADING);
    }
    if (from == STATE_RECOVERY_BOOT_INIT) {
        return (to == STATE_BOOTCODE_LOADING || to == STATE_CONFIG_LOADING);
    }

    // Modular loading
    if (from == STATE_MODULE_DEPENDENCY_RESOLVE) {
        return (to == STATE_MODULE_LOADING || to == STATE_KERNEL_LOADING);
    }
    if (from == STATE_MODULE_LOADING) {
        return (to == STATE_MODULE_VALIDATION || to == STATE_FAILURE);
    }
    if (from == STATE_MODULE_VALIDATION) {
        return (to == STATE_KERNEL_LOADING || to == STATE_SUCCESS);
    }

    // Security and trust states (Kripke modal necessity - must happen)
    if (from == STATE_HW_VALIDATION) {
        return (to == STATE_SECURITY_ATTESTATION || to == STATE_CONFIG_LOADING);
    }
    if (from == STATE_SECURITY_ATTESTATION) {
        return (to == STATE_FIRMWARE_MEASUREMENT || to == STATE_FAILURE);
    }
    if (from == STATE_FIRMWARE_MEASUREMENT) {
        return (to == STATE_BOOT_POLICY_VALIDATION || to == STATE_FAILURE);
    }
    if (from == STATE_BOOT_POLICY_VALIDATION) {
        return (to == STATE_TRUSTED_EXECUTION_INIT || to == STATE_FAILURE);
    }
    if (from == STATE_TRUSTED_EXECUTION_INIT) {
        return (to == STATE_CONFIG_LOADING || to == STATE_FAILURE);
    }

    // Configuration coherence (Grothendieck topology - local-to-global consistency)
    if (from == STATE_CONFIG_PARSING) {
        return (to == STATE_CONFIGURATION_COHERENCE_CHECK || to == STATE_CONFIG_VALIDATION);
    }
    if (from == STATE_CONFIGURATION_COHERENCE_CHECK) {
        return (to == STATE_DEPENDENCY_GRAPH_ANALYSIS || to == STATE_FAILURE);
    }
    if (from == STATE_DEPENDENCY_GRAPH_ANALYSIS) {
        return (to == STATE_CONFIG_VALIDATION || to == STATE_FAILURE);
    }

    // Verification states (Tarski model theory - semantic validation)
    if (from == STATE_CONFIG_VALIDATION) {
        return (to == STATE_SEMANTIC_VALIDATION || to == STATE_CONFIG_APPLICATION);
    }
    if (from == STATE_SEMANTIC_VALIDATION) {
        return (to == STATE_CONSISTENCY_CHECK || to == STATE_FAILURE);
    }
    if (from == STATE_CONSISTENCY_CHECK) {
        return (to == STATE_CONFIG_APPLICATION || to == STATE_FAILURE);
    }

    // Terminal states
    if (from == STATE_SUCCESS || from == STATE_FAILURE || from == STATE_HALT) {
        return 0; // No transitions from terminal states
    }

    return 0; // Invalid transition
}

void fsa_monitor_init(void) {
    fsa_monitor.current_state = STATE_POWER_ON;
    fsa_monitor.previous_state = STATE_POWER_ON;
    fsa_monitor.state_entry_time = timer_get_counter();
    fsa_monitor.state_timeout_ms = state_timeouts[STATE_POWER_ON];
    fsa_monitor.retry_count = 0;
    fsa_monitor.active_interlock = INTERLOCK_NONE;
    fsa_monitor.safety_flags = SAFETY_FLAG_HARDWARE_READY | SAFETY_FLAG_MEMORY_INTEGRITY |
                              SAFETY_FLAG_SECURITY_VALID | SAFETY_FLAG_RESOURCES_OK;

    uart_puts("FSA Monitor initialized\n");
}

transition_status_t fsa_validate_transition(boot_state_t from_state, boot_state_t to_state) {
    // Check if transition is valid using dynamic rules
    if (!is_valid_transition(from_state, to_state)) {
        uart_puts("FSA: Invalid transition attempted\n");
        fsa_stats.invalid_transitions++;
        return TRANSITION_INVALID;
    }

    // Check for active interlocks
    if (fsa_check_interlocks(to_state) != 0) {
        uart_puts("FSA: Transition blocked by interlock\n");
        fsa_stats.blocked_transitions++;
        return TRANSITION_BLOCKED;
    }

    fsa_stats.valid_transitions++;
    return TRANSITION_VALID;
}

int fsa_check_interlocks(boot_state_t target_state) {
    // Hardware readiness checks
    if (!(fsa_monitor.safety_flags & SAFETY_FLAG_HARDWARE_READY)) {
        if (target_state >= STATE_STARTELF_LOADING) {
            fsa_activate_interlock(INTERLOCK_HARDWARE_NOT_READY);
            return -1;
        }
    }

    // Memory integrity for kernel operations
    if (!(fsa_monitor.safety_flags & SAFETY_FLAG_MEMORY_INTEGRITY)) {
        if (target_state >= STATE_KERNEL_LOADING) {
            fsa_activate_interlock(INTERLOCK_MEMORY_CORRUPTION);
            return -1;
        }
    }

    // Security validation for kernel execution
    if (!(fsa_monitor.safety_flags & SAFETY_FLAG_SECURITY_VALID)) {
        if (target_state >= STATE_KERNEL_EXEC) {
            fsa_activate_interlock(INTERLOCK_SECURITY_VIOLATION);
            return -1;
        }
    }

    return 0; // No interlocks
}

void fsa_update_state(boot_state_t new_state) {
    transition_status_t status = fsa_validate_transition(fsa_monitor.current_state, new_state);

    if (status == TRANSITION_VALID) {
        fsa_monitor.previous_state = fsa_monitor.current_state;
        fsa_monitor.current_state = new_state;
        fsa_monitor.state_entry_time = timer_get_counter();
        fsa_monitor.state_timeout_ms = state_timeouts[new_state];
        fsa_monitor.retry_count = 0;

        fsa_log_transition(fsa_monitor.previous_state, new_state, status);
        fsa_record_history(new_state, status, INTERLOCK_NONE);
    } else {
        // Handle invalid/blocked transition
        fsa_log_transition(fsa_monitor.current_state, new_state, status);
        fsa_record_history(fsa_monitor.current_state, status, fsa_monitor.active_interlock);

        // Trigger recovery if blocked
        if (status == TRANSITION_BLOCKED) {
            recovery_action_t recovery = fsa_determine_recovery(fsa_monitor.current_state, fsa_monitor.active_interlock);
            fsa_execute_recovery(recovery);
        }
    }
}

void fsa_monitor_tick(void) {
    uint64_t current_time = timer_get_counter();
    uint32_t elapsed_ms = (current_time - fsa_monitor.state_entry_time) / 1000; // Convert to ms

    // Check for timeouts (except terminal states)
    if (fsa_monitor.state_timeout_ms > 0 && elapsed_ms > fsa_monitor.state_timeout_ms) {
        uart_puts("FSA: State timeout detected\n");
        fsa_stats.timeouts++;
        fsa_handle_timeout();
    }

    // Periodic safety checks
    static uint32_t last_check = 0;
    if (elapsed_ms - last_check > 1000) { // Check every second
        fsa_perform_safety_checks();
        last_check = elapsed_ms;
    }
}

void fsa_log_transition(boot_state_t from, boot_state_t to, transition_status_t status) {
    const char* state_names[] = {
        "BOOTCODE_LOADING", "BOOTCODE_EXEC", "STARTELF_LOADING", "STARTELF_EXEC",
        "KERNEL_LOADING", "KERNEL_EXEC", "SUCCESS", "FAILURE"
    };

    const char* status_names[] = {"VALID", "INVALID", "BLOCKED"};

    uart_puts("FSA: ");
    uart_puts(state_names[from]);
    uart_puts(" -> ");
    uart_puts(state_names[to]);
    uart_puts(" [");
    uart_puts(status_names[status]);
    uart_puts("]\n");
}

void fsa_handle_timeout(void) {
    uart_puts("FSA: Handling timeout in state ");
    // Could implement state-specific timeout handling
    fsa_activate_interlock(INTERLOCK_TIMEOUT);
    fsa_update_state(STATE_FAILURE);
}

void fsa_activate_interlock(interlock_type_t type) {
    fsa_monitor.active_interlock = type;
    fsa_stats.interlocks_triggered++;

    const char* interlock_names[] = {
        "NONE", "HARDWARE_NOT_READY", "MEMORY_CORRUPTION",
        "TIMEOUT", "SECURITY_VIOLATION", "RESOURCE_EXHAUSTED"
    };

    uart_puts("FSA: Interlock activated: ");
    uart_puts(interlock_names[type]);
    uart_puts("\n");
}

void fsa_clear_interlock(void) {
    fsa_monitor.active_interlock = INTERLOCK_NONE;
    uart_puts("FSA: Interlock cleared\n");
}

recovery_action_t fsa_determine_recovery(boot_state_t failed_state, interlock_type_t interlock) {
    switch (interlock) {
        case INTERLOCK_TIMEOUT:
            return RECOVERY_RETRY;
        case INTERLOCK_MEMORY_CORRUPTION:
            return RECOVERY_RESET;
        case INTERLOCK_SECURITY_VIOLATION:
            return RECOVERY_FAILSAFE;
        case INTERLOCK_HARDWARE_NOT_READY:
            return RECOVERY_RESET;
        case INTERLOCK_RESOURCE_EXHAUSTED:
            return RECOVERY_HALT;
        default:
            return RECOVERY_NONE;
    }
}

void fsa_execute_recovery(recovery_action_t action) {
    fsa_stats.recoveries_attempted++;

    switch (action) {
        case RECOVERY_RETRY:
            uart_puts("FSA: Executing retry recovery\n");
            fsa_monitor.retry_count++;
            if (fsa_monitor.retry_count < 3) {
                fsa_clear_interlock();
                fsa_stats.recoveries_successful++;
            } else {
                fsa_update_state(STATE_FAILURE);
            }
            break;

        case RECOVERY_RESET:
            uart_puts("FSA: Executing reset recovery\n");
            // Could trigger system reset
            fsa_update_state(STATE_FAILURE);
            break;

        case RECOVERY_FAILSAFE:
            uart_puts("FSA: Executing failsafe recovery\n");
            // Enter minimal safe mode
            fsa_update_state(STATE_FAILURE);
            break;

        case RECOVERY_HALT:
            uart_puts("FSA: Executing halt recovery\n");
            while (1); // Safe halt
            break;

        default:
            break;
    }
}

void fsa_record_history(boot_state_t state, transition_status_t status, interlock_type_t interlock) {
    state_history[history_index].state = state;
    state_history[history_index].timestamp = timer_get_counter();
    state_history[history_index].transition_result = status;
    state_history[history_index].interlock = interlock;

    history_index = (history_index + 1) % STATE_HISTORY_SIZE;
}

void fsa_dump_history(void) {
    uart_puts("FSA History:\n");
    for (int i = 0; i < STATE_HISTORY_SIZE; i++) {
        uint8_t idx = (history_index - 1 - i + STATE_HISTORY_SIZE) % STATE_HISTORY_SIZE;
        if (state_history[idx].timestamp > 0) {
            // Would format timestamp and state info
            uart_puts("  Entry logged\n");
        }
    }
}

state_history_entry_t* fsa_get_history(uint8_t index) {
    if (index >= STATE_HISTORY_SIZE) return NULL;
    uint8_t actual_index = (history_index - 1 - index + STATE_HISTORY_SIZE) % STATE_HISTORY_SIZE;
    return &state_history[actual_index];
}

fsa_statistics_t* fsa_get_statistics(void) {
    return &fsa_stats;
}

void fsa_perform_safety_checks(void) {
    // Hardware checks
    if (!gpio_read(GPIO_LED_PIN)) {  // Basic GPIO check
        fsa_monitor.safety_flags &= ~SAFETY_FLAG_HARDWARE_READY;
    } else {
        fsa_monitor.safety_flags |= SAFETY_FLAG_HARDWARE_READY;
    }

    // Memory checks (simplified)
    // Could check heap integrity, stack usage, etc.
    fsa_monitor.safety_flags |= SAFETY_FLAG_MEMORY_INTEGRITY;

    // Security checks (simplified)
    // Could verify code integrity, check for tampering, etc.
    fsa_monitor.safety_flags |= SAFETY_FLAG_SECURITY_VALID;

    // Resource checks
    // Could monitor CPU usage, memory usage, etc.
    fsa_monitor.safety_flags |= SAFETY_FLAG_RESOURCES_OK;
}