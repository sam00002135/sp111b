#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// RAM 記憶體
/* xv6 uses only 128MiB of memory. */
#define RAM_SIZE (1024 * 1024 * 128)

/* Same as QEMU virt machine, RAM starts at 0x80000000. */
#define RAM_BASE 0x80000000

// Core Local Interruptor (CLINT) 局部中斷 (內部)
#define CLINT_BASE 0x2000000
#define CLINT_SIZE 0x10000
#define CLINT_MTIMECMP (CLINT_BASE + 0x4000)
#define CLINT_MTIME (CLINT_BASE + 0xbff8)

// Platform-Level Interrupt Controller (PLIC) 全局中斷 (外部)
#define PLIC_BASE 0xc000000
#define PLIC_SIZE 0x4000000
#define PLIC_PENDING (PLIC_BASE + 0x1000)
#define PLIC_SENABLE (PLIC_BASE + 0x2080)
#define PLIC_SPRIORITY (PLIC_BASE + 0x201000)
#define PLIC_SCLAIM (PLIC_BASE + 0x201004)

// UART (透過 UART 傳送/接收 終端機 的訊息)
#define UART_BASE 0x10000000
#define UART_SIZE 0x100
enum { UART_RHR = UART_BASE, UART_THR = UART_BASE };
enum { UART_LCR = UART_BASE + 3, UART_LSR = UART_BASE + 5 };
enum { UART_LSR_RX = 1, UART_LSR_TX = 1 << 5 };

// VIRTIO : 磁碟控制介面。
#define VIRTIO_BASE 0x10001000
#define VIRTIO_SIZE 0x1000
#define VIRTIO_MAGIC (VIRTIO_BASE + 0x000)
#define VIRTIO_VERSION (VIRTIO_BASE + 0x004)
#define VIRTIO_DEVICE_ID (VIRTIO_BASE + 0x008)
#define VIRTIO_VENDOR_ID (VIRTIO_BASE + 0x00c)
#define VIRTIO_DEVICE_FEATURES (VIRTIO_BASE + 0x010)
#define VIRTIO_DRIVER_FEATURES (VIRTIO_BASE + 0x020)
#define VIRTIO_GUEST_PAGE_SIZE (VIRTIO_BASE + 0x028)
#define VIRTIO_QUEUE_SEL (VIRTIO_BASE + 0x030)
#define VIRTIO_QUEUE_NUM_MAX (VIRTIO_BASE + 0x034)
#define VIRTIO_QUEUE_NUM (VIRTIO_BASE + 0x038)
#define VIRTIO_QUEUE_PFN (VIRTIO_BASE + 0x040)
#define VIRTIO_QUEUE_NOTIFY (VIRTIO_BASE + 0x050)
#define VIRTIO_STATUS (VIRTIO_BASE + 0x070)

enum { VIRTIO_VRING_DESC_SIZE = 16, VIRTIO_DESC_NUM = 8 };

/* Range check
 * For any variable range checking:
 *     if (x >= minx && x <= maxx) ...
 * it is faster to use bit operation:
 *     if ((signed)((x - minx) | (maxx - x)) >= 0) ...
 */
#define RANGE_CHECK(x, minx, size) \
    ((int32_t)((x - minx) | (minx + size - 1 - x)) >= 0)

/* Machine level CSRs */
enum { MSTATUS = 0x300, MEDELEG = 0x302, MIDELEG, MIE, MTVEC };
enum { MEPC = 0x341, MCAUSE, MTVAL, MIP };

/* Supervisor level CSRs */
enum { SSTATUS = 0x100, SIE = 0x104, STVEC };
enum { SEPC = 0x141, SCAUSE, STVAL, SIP, SATP = 0x180 };

enum { MIP_SSIP = 1ULL << 1, MIP_MSIP = 1ULL << 3, MIP_STIP = 1ULL << 5 };
enum { MIP_MTIP = 1ULL << 7, MIP_SEIP = 1ULL << 9, MIP_MEIP = 1ULL << 11 };

#define PAGE_SIZE 4096 /* should be configurable */

typedef enum {
    OK = -1,
    INSTRUCTION_ADDRESS_MISALIGNED = 0,
    INSTRUCTION_ACCESS_FAULT = 1,
    ILLEGAL_INSTRUCTION = 2,
    BREAKPOINT = 3,
    LOAD_ADDRESS_MISALIGNED = 4,
    LOAD_ACCESS_FAULT = 5,
    STORE_AMO_ADDRESS_MISALIGNED = 6,
    STORE_AMO_ACCESS_FAULT = 7,
    INSTRUCTION_PAGE_FAULT = 12,
    LOAD_PAGE_FAULT = 13,
    STORE_AMO_PAGE_FAULT = 15,
} exception_t;

typedef enum {
    NONE = -1,
    SUPERVISOR_SOFTWARE_INTERRUPT = 1,
    MACHINE_SOFTWARE_INTERRUPT = 3,
    SUPERVISOR_TIMER_INTERRUPT = 5,
    MACHINE_TIMER_INTERRUPT = 7,
    SUPERVISOR_EXTERNAL_INTERRUPT = 9,
    MACHINE_EXTERNAL_INTERRUPT = 11,
} interrupt_t;

bool exception_is_fatal(const exception_t e)
{
    switch (e) {
    case INSTRUCTION_ADDRESS_MISALIGNED:
    case INSTRUCTION_ACCESS_FAULT:
    case LOAD_ACCESS_FAULT:
    case STORE_AMO_ADDRESS_MISALIGNED:
    case STORE_AMO_ACCESS_FAULT:
        return true;
    default:
        return false;
    }
}

// RAM 記憶體
struct ram {
    uint8_t *data;
};

struct ram *ram_new(const uint8_t *code, const size_t code_size)
{
    struct ram *ram = calloc(1, sizeof(struct ram));
    ram->data = calloc(RAM_SIZE, 1);
    memcpy(ram->data, code, code_size);
    return ram;
}

exception_t ram_load(const struct ram *ram,
                     const uint64_t addr,
                     const uint64_t size,
                     uint64_t *result)
{
    uint64_t index = addr - RAM_BASE, tmp = 0;
    switch (size) {
    case 64:
        tmp |= (uint64_t)(ram->data[index + 7]) << 56;
        tmp |= (uint64_t)(ram->data[index + 6]) << 48;
        tmp |= (uint64_t)(ram->data[index + 5]) << 40;
        tmp |= (uint64_t)(ram->data[index + 4]) << 32;
    case 32:
        tmp |= (uint64_t)(ram->data[index + 3]) << 24;
        tmp |= (uint64_t)(ram->data[index + 2]) << 16;
    case 16:
        tmp |= (uint64_t)(ram->data[index + 1]) << 8;
    case 8:
        tmp |= (uint64_t)(ram->data[index + 0]) << 0;
        *result = tmp;
        return OK;
    default:
        return LOAD_ACCESS_FAULT;
    }
}

exception_t ram_store(struct ram *ram,
                      const uint64_t addr,
                      const uint64_t size,
                      const uint64_t value)
{
    uint64_t index = addr - RAM_BASE;
    switch (size) {
    case 64:
        ram->data[index + 7] = (value >> 56) & 0xff;
        ram->data[index + 6] = (value >> 48) & 0xff;
        ram->data[index + 5] = (value >> 40) & 0xff;
        ram->data[index + 4] = (value >> 32) & 0xff;
    case 32:
        ram->data[index + 3] = (value >> 24) & 0xff;
        ram->data[index + 2] = (value >> 16) & 0xff;
    case 16:
        ram->data[index + 1] = (value >> 8) & 0xff;
    case 8:
        ram->data[index + 0] = (value >> 0) & 0xff;
        return OK;
    default:
        return STORE_AMO_ACCESS_FAULT;
    }
}

static void fatal(const char *msg)
{
    fprintf(stderr, "ERROR: Failed to %s.\n", msg);
    exit(1);
}

// Core Local Interruptor (CLINT) 局部中斷 (內部)
struct clint {
    uint64_t mtime, mtimecmp; // 時間控制暫存器
};

struct clint *clint_new()
{
    return calloc(1, sizeof(struct clint));
}

inline exception_t clint_load(const struct clint *clint,
                              const uint64_t addr,
                              const uint64_t size,
                              uint64_t *result)
{
    if (size != 64)
        return LOAD_ACCESS_FAULT;

    switch (addr) {
    case CLINT_MTIMECMP:
        *result = clint->mtimecmp;
        break;
    case CLINT_MTIME:
        *result = clint->mtime;
        break;
    default:
        *result = 0;
    }
    return OK;
}

inline exception_t clint_store(struct clint *clint,
                               const uint64_t addr,
                               const uint64_t size,
                               const uint64_t value)
{
    if (size != 64)
        return STORE_AMO_ACCESS_FAULT;

    switch (addr) {
    case CLINT_MTIMECMP:
        clint->mtimecmp = value;
        break;
    case CLINT_MTIME:
        clint->mtime = value;
        break;
    }
    return OK;
}

// Platform-Level Interrupt Controller (PLIC) 全局中斷 (外部)
struct plic {
    uint64_t pending;
    uint64_t senable;
    uint64_t spriority;
    uint64_t sclaim;
};

struct plic *plic_new()
{
    return calloc(1, sizeof(struct plic));
}

exception_t plic_load(const struct plic *plic,
                      const uint64_t addr,
                      const uint64_t size,
                      uint64_t *result)
{
    if (size != 32)
        return LOAD_ACCESS_FAULT;

    switch (addr) {
    case PLIC_PENDING:
        *result = plic->pending;
        break;
    case PLIC_SENABLE:
        *result = plic->senable;
        break;
    case PLIC_SPRIORITY:
        *result = plic->spriority;
        break;
    case PLIC_SCLAIM:
        *result = plic->sclaim;
        break;
    default:
        *result = 0;
    }
    return OK;
}

exception_t plic_store(struct plic *plic,
                       const uint64_t addr,
                       const uint64_t size,
                       const uint64_t value)
{
    if (size != 32)
        return STORE_AMO_ACCESS_FAULT;

    switch (addr) {
    case PLIC_PENDING:
        plic->pending = value;
        break;
    case PLIC_SENABLE:
        plic->senable = value;
        break;
    case PLIC_SPRIORITY:
        plic->spriority = value;
        break;
    case PLIC_SCLAIM:
        plic->sclaim = value;
        break;
    }
    return OK;
}

// UART (透過 UART 傳送/接收 終端機 的訊息)
struct uart {
    uint8_t data[UART_SIZE];
    bool interrupting;

    pthread_t tid;
    pthread_mutex_t lock;
    pthread_cond_t cond;
};

static void *uart_thread_func(void *priv)
{
    struct uart *uart = (struct uart *) priv;
    while (1) {
        char c;
        if (read(STDIN_FILENO, &c, 1) <= 0)  /* an error or EOF */
            continue;
        pthread_mutex_lock(&uart->lock);
        while ((uart->data[UART_LSR - UART_BASE] & UART_LSR_RX) == 1)
            pthread_cond_wait(&uart->cond, &uart->lock);

        uart->data[0] = c;
        uart->interrupting = true;
        uart->data[UART_LSR - UART_BASE] |= UART_LSR_RX;
        pthread_mutex_unlock(&uart->lock);
    }

    /* should not reach here */
    return NULL;
}

struct uart *uart_new()
{
    struct uart *uart = calloc(1, sizeof(struct uart));
    uart->data[UART_LSR - UART_BASE] |= UART_LSR_TX;
    pthread_mutex_init(&uart->lock, NULL);
    pthread_cond_init(&uart->cond, NULL);

    pthread_create(&uart->tid, NULL, uart_thread_func, (void *) uart);
    return uart;
}

exception_t uart_load(struct uart *uart,
                      const uint64_t addr,
                      const uint64_t size,
                      uint64_t *result)
{
    if (size != 8)
        return LOAD_ACCESS_FAULT;

    pthread_mutex_lock(&uart->lock);
    switch (addr) {
    case UART_RHR:
        pthread_cond_broadcast(&uart->cond);
        uart->data[UART_LSR - UART_BASE] &= ~UART_LSR_RX;
    default:
        *result = uart->data[addr - UART_BASE];
    }
    pthread_mutex_unlock(&uart->lock);
    return OK;
}

exception_t uart_store(struct uart *uart,
                       const uint64_t addr,
                       const uint64_t size,
                       const uint64_t value)
{
    if (size != 8)
        return STORE_AMO_ACCESS_FAULT;

    pthread_mutex_lock(&uart->lock);
    switch (addr) {
    case UART_THR:
        putchar(value & 0xff);
        fflush(stdout);
        break;
    default:
        uart->data[addr - UART_BASE] = value & 0xff;
    }
    pthread_mutex_unlock(&uart->lock);
    return OK;
}