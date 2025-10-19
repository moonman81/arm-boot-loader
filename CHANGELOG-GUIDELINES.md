# Changelog & Commit Message Guidelines
## Factual Disclosure Over Sales Language

**Purpose**: Maintain honest, objective record of changes without promotional hype.

---

## Philosophy

### What We Avoid
❌ Marketing language
❌ Exaggerated claims
❌ Aspirational statements presented as facts
❌ Hiding failures or limitations
❌ Cherry-picking positive results

### What We Embrace
✅ Factual descriptions
✅ Objective metrics
✅ Honest status reporting
✅ Clear disclosure of failures
✅ Balanced presentation

---

## Commit Message Rules

### Template

```
[TYPE] Brief factual description

What changed:
- Concrete change 1
- Concrete change 2

Status:
- What works: [factual assessment]
- What doesn't: [honest disclosure]
- What's untested: [known gaps]

Evidence:
- [How verified, or mark as unverified]
```

### Types
- `[FIX]` - Repair of broken functionality
- `[ADD]` - New code/feature (not "implemented" until tested)
- `[TEST]` - Testing or validation work
- `[DOC]` - Documentation changes
- `[REFACTOR]` - Code reorganization
- `[BUILD]` - Build system changes
- `[REVERT]` - Reverting previous change

---

## Examples: Bad vs. Good

### Example 1: New Feature

❌ **BAD (Sales Language)**:
```
Implemented comprehensive SD card driver with FAT32 support!
Full filesystem access now available across all platforms.
Production-ready storage subsystem.
```

✅ **GOOD (Factual)**:
```
[ADD] SD card driver code and FAT32 parser

What changed:
- Added sd.c/h with EMMC initialization code
- Added fat.c/h with FAT32 parsing functions
- Added file reading functions

Status:
- What works: Code compiles
- What doesn't: EMMC init fails in QEMU
- What's untested: Real hardware, multi-card types

Evidence:
- Compilation: VERIFIED
- QEMU test: FAILS (no SD card detected)
- Hardware test: UNTESTED
```

### Example 2: Bug Fix

❌ **BAD**:
```
Fixed critical UART bug! Serial communication now rock-solid
and reliable. Full production deployment ready.
```

✅ **GOOD**:
```
[FIX] UART baud rate calculation for BCM2711

What changed:
- Corrected clock divisor formula in uart_init()
- Changed from 250MHz to 500MHz base clock for Pi 4

Status:
- What works: QEMU output improved
- What doesn't: Not tested on real Pi 4
- What's untested: Baud rates other than 115200

Evidence:
- QEMU: Characters now display correctly
- Hardware: UNTESTED
- Other rates: UNTESTED
```

### Example 3: Testing

❌ **BAD**:
```
Comprehensive testing validates bootloader excellence!
All subsystems thoroughly verified. Quality assured.
```

✅ **GOOD**:
```
[TEST] QEMU virt machine boot validation

What changed:
- Ran bootloader on QEMU virt (-M virt)
- Captured UART output
- Verified subsystem initialization

Status:
- What works: UART, Timer, Memory in QEMU virt
- What doesn't: SD card, GPIO (no QEMU support)
- What's untested: Real hardware, other QEMU machines

Evidence:
- QEMU virt boot: SUCCESS (see qemu_output.log)
- QEMU raspi3b: NOT ATTEMPTED
- Real hardware: NOT ATTEMPTED

Test coverage: ~20% of codebase (UART, Timer, Memory only)
```

### Example 4: Documentation

❌ **BAD**:
```
Created world-class documentation suite! Complete guides
covering every aspect. Professional-grade technical writing.
```

✅ **GOOD**:
```
[DOC] Added 28 markdown documentation files

What changed:
- Generated docs/ directory with 28 .md files
- Covers architecture, build, testing, APIs
- AI-generated based on codebase analysis

Status:
- What works: Files exist, build instructions accurate
- What doesn't: Some claims not verified (test coverage, platform support)
- What's untested: Accuracy against real hardware behavior

Evidence:
- Build instructions: VERIFIED (QEMU build works)
- Hardware instructions: UNTESTED
- API accuracy: NOT VALIDATED
- Status claims: SOME OPTIMISTIC (see SCORECARD.md)

Note: Documentation should be treated as preliminary
until validated against actual implementation behavior.
```

### Example 5: Failure

❌ **BAD** (Hiding):
```
Improved boot sequence initialization.
```

✅ **GOOD** (Honest):
```
[TEST] Pi 4B hardware boot test - FAILED

What changed:
- Copied bootloader.bin to SD card
- Attempted boot on Raspberry Pi 4B
- Connected UART to monitor serial output

Status:
- What works: NOTHING on real hardware
- What doesn't: No UART output, no boot progression
- What's untested: Whether CPU even starts

Evidence:
- UART output: NONE
- LED activity: NONE
- Power draw: Normal (board powered)

Root cause hypothesis:
1. Missing GPU firmware (bootcode.bin, start4.elf)
2. Wrong file naming (needs kernel8.img)
3. Missing config.txt
4. Possible memory layout issues

Next steps:
- Download official Raspberry Pi firmware
- Rename bootloader.bin to kernel8.img
- Create config.txt with proper settings
- Retest
```

---

## Changelog Format

### Template

```markdown
## [Version/Date] - YYYY-MM-DD

### Added
- [Factual description] - [Status: tested/untested]

### Changed
- [Factual description] - [Status: improved/broken/unknown]

### Fixed
- [Factual description] - [Verified on: platform/untested]

### Removed
- [Factual description] - [Reason]

### Known Issues
- [Honest disclosure of what doesn't work]

### Test Results
- [Platform]: [Pass/Fail] - [Evidence]

### Metrics
- [Objective measurements]
```

### Example Changelog Entry

```markdown
## 2025-10-19 - Multi-Dimensional Verification Documentation

### Added
- MULTIDIMENSIONAL_FORMAL_VERIFICATION.md (913 lines) - Status: DOCUMENTED, NOT EXECUTED
- SCORECARD.md for objective status tracking - Status: ACTIVE
- Tarski model theory framework - Status: THEORETICAL
- Grothendieck category theory framework - Status: THEORETICAL
- Kripke modal logic framework - Status: THEORETICAL
- Gödel completeness analysis - Status: THEORETICAL
- Scott domain theory framework - Status: THEORETICAL

### Changed
- README.md claims now qualified in SCORECARD.md - Status: BALANCED
- Documentation now acknowledges gaps - Status: IMPROVED

### Known Issues
- Physical hardware boot still failing (Pi 4B)
- SD card driver non-functional in QEMU
- 0/14 drivers verified on real hardware
- Formal verification documented but not executed
- Test coverage claims unverified

### Test Results
- QEMU virt: PASS - Boots successfully, UART output correct
- QEMU raspi3b: PARTIAL - Loads but CPU doesn't start (expected)
- Pi 4B hardware: FAIL - No boot, no UART output
- Formal proofs: NOT RUN - Documentation only

### Metrics
- Documentation: 28 files → 30 files (+2)
- Binary size: 5.4 KB (unchanged)
- Lines of doc: +913 (verification) +[scorecard size]
- Hardware success rate: 0/1 (0%)
- QEMU success rate: 1/1 (100%)
```

---

## PR (Pull Request) Guidelines

### PR Title Format
```
[TYPE] Factual description - [STATUS]
```

Examples:
- `[FIX] UART baud rate for Pi 4 - UNTESTED ON HARDWARE`
- `[ADD] I2C driver implementation - COMPILES, UNTESTED`
- `[TEST] QEMU boot validation - PASSED`

### PR Description Template

```markdown
## Summary
[One sentence factual description]

## Changes Made
- [Concrete change 1]
- [Concrete change 2]

## Testing Performed
- [ ] Compiles cleanly
- [ ] Tested in QEMU: [PASS/FAIL/NOT RUN]
- [ ] Tested on hardware: [PASS/FAIL/NOT RUN]
- [ ] Unit tests: [PASS/FAIL/NOT RUN]
- [ ] Static analysis: [PASS/FAIL/NOT RUN]

## Test Results
**QEMU**:
- [Result with evidence]

**Hardware** (specify model):
- [Result with evidence or "NOT TESTED"]

## Known Limitations
- [Honest disclosure 1]
- [Honest disclosure 2]

## Status Before/After
| Metric | Before | After | Change |
|--------|--------|-------|--------|
| [Metric 1] | [Value] | [Value] | [±X] |

## Breaking Changes
- [Yes/No with details]

## Reviewer Checklist
- [ ] Claims verified
- [ ] Test evidence provided
- [ ] Limitations disclosed
- [ ] No exaggerated language
```

---

## Language Guidelines

### Banned Promotional Terms

Avoid these in commits/PRs/changelogs:
- ❌ "Comprehensive" (unless you have metrics)
- ❌ "Production-ready" (unless extensively tested)
- ❌ "Fully tested" (unless coverage > 90%)
- ❌ "World-class" / "Best-in-class"
- ❌ "Optimized" (unless benchmarked)
- ❌ "Robust" / "Rock-solid" (unless fault-tested)
- ❌ "Enterprise-grade" (marketing term)
- ❌ "Cutting-edge" / "State-of-the-art"
- ❌ "Revolutionary" / "Groundbreaking"
- ❌ "Seamless" / "Effortless"
- ❌ Exclamation marks (!)

### Preferred Factual Terms

Use these instead:
- ✅ "Implemented [X]" → "Added code for [X] - compiles but untested"
- ✅ "Fixed [bug]" → "Modified [X] to address [Y] - verified in [environment]"
- ✅ "Tested" → "Tested on [platform] with [result] - see [evidence]"
- ✅ "Verified" → "Verified by [method] on [date] - [result]"
- ✅ "Working" → "Working in [environment] - [status in other environments]"
- ✅ "Documented" → "Documented in [file] - accuracy not validated"
- ✅ "Supports" → "Code exists for [X] - tested: [Y/N], result: [Z]"

---

## Metrics to Include

### For Code Changes
- Lines added/removed
- Files modified
- Binary size change
- Compilation status
- Test results (with evidence)

### For Features
- What works (with proof)
- What doesn't work (with evidence)
- What's untested (be specific)
- Platforms tested on
- Known limitations

### For Bug Fixes
- Original behavior
- New behavior
- How verified
- Platforms verified on
- Platforms not verified on
- Potential regressions

---

## Examples: Version Releases

### Bad Release Notes

❌ **v1.0 - Production Release**
```
We're excited to announce v1.0!

Features:
🚀 Full Raspberry Pi support (all models!)
🚀 Comprehensive driver suite
🚀 Production-ready bootloader
🚀 Formally verified correctness
🚀 Enterprise-grade security

Download now and experience the future of ARM bootloaders!
```

### Good Release Notes

✅ **v0.1-alpha - Initial Development Snapshot**
```
Development Status: ALPHA - Not production-ready

## What Works
- Compiles with aarch64-elf-gcc ✓
- Boots in QEMU virt machine ✓
- UART output functional (QEMU only) ✓
- Basic memory allocation (QEMU only) ✓

## What Doesn't Work
- Physical Raspberry Pi boot: FAILS (tested on Pi 4B)
- SD card driver: Non-functional in QEMU
- Most peripherals: Untested

## Test Status
- QEMU virt: 1/1 passed (100%)
- QEMU raspi3b: Requires GPU firmware
- Real hardware: 0/1 passed (0%)

## Platforms Tested
- ✓ QEMU virt (working)
- ✗ Raspberry Pi 4B (failed)
- ? All other platforms (untested)

## Known Issues
See SCORECARD.md for complete status

## Recommendation
Use for:
- Learning ARM64 bootloader concepts
- QEMU experimentation
- Understanding AI-assisted development

Do NOT use for:
- Production deployments
- Real Raspberry Pi projects (until hardware boot fixed)
- Safety-critical applications

## Next Steps
1. Fix Pi 4B boot (add GPU firmware)
2. Test on multiple Pi models
3. Validate drivers on real hardware
4. Build test suite
```

---

## Commit Message Examples (Complete)

### Good Commit Messages

```
[ADD] Initial UART driver implementation

What changed:
- Created uart.c/h with PL011 UART functions
- Added uart_init(), uart_putc(), uart_puts()
- Configured for 115200 baud, 8N1

Status:
- What works: Compiles, QEMU output visible
- What doesn't: Not tested on real hardware
- What's untested: Other baud rates, error handling

Evidence:
- Compilation: SUCCESS
- QEMU test: UART output appears correctly
- Hardware test: NOT PERFORMED
```

```
[FIX] Memory allocator heap corruption

What changed:
- Fixed off-by-one error in malloc() block sizing
- Added boundary checking in free()
- Corrected heap pointer alignment

Status:
- What works: QEMU malloc/free now stable
- What doesn't: Still untested on hardware
- What's untested: Large allocations (>1MB), fragmentation

Evidence:
- Before: QEMU crash after 10 mallocs
- After: QEMU stable for 1000+ mallocs
- Hardware: NOT TESTED

Test: Ran 1000 malloc/free cycles in QEMU - no crashes
```

```
[TEST] Failed boot attempt on Raspberry Pi 4B

What changed:
- Performed physical hardware test
- Documented failure mode
- Captured observations

Status:
- What works: NOTHING on Pi 4B hardware
- What doesn't: Boot process, UART, LED
- What's untested: Other Pi models

Evidence:
- UART: No output on serial console (115200 8N1)
- Power LED: Solid (board powered)
- Activity LED: No blinks
- Power draw: ~600mA (normal for Pi 4B idle)

Test procedure:
1. Copied bootloader.bin to SD card
2. Inserted into Pi 4B
3. Connected UART adapter (GPIO14/15)
4. Powered on, monitored for 60 seconds
5. Result: No activity

Root cause hypothesis:
- Missing GPU firmware files
- Incorrect boot file naming
- Memory layout issues

Next action: Add GPU firmware and retest
```

---

## Balanced Scorecard Updates

Every significant change should update SCORECARD.md:

```markdown
### [Date] Update

**Changed Files:**
- [file1]: [change description]
- [file2]: [change description]

**Status Changes:**
- [Component]: [Old status] → [New status]
- Reason: [Factual explanation]
- Evidence: [How determined]

**New Metrics:**
- [Metric]: [Old value] → [New value]

**Test Results:**
- [Platform]: [Result with evidence]
```

---

## Summary: Core Principles

1. **Fact over Spin**: Describe what actually happened, not aspirations
2. **Evidence Required**: Claims need proof or disclaimer
3. **Disclose Failures**: Failed tests are valuable information
4. **Be Specific**: "Works in QEMU virt" not "works everywhere"
5. **Acknowledge Gaps**: "Untested" is better than assumed working
6. **No Hype**: Let the work speak for itself
7. **Metrics Matter**: Numbers > adjectives
8. **Balance**: Show strengths AND weaknesses

---

## Enforcement

### Self-Review Checklist

Before committing, ask:
- [ ] Are all claims factually accurate?
- [ ] Is test evidence provided or absence noted?
- [ ] Are limitations honestly disclosed?
- [ ] Did I avoid promotional language?
- [ ] Would I stake my reputation on these claims?
- [ ] If someone asked "how do you know?", can I answer?

### Tone Test

Read your commit message. Does it sound like:
- ❌ A product launch announcement?
- ❌ Marketing copy?
- ❌ A press release?
- ✅ A lab notebook entry?
- ✅ A scientific paper results section?
- ✅ An engineer's status report?

If it sounds promotional → rewrite factually.

---

**Maintained by**: Stuart (moonman81) + Claude Code
**Philosophy**: Feynman's "Don't fool yourself" principle
**Commitment**: Every commit follows these guidelines

---

*"It doesn't matter how beautiful your theory is, it doesn't matter how smart you are. If it doesn't agree with experiment, it's wrong."* - Richard Feynman
