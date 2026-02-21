#include "rpi.h"
#include "mbox.h"

// dump out the entire messaage.  useful for debug.
void msg_dump(const char *msg, volatile uint32_t *u, unsigned nwords) {
    printk("%s\n", msg);
    for(int i = 0; i < nwords; i++)
        output("u[%d]=%x\n", i,u[i]);
}

/*
  This is given.

  Get board serial
    Tag: 0x00010004
    Request: Length: 0
    Response: Length: 8
    Value: u64: board serial
*/
uint64_t rpi_get_serialnum(void) {
    // 16-byte aligned 32-bit array
    volatile uint32_t msg[8] __attribute__((aligned(16)));

    // make sure aligned
    assert((unsigned)msg%16 == 0);

    msg[0] = 8*4;         // total size in bytes.
    msg[1] = 0;           // sender: always 0.
    msg[2] = 0x00010004;  // serial tag
    msg[3] = 8;           // total bytes avail for reply
    msg[4] = 0;           // request code [0].
    msg[5] = 0;           // space for 1st word of reply 
    msg[6] = 0;           // space for 2nd word of reply 
    msg[7] = 0;   // end tag

    // send and receive message
    mbox_send(MBOX_CH, msg);

#if 0
    // if you want to debug.
    output("got:\n");
    for(int i = 0; i < 8; i++)
        output("msg[%d]=%x\n", i, msg[i]);
#endif

    // should have value for success: 1<<31
    if(msg[1] != 0x80000000)
		panic("invalid response: got %x\n", msg[1]);

    // high bit should be set and reply size
    assert(msg[4] == ((1<<31) | 8));

    // for me the upper 32 bits were never non-zero.  
    // not sure if always true?
    assert(msg[6] == 0);
    return msg[5];
}

uint32_t rpi_get_memsize(void) {
    // 16-byte aligned 32-bit array
    volatile uint32_t msg[8] __attribute__((aligned(16)));

    // make sure aligned
    assert((unsigned)msg%16 == 0);

    msg[0] = 8*4;         // total size in bytes.
    msg[1] = 0;           // sender: always 0.
    msg[2] = 0x00010005;  // ARM memory tag
    msg[3] = 8;           // total bytes avail for reply
    msg[4] = 0;           // request code [0].
    msg[5] = 0;           // space for 1st word of reply 
    msg[6] = 0;           // space for 2nd word of reply 
    msg[7] = 0;   // end tag

    // send and receive message
    mbox_send(MBOX_CH, msg);

    // should have value for success: 1<<31
    if(msg[1] != 0x80000000)
		panic("invalid response: got %x\n", msg[1]);

    // high bit should be set and reply size
    assert(msg[4] == ((1<<31) | 8));

    // second value is size
    return msg[6];
}


uint32_t rpi_get_model(void) {
        volatile uint32_t msg[7] __attribute__((aligned(16)));
    assert((unsigned)msg % 16 == 0);

    msg[0] = 7 * 4;
    msg[1] = 0;
    msg[2] = 0x00010001;  // board model tag
    msg[3] = 4;           // response: 4 bytes
    msg[4] = 0;
    msg[5] = 0;           // model
    msg[6] = 0;

    mbox_send(MBOX_CH, msg);
    if(msg[1] != 0x80000000)
        panic("invalid response: got %x\n", msg[1]);
    assert(msg[4] == ((1 << 31) | 4));

    return msg[5];
}

// https://www.raspberrypi-spy.co.uk/2012/09/checking-your-raspberry-pi-board-version/
uint32_t rpi_get_revision(void) {
    volatile uint32_t msg[7] __attribute__((aligned(16)));
    assert((unsigned)msg % 16 == 0);

    msg[0] = 7 * 4;
    msg[1] = 0;
    msg[2] = 0x00010002;  // board revision tag
    msg[3] = 4;
    msg[4] = 0;
    msg[5] = 0;
    msg[6] = 0;

    mbox_send(MBOX_CH, msg);
    if(msg[1] != 0x80000000)
        panic("invalid response: got %x\n", msg[1]);
    assert(msg[4] == ((1 << 31) | 4));

    return msg[5];
}

uint32_t rpi_temp_get(void) {
    volatile uint32_t msg[8] __attribute__((aligned(16)));
    assert((unsigned)msg % 16 == 0);

    msg[0] = 8 * 4;
    msg[1] = 0;
    msg[2] = 0x00030006;  // temperature tag
    msg[3] = 8;           // response: id + temp = 8 bytes
    msg[4] = 0;
    msg[5] = 0;           // temperature id (0)
    msg[6] = 0;           // temperature value
    msg[7] = 0;

    mbox_send(MBOX_CH, msg);
    if(msg[1] != 0x80000000)
        panic("invalid response: got %x\n", msg[1]);
    assert(msg[4] == ((1 << 31) | 8));

    // msg[6] = temperature in thousandths of a degree C
    return msg[6];
}


// Tag: 0x00030002 - Get clock rate
uint32_t rpi_clock_curhz_get(uint32_t clk) {
    volatile uint32_t msg[8] __attribute__((aligned(16)));
    msg[0] = 8 * 4;
    msg[1] = 0;
    msg[2] = 0x00030002;
    msg[3] = 8;
    msg[4] = 0;
    msg[5] = clk;
    msg[6] = 0;
    msg[7] = 0;
    mbox_send(MBOX_CH, msg);
    if(msg[1] != 0x80000000)
        panic("invalid response: got %x\n", msg[1]);
    assert(msg[4] == ((1 << 31) | 8));
    return msg[6];
}

// Tag: 0x00030004 - Get max clock rate
uint32_t rpi_clock_maxhz_get(uint32_t clk) {
    volatile uint32_t msg[8] __attribute__((aligned(16)));
    msg[0] = 8 * 4; // message size (32 bytes)
    msg[1] = 0;  // always 0 for send
    msg[2] = 0x00030004; // Get max clock rate tag
    msg[3] = 8; // response: 8 bytes
    msg[4] = 0; // request: 4 bytes
    msg[5] = clk; // 
    msg[6] = 0;
    msg[7] = 0;
    mbox_send(MBOX_CH, msg);
    if(msg[1] != 0x80000000)
        panic("invalid response: got %x\n", msg[1]);
    assert(msg[4] == ((1 << 31) | 8));
    return msg[6];
}

// Tag: 0x00030047 - Get real (measured) clock rate
uint32_t rpi_clock_realhz_get(uint32_t clk) {
    volatile uint32_t msg[8] __attribute__((aligned(16)));
    msg[0] = 8 * 4;
    msg[1] = 0;
    msg[2] = 0x00030047;
    msg[3] = 8;
    msg[4] = 0;
    msg[5] = clk;
    msg[6] = 0;
    msg[7] = 0;
    mbox_send(MBOX_CH, msg);
    if(msg[1] != 0x80000000)
        panic("invalid response: got %x\n", msg[1]);
    assert(msg[4] == ((1 << 31) | 8));
    return msg[6];
}

// Tag: 0x00030007 - Get min clock rate
uint32_t rpi_clock_minhz_get(uint32_t clk) {
    volatile uint32_t msg[8] __attribute__((aligned(16)));
    msg[0] = 8 * 4;
    msg[1] = 0;
    msg[2] = 0x00030007;
    msg[3] = 8;
    msg[4] = 0;
    msg[5] = clk;
    msg[6] = 0;
    msg[7] = 0;
    mbox_send(MBOX_CH, msg);
    if(msg[1] != 0x80000000)
        panic("invalid response: got %x\n", msg[1]);
    assert(msg[4] == ((1 << 31) | 8));
    return msg[6];
}

// Tag: 0x00038002 - Set clock rate
uint32_t rpi_clock_hz_set(uint32_t clk, uint32_t hz) {
    volatile uint32_t msg[9] __attribute__((aligned(16)));
    msg[0] = 9 * 4;
    msg[1] = 0;
    msg[2] = 0x00038002;
    msg[3] = 12;
    msg[4] = 0;
    msg[5] = clk;
    msg[6] = hz;
    msg[7] = 0;       
    msg[8] = 0;
    mbox_send(MBOX_CH, msg);
    if(msg[1] != 0x80000000)
        panic("invalid response: got %x\n", msg[1]);
    assert(msg[4] == ((1 << 31) | 8));
    return msg[6];
}
