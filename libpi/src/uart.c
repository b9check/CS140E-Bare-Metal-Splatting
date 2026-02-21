// simple mini-uart driver: implement every routine 
// with a <todo>.
//
// NOTE: 
//  - from broadcom: if you are writing to different 
//    devices you MUST use a dev_barrier().   
//  - its not always clear when X and Y are different
//    devices.
//  - pay attenton for errata!   there are some serious
//    ones here.  if you have a week free you'd learn 
//    alot figuring out what these are (esp hard given
//    the lack of printing) but you'd learn alot, and
//    definitely have new-found respect to the pioneers
//    that worked out the bcm eratta.
//
// historically a problem with writing UART code for
// this class (and for human history) is that when 
// things go wrong you can't print since doing so uses
// uart.  thus, debugging is very old school circa
// 1950s, which modern brains arne't built for out of
// the box.   you have two options:
//  1. think hard.  we recommend this.
//  2. use the included bit-banging sw uart routine
//     to print.   this makes things much easier.
//     but if you do make sure you delete it at the 
//     end, otherwise your GPIO will be in a bad state.
//
// in either case, in the next part of the lab you'll
// implement bit-banged UART yourself.
#include "rpi.h"
#include "gpio.h"


// change "1" to "0" if you want to comment out
// the entire block.
#if 1
//*****************************************************
// We provide a bit-banged version of UART for debugging
// your UART code.  delete when done!
//
// NOTE: if you call <emergency_printk>, it takes 
// over the UART GPIO pins (14,15). Thus, your UART 
// GPIO initialization will get destroyed.  Do not 
// forget!   

// header in <libpi/include/sw-uart.h>
#include "sw-uart.h"
static sw_uart_t sw_uart;

// a sw-uart putc implementation.
static int sw_uart_putc(int chr) {
    sw_uart_put8(&sw_uart,chr);
    return chr;
}

// call this routine to print stuff. 
//
// note the function pointer hack: after you call it 
// once can call the regular printk etc.
__attribute__((noreturn)) 
static void emergency_printk(const char *fmt, ...)  {
    // we forcibly initialize in case the 
    // GPIO got reset. this will setup 
    // gpio 14,15 for sw-uart.
    sw_uart = sw_uart_default();

    // all libpi output is via a <putc>
    // function pointer: this installs ours
    // instead of the default
    rpi_putchar_set(sw_uart_putc);

    // do print
    va_list args;
    va_start(args, fmt);
    vprintk(fmt, args);
    va_end(args);

    // at this point UART is all messed up b/c we took it over
    // so just reboot.   we've set the putchar so this will work
    clean_reboot();
}

#undef todo
#define todo(msg) do {                          \
    emergency_printk("%s:%d:%s\nDONE!!!\n",     \
            __FUNCTION__,__LINE__,msg);         \
} while(0)

// END of the bit bang code.
#endif

enum UART_REGS {
    AUX_ENABLES      = 0x20215004,  
    AUX_MU_IO_REG    = 0x20215040,
    AUX_MU_IER_REG   = 0x20215044,
    AUX_MU_IIR_REG   = 0x20215048,
    AUX_MU_LCR_REG   = 0x2021504C,
    AUX_MU_MCR_REG   = 0x20215050,  
    AUX_MU_LSR_REG   = 0x20215054,
    AUX_MU_CNTL_REG  = 0x20215060,
    AUX_MU_STAT_REG  = 0x20215064,  
    AUX_MU_BAUD      = 0x20215068
};


//*****************************************************
// the rest you should implement.

// called first to setup uart to 8n1 115200  baud,
// no interrupts.
//  - you will need memory barriers, use <dev_barrier()>
//
//  later: should add an init that takes a baud rate.
void uart_init(void) {
    // NOTE: for cross-checking: make sure write UART 
    // addresses in order
    
    // Turn on the mini UART in AUX (set bit 0)
    gpio_set_function(14, GPIO_FUNC_ALT5); 
    gpio_set_function(15, GPIO_FUNC_ALT5); 
    
    // Switching from GPIO to AUX device
    dev_barrier();

    // Enable UART
    unsigned aux = GET32(AUX_ENABLES);
    PUT32(AUX_ENABLES, aux | 1);

    // Switching from AUX to UART
    dev_barrier();

    // Disable TX/RX (clear last 2 bits):
    PUT32(AUX_MU_CNTL_REG, 0);

    // Disable interrupts (clear last 2 bits)
    PUT32(AUX_MU_IER_REG, 0);
  
    // Find and clear FIFO queues
    PUT32(AUX_MU_IIR_REG, 0x6);

    // Set to 8-bit mode
    PUT32(AUX_MU_LCR_REG, 0x3);

    // No flow control
    PUT32(AUX_MU_MCR_REG, 0);

    // Enable TX/RX (set last 2 bits):
    PUT32(AUX_MU_CNTL_REG, 0x3);

    // Set baudrate to 115200
    PUT32(AUX_MU_BAUD, 270);
    dev_barrier();
}

// disable the uart: make sure all bytes have been
// 
void uart_disable(void) {
    dev_barrier();
    uart_flush_tx();
    PUT32(AUX_MU_CNTL_REG, 0);  // disable TX/RX
    dev_barrier();
}

// returns one byte from the RX (input) hardware
// FIFO.  if FIFO is empty, blocks until there is 
// at least one byte.
int uart_get8(void) {
    dev_barrier();
    while (!uart_has_data());
    int c = GET32(AUX_MU_IO_REG) & 0xFF;
    dev_barrier();
    return c;
}

// returns 1 if the hardware TX (output) FIFO has room
// for at least one byte.  returns 0 otherwise.
int uart_can_put8(void) {
    return (GET32(AUX_MU_LSR_REG) >> 5) & 1;
}

// put one byte on the TX FIFO, if necessary, waits
// until the FIFO has space.
int uart_put8(uint8_t c) {
    dev_barrier();
    while (!uart_can_put8());
    PUT32(AUX_MU_IO_REG, c);
    dev_barrier();
    return 0;
}

// returns:
//  - 1 if at least one byte on the hardware RX FIFO.
//  - 0 otherwise
int uart_has_data(void) {
    return (GET32(AUX_MU_LSR_REG) & 1);
}

// returns:
//  -1 if no data on the RX FIFO.
//  otherwise reads a byte and returns it.
int uart_get8_async(void) { 
    if(!uart_has_data())
        return -1;
    return uart_get8();
}

// returns:
//  - 1 if TX FIFO empty AND idle.
//  - 0 if not empty.
int uart_tx_is_empty(void) {
    return (GET32(AUX_MU_LSR_REG) >> 6) & 1;
}

// return only when the TX FIFO is empty AND the
// TX transmitter is idle.  
//
// used when rebooting or turning off the UART to
// make sure that any output has been completely 
// transmitted.  otherwise can get truncated 
// if reboot happens before all bytes have been
// received.
void uart_flush_tx(void) {
    while (!uart_tx_is_empty())
        rpi_wait();
}
