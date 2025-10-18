/* Basic Driver Tests for BSP */

#include "uart.h"
#include "gpio.h"
#include "timer.h"
#include "interrupt.h"
#include "mailbox.h"
#include "clock.h"
#include "memory.h"
#include "dma.h"
#include "i2c.h"
#include "spi.h"
#include "pwm.h"
#include "usb.h"
#include "ethernet.h"

void test_gpio(void) {
    uart_puts("Testing GPIO...\n");

    // Test basic output functionality
    uart_puts("Testing GPIO output...\n");
    gpio_set_function(GPIO_LED_PIN, GPIO_FUNC_OUTPUT);
    gpio_set(GPIO_LED_PIN);
    timer_delay_ms(50);
    gpio_clear(GPIO_LED_PIN);
    timer_delay_ms(50);
    gpio_set(GPIO_LED_PIN);
    timer_delay_ms(50);
    gpio_toggle(GPIO_LED_PIN);
    timer_delay_ms(50);
    gpio_toggle(GPIO_LED_PIN);

    // Test input functionality (if we have a button pin available)
    uart_puts("Testing GPIO input...\n");
    gpio_set_function(GPIO_BTN_PIN, GPIO_FUNC_INPUT);
    gpio_set_pull(GPIO_BTN_PIN, GPIO_PUD_UP);
    uint8_t btn_state = gpio_read(GPIO_BTN_PIN);
    uart_puts("Button state: ");
    uart_puts(btn_state ? "HIGH\n" : "LOW\n");

    // Test pull-up/down functionality
    uart_puts("Testing GPIO pull-up/down...\n");
    gpio_set_pull(GPIO_BTN_PIN, GPIO_PUD_DOWN);
    timer_delay_ms(10);
    btn_state = gpio_read(GPIO_BTN_PIN);
    uart_puts("Pull-down state: ");
    uart_puts(btn_state ? "HIGH\n" : "LOW\n");

    gpio_set_pull(GPIO_BTN_PIN, GPIO_PUD_UP);
    timer_delay_ms(10);
    btn_state = gpio_read(GPIO_BTN_PIN);
    uart_puts("Pull-up state: ");
    uart_puts(btn_state ? "HIGH\n" : "LOW\n");

    // Test alternate functions (safe ones)
    uart_puts("Testing GPIO alternate functions...\n");
    gpio_set_function(14, GPIO_FUNC_ALT0); // UART TX
    gpio_set_function(15, GPIO_FUNC_ALT0); // UART RX
    timer_delay_ms(10);
    gpio_set_function(14, GPIO_FUNC_INPUT); // Restore
    gpio_set_function(15, GPIO_FUNC_INPUT); // Restore

    // Test interrupt setup (without actually triggering)
    uart_puts("Testing GPIO interrupt setup...\n");
    gpio_enable_interrupt(GPIO_BTN_PIN, GPIO_INT_RISING);
    timer_delay_ms(10);
    gpio_disable_interrupt(GPIO_BTN_PIN);
    gpio_clear_interrupt(GPIO_BTN_PIN);

    uart_puts("GPIO comprehensive test completed\n");
}

void test_timer(void) {
    uart_puts("Testing Timer...\n");

    uint64_t start = timer_get_counter();
    timer_delay_ms(10);
    uint64_t end = timer_get_counter();

    if (end > start) {
        uart_puts("Timer test passed\n");
    } else {
        uart_puts("Timer test failed\n");
    }
}

void test_memory(void) {
    uart_puts("Testing Memory...\n");

    void *ptr1 = memory_alloc(64);
    void *ptr2 = memory_alloc(128);
    void *ptr3 = memory_alloc(256);

    if (ptr1 && ptr2 && ptr3) {
        memory_free(ptr2);
        void *ptr4 = memory_alloc(64); // Should reuse freed block

        if (ptr4) {
            uart_puts("Memory test passed\n");
            memory_free(ptr1);
            memory_free(ptr3);
            memory_free(ptr4);
        } else {
            uart_puts("Memory test failed\n");
        }
    } else {
        uart_puts("Memory allocation failed\n");
    }
}

void test_mailbox(void) {
    uart_puts("Testing Mailbox...\n");

    uint32_t rev = mailbox_get_firmware_revision();
    if (rev != 0) {
        uart_puts("Mailbox test passed\n");
    } else {
        uart_puts("Mailbox test failed\n");
    }
}

void test_clock(void) {
    uart_puts("Testing Clock...\n");

    // Basic clock test - just check if functions don't crash
    clock_init();
    uart_puts("Clock test completed\n");
}

void test_dma(void) {
    uart_puts("Testing DMA...\n");

    dma_init();
    uart_puts("DMA test completed\n");
}

void test_i2c(void) {
    uart_puts("Testing I2C...\n");

    i2c_init(I2C_BUS_0);
    uart_puts("I2C test completed\n");
}

void test_spi(void) {
    uart_puts("Testing SPI...\n");

    spi_init(SPI_BUS_0);
    uart_puts("SPI test completed\n");
}

void test_pwm(void) {
    uart_puts("Testing PWM...\n");

    pwm_init();
    pwm_set_range(PWM_CHANNEL_1, 1024);
    pwm_set_data(PWM_CHANNEL_1, 512);
    pwm_enable(PWM_CHANNEL_1);
    timer_delay_ms(100);
    pwm_disable(PWM_CHANNEL_1);

    uart_puts("PWM test completed\n");
}

void test_usb(void) {
    uart_puts("Testing USB...\n");

    usb_init();
    usb_reset_controller();
    usb_start_controller();
    timer_delay_ms(10);
    usb_stop_controller();

    uart_puts("USB test completed\n");
}

void test_ethernet(void) {
    uart_puts("Testing Ethernet...\n");

    ethernet_init();
    ethernet_disable();

    uart_puts("Ethernet test completed\n");
}

void run_driver_tests(void) {
    uart_puts("Running BSP Driver Tests...\n\n");

    test_gpio();
    test_timer();
    test_memory();
    test_mailbox();
    test_clock();
    test_dma();
    test_i2c();
    test_spi();
    test_pwm();
    test_usb();
    test_ethernet();

    uart_puts("\nAll driver tests completed\n");
}