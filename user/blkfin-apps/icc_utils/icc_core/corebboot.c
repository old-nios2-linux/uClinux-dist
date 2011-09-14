/*
 * IPI management based on arch/arm/kernel/smp.c (Copyright 2002 ARM Limited)
 *
 * Copyright 2007-2009 Analog Devices Inc.
 *                         Philippe Gerum <rpm@xenomai.org>
 *
 * Licensed under the GPL-2.
 */

#include <generated/autoconf.h>

#include <mach/defBF561.h>

#include <linux/types.h>
#include <mach/irq.h>
#include <mach/bf561.h>
#include <mach/cdefBF561.h>
#include <icc.h>
#include <blackfin.h>
#include <asm/cplb.h>
#include <asm/irqflags.h>

#include <stdarg.h>
#include <asm/blackfin.h>

#include <protocol.h>
#include <debug.h>

extern uint16_t pending;

static inline void coreb_idle(void)
{
	__asm__ __volatile__( \
			".align 8;" \
			"nop;"  \
			"nop;"  \
			"idle;" \
			: \
			:  \
			);
}


extern void evt_evt7(void );
extern void evt_evt6(void );
extern void evt_evt2(void );
extern void evt_evt3(void );
extern void evt_evt5(void );

#define blackfin_core_id() (bfin_dspid() & 0xff)

enum irqreturn {
	IRQ_NONE=0,
	IRQ_HANDLED,
	IRQ_WAKE_THREAD,
};

typedef enum irqreturn irqreturn_t;


extern unsigned long mcc_arg;
extern int iccq_should_stop;

sm_uint16_t intcnt;

#define BFIN_IPI_RESCHEDULE   0
#define BFIN_IPI_CALL_FUNC    1
#define BFIN_IPI_CPU_STOP     2

#if !CONFIG_BFIN_EXTMEM_WRITETHROUGH
# error need CONFIG_BFIN_EXTMEM_WRITETHROUGH
#endif
extern int vsprintf(char *buf, const char *fmt, va_list args);


void udelay(sm_uint32_t count)
{
	while(count--);
}

void delay(sm_uint32_t count)
{
	sm_uint32_t ncount = 30 * count;
	while(ncount--)
		udelay(10000);
}

void *memcpy(void *dest, const void *src, size_t count)
{
	char *tmp = dest;
	const char *s = src;

	while (count--)
		*tmp++ = *s++;
	return dest;
}

void *memset(void *s, int c, size_t count)
{
	char *xs = s;

	while (count--)
		*xs++ = c;
	return s;
}

char *strcpy(char *dest, const char *src)
{
	char *tmp = dest;

	while ((*dest++ = *src++) != '\0')
		/* nothing */;
	return tmp;
}

size_t strlen(const char *s)
{
	const char *sc;

	for (sc = s; *sc != '\0'; ++sc)
		/* nothing */;
	return sc - s;
}


void platform_send_ipi_cpu(unsigned int cpu, int irq)
{
	int offset = (irq == IRQ_SUPPLE_0) ? 6 : 8;
	SSYNC();
	bfin_write_SICB_SYSCR(bfin_read_SICB_SYSCR() | (1 << (offset + cpu)));
	SSYNC();
}

void platform_clear_ipi(unsigned int cpu, int irq)
{
	int offset = (irq == IRQ_SUPPLE_0) ? 10 : 12;
	SSYNC();
	bfin_write_SICB_SYSCR(bfin_read_SICB_SYSCR() | (1 << (offset + cpu)));
	SSYNC();
}

void bfin_coretmr_init(void)
{
	/* power up the timer, but don't enable it just yet */
	bfin_write_TCNTL(TMPWR);
	CSYNC();

	/* the TSCALE prescaler counter. */
	bfin_write_TSCALE(0);
	bfin_write_TPERIOD(0);
	bfin_write_TCOUNT(0);

	CSYNC();
}

int bfin_coretmr_set_next_event(unsigned long cycles)
{
	bfin_write_TCNTL(TMPWR);
	CSYNC();
	bfin_write_TCOUNT(cycles);
	CSYNC();
	bfin_write_TCNTL(TMPWR | TMREN);
	return 0;
}

#ifdef DEBUG
# define MSG_LINE 128
void coreb_msg(char *fmt, ...)
{
	va_list args;
	int i;
	char buf[MSG_LINE] = "COREB: ";
	struct sm_message_queue *queue = (struct sm_message_queue *)MSGQ_START_ADDR;
	struct sm_msg *msg = &queue->messages[0];
	sm_atomic_t sent, received;
	sent = sm_atomic_read(&queue->sent);
	received = sm_atomic_read(&queue->received);
	void *p = (void *)DEBUG_MSG_BUF_ADDR + (sent % SM_MSGQ_LEN) * MSG_LINE;
	va_start(args, fmt);
	i = vsprintf(buf + 7, fmt, args);
	va_end(args);
	memset(p, 0, MSG_LINE);
	SSYNC();
	strcpy(p, buf);
	while((sent - received) >= (SM_MSGQ_LEN - 1)) {
		delay(1);
		sent = sm_atomic_read(&queue->sent);
		received = sm_atomic_read(&queue->received);
	}
	memset(&msg[sent%SM_MSGQ_LEN], 0, sizeof(struct sm_msg));
	msg[(sent % SM_MSGQ_LEN)].type = SM_BAD_MSG;
	msg[(sent % SM_MSGQ_LEN)].dst_ep = received;
	msg[(sent % SM_MSGQ_LEN)].src_ep = (sent + 1);
	msg[(sent % SM_MSGQ_LEN)].payload = p;
	sent++;
	sm_atomic_write(&queue->sent, sent);
	SSYNC();
	platform_send_ipi_cpu(0, IRQ_SUPPLE_0);
	delay(1);
}
#endif

void dump_execption(unsigned int errno, unsigned int addr)
{
	coreb_msg("execption %x addr %x\n", errno, addr);
}

void init_exception_vectors(void)
{
        /* cannot program in software:
         * evt0 - emulation (jtag)
         * evt1 - reset
         */
	/* ipi evt */
	bfin_write_EVT7(evt_evt7);
	bfin_write_EVT6(evt_evt6);
	bfin_write_EVT2(evt_evt2);
	bfin_write_EVT3(evt_evt3);
	bfin_write_EVT5(evt_evt5);
	CSYNC();

}

void platform_secondary_init(void)
{
        bfin_write_SICB_IMASK0(bfin_read_SIC_IMASK0());
        bfin_write_SICB_IMASK1(bfin_read_SIC_IMASK1());
        SSYNC();

        /* Clone setup for IARs from CoreA. */
        bfin_write_SICB_IAR0(bfin_read_SIC_IAR0());
        bfin_write_SICB_IAR1(bfin_read_SIC_IAR1());
        bfin_write_SICB_IAR2(bfin_read_SIC_IAR2());
        bfin_write_SICB_IAR3(bfin_read_SIC_IAR3());
        bfin_write_SICB_IAR4(bfin_read_SIC_IAR4());
        bfin_write_SICB_IAR5(bfin_read_SIC_IAR5());
        bfin_write_SICB_IAR6(bfin_read_SIC_IAR6());
        bfin_write_SICB_IAR7(bfin_read_SIC_IAR7());
        bfin_write_SICB_IWR0(IWR_DISABLE_ALL);
        bfin_write_SICB_IWR1(0xC0000000);
        SSYNC();
}


/* Use IRQ_SUPPLE_0 to request reschedule.
 * When returning from interrupt to user space,
 * there is chance to reschedule */
irqreturn_t ipi_handler_int0(int irq, void *dev_instance)
{
	unsigned int cpu = blackfin_core_id();
	++intcnt;

	platform_clear_ipi(cpu, IRQ_SUPPLE_0);
	pending = iccqueue_getpending(cpu);
	sm_handle_control_message(cpu);
	return IRQ_HANDLED;
}

irqreturn_t ipi_handler_int1(int irq, void *dev_instance)
{
	sm_uint32_t cpu = blackfin_core_id();

	platform_clear_ipi(cpu, IRQ_SUPPLE_1);
	pending = iccqueue_getpending(cpu);
	return IRQ_HANDLED;
}

irqreturn_t timer_handle(int irq, void *dev_instance)
{
	sm_uint32_t cpu = blackfin_core_id();

	pending = iccqueue_getpending(cpu);
	return IRQ_HANDLED;
}

static void setup_secondary(unsigned int cpu)
{
	unsigned long ilat;
	unsigned long bfin_irq_flags;

	bfin_write_IMASK(0);
	CSYNC();
	ilat = bfin_read_ILAT();
	CSYNC();
	bfin_write_ILAT(ilat);
	CSYNC();

	/* Enable interrupt levels IVG7. IARs have been already
	 * programmed by the boot CPU.  */
//	bfin_irq_flags = IMASK_IVG7 | IMASK_IVGHW;
	bfin_irq_flags = IMASK_IVG7| IMASK_IVGTMR;
	bfin_sti(bfin_irq_flags);
	SSYNC();
}

inline int readipend(void)
{
	int _tmp;
	__asm__ __volatile__(
		"P1.H = ((0xffe02108>>16) & 0xFFFF);"
		"P1.L = (0xffe02108 & 0xFFFF);"
		"%0 = [P1];"
		: "=r" (_tmp) ::"P1"
	);
	return _tmp;
}


void bfin_setup_caches(unsigned int cpu)
{
	unsigned long addr;
	int i;

	addr = 4 * 1024 * 1024;
	i = 0;


	bfin_write32(ICPLB_ADDR0 + i * 4, L2_START);
	bfin_write32(ICPLB_DATA0 + i * 4, (CPLB_COMMON | PAGE_SIZE_1MB));
	bfin_write32(DCPLB_ADDR0 + i * 4, L2_START);
	bfin_write32(DCPLB_DATA0 + i * 4, (CPLB_COMMON | PAGE_SIZE_1MB));
	i++;

	for(i = 1; i < 16; i++) {
		bfin_write32(ICPLB_ADDR0 + i * 4, addr + (i - 1) * 4 * 1024 * 1024);
		bfin_write32(ICPLB_DATA0 + i * 4 ,((SDRAM_IGENERIC & ~CPLB_L1_CHBL) | PAGE_SIZE_4MB));
		bfin_write32(DCPLB_ADDR0 + i * 4, addr + (i - 1) * 4 * 1024 * 1024);
		bfin_write32(DCPLB_DATA0 + i * 4, (CPLB_COMMON | PAGE_SIZE_4MB));
	}
	_enable_cplb(IMEM_CONTROL, (IMC | ENICPLB));

	delay(1);

//	coreb_msg("IMEM %X \n", (IMC | ENICPLB));

	_enable_cplb(DMEM_CONTROL, (DMEM_CNTR | PORT_PREF0 | PORT_PREF1 ));

}

void icc_run_task(void);
void coreb_icc_dispatcher(void)
{
	while (1) {
		while (iccq_should_stop) {
			/*to do drop no control messages*/
			coreb_idle();
		}
		icc_wait(0);
	}
}

void icc_init(void)
{
	struct gen_pool *pool;

	memset(COREB_MEMPOOL_START , 0, 0x100000 * 2);
	pool = gen_pool_create(12);
	if (!pool)
		coreb_msg("@@@ create 4k pool failed\n");
	coreb_info.pool = pool;
	if (gen_pool_add(pool, COREB_MEMPOOL_START, (1 << 12) * 64))
		coreb_msg("@@@add chunk fail\n");

	pool = gen_pool_create(6);
	if (!pool)
		coreb_msg("@@@ create msg pool failed\n");
	coreb_info.msg_pool = pool;
	if (gen_pool_add(pool, COREB_MEMPOOL_START + (1 << 12) * 64 , (1 << 6) * 64))
		coreb_msg("@@@add chunk fail\n");


	coreb_info.icc_info.icc_queue = (struct sm_message_queue *)MSGQ_START_ADDR;
	init_sm_session_table();
	register_sm_proto();
}

void secondary_start_kernel(void)
{
	unsigned int cpu = blackfin_core_id();
	init_exception_vectors();
	SSYNC();

	setup_secondary(cpu);

	platform_secondary_init();

	bfin_setup_caches(cpu);

	bfin_coretmr_init();

	icc_init();

	coreb_icc_dispatcher();
}

