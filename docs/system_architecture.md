# System Architecture Diagrams

## 1. Overall Bootloader Architecture

```mermaid
classDiagram
    class Bootloader {
        +main()
        +initialize_hardware()
        +run_fsa()
    }

    class FSAMonitor {
        +fsa_monitor_init()
        +fsa_update_state()
        +fsa_validate_transition()
        +fsa_check_interlocks()
        +fsa_monitor_tick()
        +fsa_handle_timeout()
        +fsa_execute_recovery()
        -state_monitor_t fsa_monitor
        -state_history_entry_t state_history[16]
        -fsa_statistics_t fsa_stats
    }

    class BSP {
        +initialize_drivers()
        +test_drivers()
    }

    class CoreDrivers {
        +UART: uart_init(), uart_puts()
        +GPIO: gpio_init(), gpio_set_function()
        +Timer: timer_init(), timer_get_counter()
        +Interrupt: interrupt_init(), interrupt_enable()
        +Mailbox: mailbox_init(), mailbox_get_firmware_revision()
        +Clock: clock_init()
        +Memory: memory_init(), memory_alloc()
    }

    class OptionalDrivers {
        +DMA: dma_init(), dma_transfer()
        +I2C: i2c_init(), i2c_write(), i2c_read()
        +SPI: spi_init(), spi_transfer()
        +PWM: pwm_init(), pwm_set_range()
        +USB: usb_init(), usb_reset_controller()
        +Ethernet: ethernet_init(), ethernet_send_frame()
    }

    class Services {
        +SDCard: sd_init(), sd_load_file()
        +Config: config_parse()
        +DTB: dtb_create()
    }

    Bootloader --> FSAMonitor : uses
    Bootloader --> BSP : initializes
    BSP --> CoreDrivers : manages
    BSP --> OptionalDrivers : manages
    Bootloader --> Services : uses
    FSAMonitor --> CoreDrivers : monitors
```

## 2. FSA State Machine

```mermaid
stateDiagram-v2
    [*] --> BootcodeLoading

    BootcodeLoading --> BootcodeExec : SD init success
    BootcodeLoading --> Failure : SD init failed (3 retries)

    BootcodeExec --> StartelfLoading : Bootcode executed

    StartelfLoading --> StartelfExec : Start.elf loaded
    StartelfLoading --> Failure : Load failed (3 retries)

    StartelfExec --> KernelLoading : Hardware initialized

    KernelLoading --> KernelExec : Kernel loaded
    KernelLoading --> Failure : Load failed (3 retries)

    KernelExec --> Success : Kernel handover complete
    KernelExec --> Failure : Handover failed

    Success --> [*] : Boot complete
    Failure --> [*] : Boot failed

    note right of BootcodeLoading : Timeout: 5s
    note right of BootcodeExec : Timeout: 3s
    note right of StartelfLoading : Timeout: 5s
    note right of StartelfExec : Timeout: 10s
    note right of KernelLoading : Timeout: 10s
    note right of KernelExec : Timeout: 5s
```

## 3. FSA Monitor Class Structure

```mermaid
classDiagram
    class FSAMonitor {
        +fsa_monitor_init()
        +fsa_update_state(boot_state_t)
        +fsa_validate_transition(boot_state_t, boot_state_t)
        +fsa_check_interlocks(boot_state_t)
        +fsa_monitor_tick()
        +fsa_log_transition(boot_state_t, boot_state_t, transition_status_t)
        +fsa_handle_timeout()
        +fsa_activate_interlock(interlock_type_t)
        +fsa_clear_interlock()
        +fsa_execute_recovery(recovery_action_t)
        +fsa_record_history(boot_state_t, transition_status_t, interlock_type_t)
        +fsa_dump_history()
        +fsa_get_history(uint8_t)
        +fsa_get_statistics()
        +fsa_perform_safety_checks()
    }

    class StateMonitor {
        +boot_state_t current_state
        +boot_state_t previous_state
        +uint64_t state_entry_time
        +uint32_t state_timeout_ms
        +uint32_t retry_count
        +interlock_type_t active_interlock
        +uint8_t safety_flags
    }

    class StateHistoryEntry {
        +boot_state_t state
        +uint64_t timestamp
        +transition_status_t transition_result
        +interlock_type_t interlock
    }

    class FSAStatistics {
        +uint32_t total_transitions
        +uint32_t valid_transitions
        +uint32_t invalid_transitions
        +uint32_t blocked_transitions
        +uint32_t timeouts
        +uint32_t interlocks_triggered
        +uint32_t recoveries_attempted
        +uint32_t recoveries_successful
    }

    FSAMonitor --> StateMonitor : manages
    FSAMonitor --> StateHistoryEntry : records
    FSAMonitor --> FSAStatistics : tracks
```

## 4. Driver Hierarchy

```mermaid
classDiagram
    class DriverInterface {
        <<interface>>
        +init()
        +deinit()
        +test()
    }

    class UARTDriver {
        +uart_init()
        +uart_puts(char*)
        +uart_getc()
        -uart_regs_t* regs
    }

    class GPIODriver {
        +gpio_init()
        +gpio_set_function(uint8_t, uint8_t)
        +gpio_set(uint8_t)
        +gpio_clear(uint8_t)
        +gpio_read(uint8_t)
        +gpio_enable_interrupt(uint8_t, uint8_t)
        +gpio_disable_interrupt(uint8_t)
        +gpio_clear_interrupt(uint8_t)
    }

    class TimerDriver {
        +timer_init()
        +timer_get_counter()
        +timer_delay_us(uint32_t)
        +timer_delay_ms(uint32_t)
        +timer_arm_init()
        +timer_arm_set_load(uint32_t)
        +timer_arm_enable_interrupt()
        +timer_arm_clear_interrupt()
    }

    class InterruptController {
        +interrupt_init()
        +interrupt_enable(uint32_t)
        +interrupt_disable(uint32_t)
        +interrupt_register_handler(uint32_t, interrupt_handler_t)
        +interrupt_set_priority(uint32_t, uint8_t)
        +interrupt_set_target(uint32_t, uint8_t)
        -interrupt_handler_t interrupt_handlers[256]
    }

    class MailboxDriver {
        +mailbox_init()
        +mailbox_get_firmware_revision()
        +mailbox_send_message(property_message_t*)
        +mailbox_receive_message(property_message_t*)
    }

    class ClockDriver {
        +clock_init()
        +clock_set_frequency(uint32_t, uint32_t)
        +clock_enable(uint32_t)
        +clock_disable(uint32_t)
    }

    class MemoryManager {
        +memory_init()
        +memory_alloc(size_t)
        +memory_free(void*)
        +memory_get_free()
        +memory_get_used()
        -mem_block_t* heap_start
        -size_t heap_size
    }

    DriverInterface <|.. UARTDriver
    DriverInterface <|.. GPIODriver
    DriverInterface <|.. TimerDriver
    DriverInterface <|.. InterruptController
    DriverInterface <|.. MailboxDriver
    DriverInterface <|.. ClockDriver
    DriverInterface <|.. MemoryManager
```

## 5. BSP Component Relationships

```mermaid
classDiagram
    class BSPManager {
        +bsp_init()
        +bsp_test_all()
        +bsp_get_status()
    }

    class DMADriver {
        +dma_init()
        +dma_transfer(uint8_t, void*, void*, uint32_t, uint32_t)
        +dma_transfer_async(uint8_t, dma_control_block_t*)
        +dma_wait_transfer(uint8_t)
        +dma_abort_transfer(uint8_t)
    }

    class I2CDriver {
        +i2c_init(uint8_t)
        +i2c_write(uint8_t, uint8_t, uint8_t*, uint32_t)
        +i2c_read(uint8_t, uint8_t, uint8_t*, uint32_t)
        +i2c_write_reg(uint8_t, uint8_t, uint8_t, uint8_t)
        +i2c_read_reg(uint8_t, uint8_t, uint8_t, uint8_t*)
    }

    class SPIDriver {
        +spi_init(uint8_t)
        +spi_set_mode(uint8_t, uint8_t)
        +spi_set_clock(uint8_t, uint32_t)
        +spi_transfer(uint8_t, uint8_t, uint8_t*, uint8_t*, uint32_t)
        +spi_write(uint8_t, uint8_t, uint8_t*, uint32_t)
        +spi_read(uint8_t, uint8_t, uint8_t*, uint32_t)
    }

    class PWMDriver {
        +pwm_init()
        +pwm_set_mode(uint8_t, uint8_t)
        +pwm_set_range(uint8_t, uint32_t)
        +pwm_set_data(uint8_t, uint32_t)
        +pwm_enable(uint8_t)
        +pwm_disable(uint8_t)
        +pwm_set_polarity(uint8_t, uint8_t)
    }

    class USBDriver {
        +usb_init()
        +usb_reset_controller()
        +usb_start_controller()
        +usb_stop_controller()
        +usb_get_port_status(uint8_t)
        +usb_reset_port(uint8_t)
        +usb_enable_port(uint8_t)
    }

    class EthernetDriver {
        +ethernet_init()
        +ethernet_set_mac_address(uint8_t*)
        +ethernet_send_frame(ethernet_frame_t*, uint16_t)
        +ethernet_receive_frame(ethernet_frame_t*, uint16_t*)
        +ethernet_get_status()
        +ethernet_enable()
        +ethernet_disable()
    }

    BSPManager --> DMADriver : manages
    BSPManager --> I2CDriver : manages
    BSPManager --> SPIDriver : manages
    BSPManager --> PWMDriver : manages
    BSPManager --> USBDriver : manages
    BSPManager --> EthernetDriver : manages
```

## 6. Safety Interlocks System

```mermaid
classDiagram
    class SafetyInterlock {
        +INTERLOCK_NONE
        +INTERLOCK_HARDWARE_NOT_READY
        +INTERLOCK_MEMORY_CORRUPTION
        +INTERLOCK_TIMEOUT
        +INTERLOCK_SECURITY_VIOLATION
        +INTERLOCK_RESOURCE_EXHAUSTED
    }

    class InterlockManager {
        +fsa_check_interlocks(boot_state_t)
        +fsa_activate_interlock(interlock_type_t)
        +fsa_clear_interlock()
        +fsa_determine_recovery(boot_state_t, interlock_type_t)
        +fsa_execute_recovery(recovery_action_t)
    }

    class RecoveryActions {
        +RECOVERY_NONE
        +RECOVERY_RETRY
        +RECOVERY_RESET
        +RECOVERY_FAILSAFE
        +RECOVERY_HALT
    }

    class SafetyChecks {
        +check_hardware_readiness()
        +check_memory_integrity()
        +check_security_validation()
        +check_resource_availability()
    }

    InterlockManager --> SafetyInterlock : uses
    InterlockManager --> RecoveryActions : executes
    InterlockManager --> SafetyChecks : performs
```

## 7. Data Flow Architecture

```mermaid
flowchart TD
    A[Power On] --> B[Hardware Init]
    B --> C[FSA Monitor Init]
    C --> D[Driver Initialization]
    D --> E[BSP Test Suite]
    E --> F{FSA State Machine}

    F --> G[Bootcode Loading]
    G --> H{Valid Transition?}
    H -->|Yes| I[Bootcode Exec]
    H -->|No| J[Interlock Activated]

    I --> K[Start.elf Loading]
    K --> L[Start.elf Exec]
    L --> M[Kernel Loading]
    M --> N[Kernel Exec]
    N --> O[Success]

    J --> P[Recovery Action]
    P --> Q{Recovery Success?}
    Q -->|Yes| F
    Q -->|No| R[Failure]

    O --> S[System Ready]
    R --> T[Error Logging]
    T --> U[Safe Halt]
```

## 8. Component Interaction Sequence

```mermaid
sequenceDiagram
    participant Main as Main Bootloader
    participant FSA as FSA Monitor
    participant HW as Hardware Drivers
    participant BSP as BSP Components
    participant SVC as System Services

    Main->>FSA: fsa_monitor_init()
    FSA-->>Main: Monitor initialized

    Main->>HW: Initialize core drivers
    HW-->>Main: Drivers ready

    Main->>BSP: Initialize BSP components
    BSP-->>Main: BSP ready

    Main->>FSA: fsa_update_state(BOOTCODE_LOADING)
    FSA->>FSA: Validate transition
    FSA->>FSA: Check interlocks
    FSA-->>Main: State updated

    loop FSA State Machine
        Main->>FSA: fsa_monitor_tick()
        FSA->>FSA: Check timeouts
        FSA->>FSA: Perform safety checks

        Main->>SVC: Execute state logic
        SVC-->>Main: State result

        alt Valid transition
            Main->>FSA: fsa_update_state(new_state)
            FSA-->>Main: State changed
        else Invalid transition
            FSA->>FSA: Activate interlock
            FSA->>FSA: Execute recovery
        end
    end

    Main->>FSA: fsa_dump_history()
    FSA-->>Main: History logged
```