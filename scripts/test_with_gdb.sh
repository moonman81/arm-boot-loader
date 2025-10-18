#!/bin/bash
# Test bootloader with GDB to manually start execution

echo "======================================
  QEMU + GDB Boot Test
======================================"
echo ""
echo "This script starts QEMU in debug mode and connects GDB"
echo "to manually set PC and verify the bootloader works."
echo ""

# Start QEMU in background with GDB server
echo "Starting QEMU with GDB server..."
qemu-system-aarch64 \
    -M raspi3b \
    -m 1G \
    -kernel bootloader.bin \
    -serial stdio \
    -nographic \
    -S -s \
    -monitor none &

QEMU_PID=$!
sleep 1

echo "QEMU started (PID: $QEMU_PID)"
echo "GDB server listening on localhost:1234"
echo ""
echo "Now run this in another terminal:"
echo ""
echo "  aarch64-elf-gdb bootloader.elf"
echo "  (gdb) target remote localhost:1234"
echo "  (gdb) break *0x80000"
echo "  (gdb) set \$pc=0x80000"
echo "  (gdb) continue"
echo ""
echo "You should see the bootloader output!"
echo ""
echo "Press Ctrl-C to stop QEMU"

# Wait for QEMU
wait $QEMU_PID
