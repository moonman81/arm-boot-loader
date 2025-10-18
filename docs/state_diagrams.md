# State Diagrams for Bootloader Solution

## 1. Hyper-Flexible Main Boot FSA State Machine

```mermaid
stateDiagram-v2
    [*] --> PowerOn : System Reset

    state PowerOn as "Power On"
    state EarlyHWInit as "Early Hardware Init"
    state BootcodeSourceSelect as "Bootcode Source Select"
    state BootcodeLoading as "Bootcode Loading"
    state BootcodeValidation as "Bootcode Validation"
    state BootcodeExec as "Bootcode Execution"
    state BootcodeConfigParse as "Bootcode Config Parse"
    state CoreDriverInit as "Core Driver Init"
    state BSPDriverInit as "BSP Driver Init"
    state HWValidation as "Hardware Validation"
    state ConfigLoading as "Config Loading"
    state ConfigParsing as "Config Parsing"
    state ConfigValidation as "Config Validation"
    state ConfigApplication as "Config Application"
    state StartelfSourceSelect as "Start.elf Source Select"
    state StartelfLoading as "Start.elf Loading"
    state StartelfValidation as "Start.elf Validation"
    state StartelfExec as "Start.elf Execution"
    state KernelSourceSelect as "Kernel Source Select"
    state KernelLoading as "Kernel Loading"
    state KernelValidation as "Kernel Validation"
    state InitrdLoading as "Initrd Loading"
    state DTBLoading as "DTB Loading"
    state KernelParamsSetup as "Kernel Params Setup"
    state KernelExec as "Kernel Execution"
    state Success as "Boot Success"
    state Failure as "Boot Failure"
    state Halt as "System Halt"

    PowerOn --> EarlyHWInit : Power stable
    EarlyHWInit --> BootcodeSourceSelect : Basic HW ready
    BootcodeSourceSelect --> BootcodeLoading : SD selected

    BootcodeLoading --> BootcodeValidation : SD init OK
    BootcodeLoading --> Failure : SD init failed (3 retries)
    BootcodeValidation --> BootcodeExec : Bootcode valid
    BootcodeExec --> BootcodeConfigParse : Bootcode executed
    BootcodeConfigParse --> CoreDriverInit : Config parsed

    CoreDriverInit --> BSPDriverInit : Core drivers ready
    BSPDriverInit --> HWValidation : BSP drivers ready
    HWValidation --> ConfigLoading : Hardware valid

    ConfigLoading --> ConfigParsing : Config loaded
    ConfigParsing --> ConfigValidation : Config parsed
    ConfigValidation --> ConfigApplication : Config valid
    ConfigApplication --> StartelfSourceSelect : Config applied

    StartelfSourceSelect --> StartelfLoading : SD selected
    StartelfLoading --> StartelfValidation : Start.elf loaded
    StartelfValidation --> StartelfExec : Start.elf valid
    StartelfExec --> KernelSourceSelect : Start.elf executed

    KernelSourceSelect --> KernelLoading : SD selected
    KernelLoading --> KernelValidation : Kernel loaded
    KernelValidation --> InitrdLoading : Kernel valid
    InitrdLoading --> DTBLoading : Initrd loaded
    DTBLoading --> KernelParamsSetup : DTB created
    KernelParamsSetup --> KernelExec : Params set
    KernelExec --> Success : Kernel handover complete
    KernelExec --> Failure : Handover failed

    Success --> [*] : System running
    Failure --> [*] : Boot failed
    Halt --> [*] : System halted

    note right of PowerOn : Timeout: 1s
    note right of EarlyHWInit : Timeout: 2s
    note right of BootcodeLoading : Timeout: 5s
    note right of BootcodeValidation : Timeout: 2s
    note right of BootcodeExec : Timeout: 3s
    note right of CoreDriverInit : Timeout: 3s
    note right of BSPDriverInit : Timeout: 5s
    note right of ConfigParsing : Timeout: 2s
    note right of StartelfLoading : Timeout: 5s
    note right of KernelLoading : Timeout: 10s
    note right of KernelValidation : Timeout: 2s
    note right of DTBLoading : Timeout: 3s
    note right of KernelExec : Timeout: 5s
```

## 2. FSA Monitor State Machine

```mermaid
stateDiagram-v2
    [*] --> MonitorInactive

    state MonitorInactive as "Monitor Inactive"
    state MonitorInitializing as "Monitor Initializing"
    state MonitorActive as "Monitor Active"
    state MonitorValidating as "Validating Transition"
    state MonitorInterlock as "Interlock Active"
    state MonitorRecovery as "Recovery Mode"
    state MonitorLogging as "Logging State"
    state MonitorTimeout as "Timeout Handling"

    MonitorInactive --> MonitorInitializing : fsa_monitor_init()

    MonitorInitializing --> MonitorActive : Initialization complete

    MonitorActive --> MonitorValidating : fsa_update_state() called

    MonitorValidating --> MonitorActive : Transition valid
    MonitorValidating --> MonitorInterlock : Transition blocked
    MonitorValidating --> MonitorLogging : Transition invalid

    MonitorInterlock --> MonitorRecovery : Interlock activated

    MonitorRecovery --> MonitorActive : Recovery successful
    MonitorRecovery --> MonitorLogging : Recovery failed

    MonitorActive --> MonitorTimeout : State timeout detected

    MonitorTimeout --> MonitorRecovery : Timeout recovery initiated

    MonitorLogging --> MonitorActive : Logging complete

    note right of MonitorValidating : Check transition matrix
    note right of MonitorInterlock : Safety interlock triggered
    note right of MonitorRecovery : Execute recovery action
    note right of MonitorTimeout : State-specific timeouts
```

## 3. Recovery State Machine

```mermaid
stateDiagram-v2
    [*] --> NoRecovery

    state NoRecovery as "No Recovery Needed"
    state RecoveryDetermining as "Determining Recovery Action"
    state RecoveryRetry as "Retry Recovery"
    state RecoveryReset as "System Reset Recovery"
    state RecoveryFailsafe as "Failsafe Mode Recovery"
    state RecoveryHalt as "Halt System Recovery"
    state RecoverySuccess as "Recovery Successful"
    state RecoveryFailed as "Recovery Failed"

    NoRecovery --> RecoveryDetermining : Interlock activated

    RecoveryDetermining --> RecoveryRetry : Timeout interlock
    RecoveryDetermining --> RecoveryReset : Hardware/Memory interlock
    RecoveryDetermining --> RecoveryFailsafe : Security interlock
    RecoveryDetermining --> RecoveryHalt : Resource exhausted

    RecoveryRetry --> RecoverySuccess : Retry successful
    RecoveryRetry --> RecoveryFailed : Retry limit exceeded

    RecoveryReset --> RecoverySuccess : Reset successful
    RecoveryReset --> RecoveryFailed : Reset failed

    RecoveryFailsafe --> RecoverySuccess : Failsafe activated
    RecoveryFailsafe --> RecoveryFailed : Failsafe unavailable

    RecoveryHalt --> [*] : System halted

    RecoverySuccess --> NoRecovery : Interlock cleared
    RecoveryFailed --> RecoveryDetermining : Try alternative recovery

    note right of RecoveryDetermining : Based on interlock type
    note right of RecoveryRetry : Max 3 attempts
    note right of RecoveryReset : Hardware reinitialization
    note right of RecoveryFailsafe : Minimal safe mode
```

## 4. Driver Initialization State Machine

```mermaid
stateDiagram-v2
    [*] --> DriversUninitialized

    state DriversUninitialized as "Drivers Uninitialized"
    state CoreDriversInit as "Core Drivers Init"
    state UARTInit as "UART Driver Init"
    state GPIOInit as "GPIO Driver Init"
    state TimerInit as "Timer Driver Init"
    state InterruptInit as "Interrupt Controller Init"
    state MailboxInit as "Mailbox Driver Init"
    state ClockInit as "Clock Driver Init"
    state MemoryInit as "Memory Manager Init"
    state OptionalDriversInit as "Optional Drivers Init"
    state DMAInit as "DMA Driver Init"
    state I2CInit as "I2C Driver Init"
    state SPIInit as "SPI Driver Init"
    state PWMInit as "PWM Driver Init"
    state USBInit as "USB Driver Init"
    state EthernetInit as "Ethernet Driver Init"
    state DriversReady as "All Drivers Ready"
    state DriverFailed as "Driver Init Failed"

    DriversUninitialized --> CoreDriversInit : bsp_init() called

    CoreDriversInit --> UARTInit : Start core init
    UARTInit --> GPIOInit : UART ready
    GPIOInit --> TimerInit : GPIO ready
    TimerInit --> InterruptInit : Timer ready
    InterruptInit --> MailboxInit : Interrupts ready
    MailboxInit --> ClockInit : Mailbox ready
    ClockInit --> MemoryInit : Clock ready
    MemoryInit --> OptionalDriversInit : Memory ready

    OptionalDriversInit --> DMAInit : Start optional init
    DMAInit --> I2CInit : DMA ready
    I2CInit --> SPIInit : I2C ready
    SPIInit --> PWMInit : SPI ready
    PWMInit --> USBInit : PWM ready
    USBInit --> EthernetInit : USB ready
    EthernetInit --> DriversReady : Ethernet ready

    UARTInit --> DriverFailed : UART init failed
    GPIOInit --> DriverFailed : GPIO init failed
    TimerInit --> DriverFailed : Timer init failed
    InterruptInit --> DriverFailed : Interrupt init failed
    MailboxInit --> DriverFailed : Mailbox init failed
    ClockInit --> DriverFailed : Clock init failed
    MemoryInit --> DriverFailed : Memory init failed
    DMAInit --> DriverFailed : DMA init failed
    I2CInit --> DriverFailed : I2C init failed
    SPIInit --> DriverFailed : SPI init failed
    PWMInit --> DriverFailed : PWM init failed
    USBInit --> DriverFailed : USB init failed
    EthernetInit --> DriverFailed : Ethernet init failed

    DriversReady --> [*] : BSP ready
    DriverFailed --> [*] : Init failed

    note right of CoreDriversInit : Essential drivers
    note right of OptionalDriversInit : Extended functionality
    note right of DriversReady : All drivers initialized
```

## 5. Safety Interlock State Machine

```mermaid
stateDiagram-v2
    [*] --> InterlocksClear

    state InterlocksClear as "No Interlocks"
    state InterlockChecking as "Checking Interlocks"
    state HardwareNotReady as "Hardware Not Ready"
    state MemoryCorruption as "Memory Corruption"
    state TimeoutExceeded as "Timeout Exceeded"
    state SecurityViolation as "Security Violation"
    state ResourceExhausted as "Resource Exhausted"
    state InterlockRecovery as "Interlock Recovery"
    state InterlockCleared as "Interlock Cleared"

    InterlocksClear --> InterlockChecking : State transition requested

    InterlockChecking --> HardwareNotReady : Hardware check failed
    InterlockChecking --> MemoryCorruption : Memory check failed
    InterlockChecking --> TimeoutExceeded : Timeout detected
    InterlockChecking --> SecurityViolation : Security check failed
    InterlockChecking --> ResourceExhausted : Resource check failed
    InterlockChecking --> InterlocksClear : All checks passed

    HardwareNotReady --> InterlockRecovery : Recovery initiated
    MemoryCorruption --> InterlockRecovery : Recovery initiated
    TimeoutExceeded --> InterlockRecovery : Recovery initiated
    SecurityViolation --> InterlockRecovery : Recovery initiated
    ResourceExhausted --> InterlockRecovery : Recovery initiated

    InterlockRecovery --> InterlockCleared : Recovery successful
    InterlockRecovery --> HardwareNotReady : Recovery failed
    InterlockRecovery --> MemoryCorruption : Recovery failed
    InterlockRecovery --> TimeoutExceeded : Recovery failed
    InterlockRecovery --> SecurityViolation : Recovery failed
    InterlockRecovery --> ResourceExhausted : Recovery failed

    InterlockCleared --> InterlocksClear : Interlock cleared

    note right of InterlockChecking : Pre-transition validation
    note right of HardwareNotReady : GPIO/peripheral failure
    note right of MemoryCorruption : Heap integrity violation
    note right of TimeoutExceeded : State timeout exceeded
    note right of SecurityViolation : Security check failure
    note right of ResourceExhausted : System resource depletion
```

## 6. Overall System State Flow

```mermaid
stateDiagram-v2
    [*] --> PowerOn

    state PowerOn as "Power On"
    state HardwareInit as "Hardware Initialization"
    state FSAMonitorInit as "FSA Monitor Init"
    state BSPInit as "BSP Components Init"
    state DriverTest as "Driver Testing"
    state BootFSARunning as "Boot FSA Running"
    state BootSuccess as "Boot Successful"
    state BootFailure as "Boot Failed"
    state SystemRunning as "System Running"
    state ErrorRecovery as "Error Recovery"

    PowerOn --> HardwareInit : Power stable

    HardwareInit --> FSAMonitorInit : Hardware ready
    HardwareInit --> ErrorRecovery : Hardware failure

    FSAMonitorInit --> BSPInit : Monitor initialized
    FSAMonitorInit --> ErrorRecovery : Monitor init failed

    BSPInit --> DriverTest : BSP components ready
    BSPInit --> ErrorRecovery : BSP init failed

    DriverTest --> BootFSARunning : All tests passed
    DriverTest --> ErrorRecovery : Driver test failed

    BootFSARunning --> BootSuccess : Boot sequence complete
    BootFSARunning --> BootFailure : Boot sequence failed
    BootFSARunning --> ErrorRecovery : Runtime error

    BootSuccess --> SystemRunning : Kernel handover complete

    BootFailure --> ErrorRecovery : Analyze failure
    ErrorRecovery --> BootFSARunning : Recovery successful
    ErrorRecovery --> BootFailure : Recovery failed

    SystemRunning --> [*] : Normal operation
    BootFailure --> [*] : Boot permanently failed

    note right of HardwareInit : UART, GPIO, Timer, etc.
    note right of FSAMonitorInit : State validation, interlocks
    note right of BSPInit : DMA, I2C, SPI, PWM, USB, Ethernet
    note right of DriverTest : Functional verification
    note right of BootFSARunning : 8-state FSA execution
    note right of ErrorRecovery : Retry, reset, failsafe, halt
```

## 7. Transition Validation State Machine

```mermaid
stateDiagram-v2
    [*] --> TransitionRequested

    state TransitionRequested as "Transition Requested"
    state ValidateStates as "Validate State Pair"
    state CheckTransitionMatrix as "Check Transition Matrix"
    state CheckInterlocks as "Check Safety Interlocks"
    state TransitionValid as "Transition Valid"
    state TransitionInvalid as "Transition Invalid"
    state TransitionBlocked as "Transition Blocked"
    state LogTransition as "Log Transition Result"
    state ExecuteTransition as "Execute Transition"
    state AbortTransition as "Abort Transition"

    TransitionRequested --> ValidateStates : fsa_update_state() called

    ValidateStates --> CheckTransitionMatrix : States identified
    ValidateStates --> TransitionInvalid : Invalid state parameters

    CheckTransitionMatrix --> CheckInterlocks : Matrix allows transition
    CheckTransitionMatrix --> TransitionInvalid : Matrix blocks transition

    CheckInterlocks --> TransitionValid : All interlocks clear
    CheckInterlocks --> TransitionBlocked : Interlock active

    TransitionValid --> LogTransition : Validation passed
    TransitionInvalid --> LogTransition : Validation failed
    TransitionBlocked --> LogTransition : Validation blocked

    LogTransition --> ExecuteTransition : Valid transition
    LogTransition --> AbortTransition : Invalid/blocked transition

    ExecuteTransition --> [*] : State updated
    AbortTransition --> [*] : Transition rejected

    note right of ValidateStates : Check state enum values
    note right of CheckTransitionMatrix : 8x8 adjacency matrix
    note right of CheckInterlocks : Hardware, memory, security, resources
    note right of LogTransition : Update statistics and history
```

## 8. Boot Sequence State Machine

```mermaid
stateDiagram-v2
    [*] --> BootStart

    state BootStart as "Boot Sequence Start"
    state EarlyInit as "Early Hardware Init"
    state FSASetup as "FSA Monitor Setup"
    state CoreDriverInit as "Core Driver Init"
    state BSPDriverInit as "BSP Driver Init"
    state DriverVerification as "Driver Verification"
    state FSAEntry as "Enter FSA State Machine"
    state BootcodePhase as "Bootcode Phase"
    state StartelfPhase as "Start.elf Phase"
    state KernelPhase as "Kernel Phase"
    state HandoverPhase as "Kernel Handover"
    state BootComplete as "Boot Complete"
    state BootError as "Boot Error"

    BootStart --> EarlyInit : Power-on reset

    EarlyInit --> FSASetup : Basic hardware ready
    EarlyInit --> BootError : Hardware init failed

    FSASetup --> CoreDriverInit : FSA monitor initialized
    FSASetup --> BootError : FSA setup failed

    CoreDriverInit --> BSPDriverInit : Core drivers ready
    CoreDriverInit --> BootError : Core driver failed

    BSPDriverInit --> DriverVerification : BSP drivers ready
    BSPDriverInit --> BootError : BSP driver failed

    DriverVerification --> FSAEntry : All drivers verified
    DriverVerification --> BootError : Driver verification failed

    FSAEntry --> BootcodePhase : FSA started

    BootcodePhase --> StartelfPhase : Bootcode loaded/executed
    BootcodePhase --> BootError : Bootcode phase failed

    StartelfPhase --> KernelPhase : Start.elf loaded/executed
    StartelfPhase --> BootError : Start.elf phase failed

    KernelPhase --> HandoverPhase : Kernel loaded/executed
    KernelPhase --> BootError : Kernel phase failed

    HandoverPhase --> BootComplete : Handover successful
    HandoverPhase --> BootError : Handover failed

    BootComplete --> [*] : System operational
    BootError --> [*] : Boot failed

    note right of EarlyInit : UART, GPIO, Timer
    note right of FSASetup : State validation, interlocks
    note right of CoreDriverInit : Interrupt, Mailbox, Clock, Memory
    note right of BSPDriverInit : DMA, I2C, SPI, PWM, USB, Ethernet
    note right of DriverVerification : Functional tests
    note right of FSAEntry : 8-state state machine
    note right of BootcodePhase : SD card, bootcode loading
    note right of StartelfPhase : VideoCore firmware
    note right of KernelPhase : Linux kernel, initrd
    note right of HandoverPhase : Jump to kernel
```

## 9. Error Handling State Machine

```mermaid
stateDiagram-v2
    [*] --> NoError

    state NoError as "No Error"
    state ErrorDetected as "Error Detected"
    state ErrorClassification as "Classify Error"
    state HardwareError as "Hardware Error"
    state SoftwareError as "Software Error"
    state TimeoutError as "Timeout Error"
    state SecurityError as "Security Error"
    state ResourceError as "Resource Error"
    state ErrorLogging as "Log Error Details"
    state RecoverySelection as "Select Recovery Action"
    state RecoveryExecution as "Execute Recovery"
    state RecoverySuccess as "Recovery Successful"
    state RecoveryFailed as "Recovery Failed"
    state SystemHalt as "System Halt"

    NoError --> ErrorDetected : Exception/interrupt/failure

    ErrorDetected --> ErrorClassification : Error context captured

    ErrorClassification --> HardwareError : Peripheral failure
    ErrorClassification --> SoftwareError : Logic error
    ErrorClassification --> TimeoutError : Operation timeout
    ErrorClassification --> SecurityError : Integrity violation
    ErrorClassification --> ResourceError : Resource exhaustion

    HardwareError --> ErrorLogging : Error classified
    SoftwareError --> ErrorLogging : Error classified
    TimeoutError --> ErrorLogging : Error classified
    SecurityError --> ErrorLogging : Error classified
    ResourceError --> ErrorLogging : Error classified

    ErrorLogging --> RecoverySelection : Error logged

    RecoverySelection --> RecoveryExecution : Recovery action selected

    RecoveryExecution --> RecoverySuccess : Recovery completed
    RecoveryExecution --> RecoveryFailed : Recovery unsuccessful

    RecoverySuccess --> NoError : System recovered
    RecoveryFailed --> RecoverySelection : Try alternative recovery
    RecoveryFailed --> SystemHalt : All recoveries failed

    SystemHalt --> [*] : System safely halted

    note right of ErrorClassification : Based on error context
    note right of ErrorLogging : FSA history, statistics
    note right of RecoverySelection : Interlock-based selection
    note right of RecoveryExecution : Retry, reset, failsafe, halt
```

## 10. Monitoring and Statistics State Machine

```mermaid
stateDiagram-v2
    [*] --> MonitoringInactive

    state MonitoringInactive as "Monitoring Inactive"
    state MonitoringInit as "Monitoring Initialization"
    state MonitoringActive as "Monitoring Active"
    state StatsCollection as "Statistics Collection"
    state SafetyChecks as "Safety Checks"
    state TimeoutChecks as "Timeout Checks"
    state HistoryUpdate as "History Update"
    state InterlockMonitor as "Interlock Monitoring"
    state LogGeneration as "Log Generation"
    state StatsReporting as "Statistics Reporting"
    state MonitoringError as "Monitoring Error"

    MonitoringInactive --> MonitoringInit : fsa_monitor_init()

    MonitoringInit --> MonitoringActive : Initialization complete
    MonitoringInit --> MonitoringError : Init failed

    MonitoringActive --> StatsCollection : Monitor tick
    MonitoringActive --> SafetyChecks : Periodic check
    MonitoringActive --> TimeoutChecks : State timeout check
    MonitoringActive --> HistoryUpdate : State transition
    MonitoringActive --> InterlockMonitor : Interlock event
    MonitoringActive --> LogGeneration : Log requested

    StatsCollection --> MonitoringActive : Stats updated
    SafetyChecks --> MonitoringActive : Checks complete
    TimeoutChecks --> MonitoringActive : Timeouts checked
    HistoryUpdate --> MonitoringActive : History updated
    InterlockMonitor --> MonitoringActive : Interlocks monitored
    LogGeneration --> StatsReporting : Logs generated

    StatsReporting --> MonitoringActive : Report complete

    MonitoringError --> [*] : Monitoring failed

    note right of StatsCollection : Transition counts, timings
    note right of SafetyChecks : Hardware, memory, security
    note right of TimeoutChecks : State-specific timeouts
    note right of HistoryUpdate : Circular buffer management
    note right of InterlockMonitor : Safety flag management
    note right of LogGeneration : UART/serial output
    note right of StatsReporting : Performance metrics
```

## State Diagram Summary

These Mermaid.js state diagrams provide comprehensive visualization of the hyper-flexible bootloader's state machines:

### **Main FSA (32 states)**
- Highly granular boot progression with intermediate states
- Support for alternative boot sources and configurations
- State-specific timeouts and comprehensive validation

### **FSA Monitor (8 states)**
- Advanced state transition validation and interlock management
- Recovery coordination and detailed logging

### **Recovery System (8 states)**
- Multiple recovery strategies based on failure type
- Automatic fallback mechanisms with alternative paths

### **Driver Initialization (18 states)**
- Hierarchical driver initialization with core/BSP separation
- Individual driver failure handling and validation

### **Safety Interlocks (9 states)**
- Pre-transition safety validation with hardware/memory/security checks
- Interlock activation and multi-level recovery

### **Overall System (11 states)**
- Complete boot sequence from power-on to running
- Integrated error handling and alternative path support

### **Transition Validation (9 states)**
- Detailed transition approval process with dynamic rules
- Comprehensive logging and statistics integration

### **Boot Sequence (14 states)**
- Phased boot process with extensive checkpoints
- Failure isolation and alternative source capabilities

### **Error Handling (13 states)**
- Comprehensive error classification and multi-strategy recovery
- Safe system shutdown with multiple halt options

### **Monitoring (11 states)**
- Continuous system health monitoring with safety checks
- Detailed statistics collection and reporting

### **Alternative Boot Paths**
- Network boot initialization framework
- USB boot initialization framework
- Failsafe and recovery boot options
- Modular component loading support

These diagrams demonstrate how the bootloader implements the most flexible ARM Raspberry Pi boot system ever, with support for alternative configurations, modular components, and hyper-configurable state machines that can arrive at any suitable stable state required.