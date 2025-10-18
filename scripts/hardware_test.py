#!/usr/bin/env python3
"""
Hardware Testing Framework for Bootloader
Tests bootloader components on actual Raspberry Pi hardware
"""

import serial
import time
import sys
import argparse

class HardwareTester:
    def __init__(self, port='/dev/ttyUSB0', baudrate=115200):
        self.port = port
        self.baudrate = baudrate
        self.serial = None

    def connect(self):
        """Connect to Raspberry Pi via UART"""
        try:
            self.serial = serial.Serial(self.port, self.baudrate, timeout=1)
            print(f"Connected to {self.port} at {self.baudrate} baud")
            return True
        except serial.SerialException as e:
            print(f"Failed to connect: {e}")
            return False

    def disconnect(self):
        """Disconnect from UART"""
        if self.serial:
            self.serial.close()
            self.serial = None

    def read_output(self, timeout=30):
        """Read bootloader output until timeout"""
        output = ""
        start_time = time.time()

        while time.time() - start_time < timeout:
            if self.serial and self.serial.in_waiting:
                data = self.serial.read(self.serial.in_waiting).decode('utf-8', errors='ignore')
                output += data
                print(data, end='', flush=True)

                # Check for completion markers
                if "Boot successful!" in output:
                    break
                if "Boot failed!" in output:
                    break

            time.sleep(0.1)

        return output

    def test_network_boot(self):
        """Test network booting with DHCP"""
        print("\n=== Testing Network Boot ===")

        if not self.connect():
            return False

        print("Waiting for bootloader output...")
        output = self.read_output(60)  # 60 second timeout for network

        success_indicators = [
            "Network config successful",
            "DHCP successful",
            "IP:",
            "Boot successful!"
        ]

        failures = [
            "Network configuration failed",
            "DHCP failed",
            "Boot failed!"
        ]

        success = any(indicator in output for indicator in success_indicators)
        failure = any(fail in output for fail in failures)

        self.disconnect()

        if success and not failure:
            print("✓ Network boot test PASSED")
            return True
        else:
            print("✗ Network boot test FAILED")
            print("Output analysis:")
            for line in output.split('\n'):
                if any(keyword in line for keyword in success_indicators + failures):
                    print(f"  {line}")
            return False

    def test_usb_boot(self):
        """Test USB mass storage booting"""
        print("\n=== Testing USB Boot ===")

        if not self.connect():
            return False

        print("Waiting for bootloader output...")
        output = self.read_output(45)  # 45 second timeout for USB

        success_indicators = [
            "USB mass storage device found",
            "USB boot initialized",
            "Boot successful!"
        ]

        failures = [
            "No USB mass storage device found",
            "USB boot not implemented",
            "Boot failed!"
        ]

        success = any(indicator in output for indicator in success_indicators)
        failure = any(fail in output for fail in failures)

        self.disconnect()

        if success and not failure:
            print("✓ USB boot test PASSED")
            return True
        else:
            print("✗ USB boot test FAILED")
            print("Output analysis:")
            for line in output.split('\n'):
                if any(keyword in line for keyword in success_indicators + failures):
                    print(f"  {line}")
            return False

    def test_sd_boot(self):
        """Test standard SD card booting"""
        print("\n=== Testing SD Card Boot ===")

        if not self.connect():
            return False

        print("Waiting for bootloader output...")
        output = self.read_output(30)  # 30 second timeout for SD

        success_indicators = [
            "Loading kernel...",
            "Executing kernel...",
            "Boot successful!"
        ]

        failures = [
            "Kernel load failed",
            "Boot failed!"
        ]

        success = any(indicator in output for indicator in success_indicators)
        failure = any(fail in output for fail in failures)

        self.disconnect()

        if success and not failure:
            print("✓ SD boot test PASSED")
            return True
        else:
            print("✗ SD boot test FAILED")
            print("Output analysis:")
            for line in output.split('\n'):
                if any(keyword in line for keyword in success_indicators + failures):
                    print(f"  {line}")
            return False

    def run_full_test_suite(self):
        """Run complete hardware test suite"""
        print("=== Raspberry Pi Bootloader Hardware Test Suite ===")
        print("Ensure:")
        print("1. Bootloader is flashed to SD card")
        print("2. UART is connected to Raspberry Pi")
        print("3. For network tests: Ethernet cable connected to DHCP network")
        print("4. For USB tests: USB mass storage device inserted")
        print()

        results = []

        # Test SD boot (baseline)
        results.append(("SD Card Boot", self.test_sd_boot()))

        # Test network boot
        results.append(("Network Boot", self.test_network_boot()))

        # Test USB boot
        results.append(("USB Boot", self.test_usb_boot()))

        # Summary
        print("\n=== Test Results Summary ===")
        passed = 0
        for test_name, result in results:
            status = "PASSED" if result else "FAILED"
            print(f"{test_name}: {status}")
            if result:
                passed += 1

        print(f"\nOverall: {passed}/{len(results)} tests passed")

        if passed == len(results):
            print("🎉 All hardware tests PASSED!")
            return True
        else:
            print("❌ Some hardware tests FAILED")
            return False

def main():
    parser = argparse.ArgumentParser(description='Hardware test Raspberry Pi bootloader')
    parser.add_argument('--port', default='/dev/ttyUSB0', help='UART port (default: /dev/ttyUSB0)')
    parser.add_argument('--baud', type=int, default=115200, help='UART baud rate (default: 115200)')
    parser.add_argument('--test', choices=['sd', 'network', 'usb', 'full'], default='full',
                       help='Test type to run (default: full)')

    args = parser.parse_args()

    tester = HardwareTester(args.port, args.baud)

    if args.test == 'sd':
        success = tester.test_sd_boot()
    elif args.test == 'network':
        success = tester.test_network_boot()
    elif args.test == 'usb':
        success = tester.test_usb_boot()
    else:
        success = tester.run_full_test_suite()

    sys.exit(0 if success else 1)

if __name__ == "__main__":
    main()