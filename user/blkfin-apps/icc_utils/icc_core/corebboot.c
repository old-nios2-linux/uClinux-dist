/*
 * IPI management based on arch/arm/kernel/smp.c (Copyright 2002 ARM Limited)
 *
 * Copyright 2007-2009 Analog Devices Inc.
 *                         Philippe Gerum <rpm@xenomai.org>
 *
 * Licensed under the GPL-2.
 */

#if 0
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/spinlock.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/cache.h>
#include <linux/profile.h>
#include <linux/errno.h>
#include <linux/mm.h>
#include <linux/cpu.h>
#include <linux/smp.h>
#include <linux/cpumask.h>
#include <linux/seq_file.h>
#include <linux/slab.h>
#include <asm/atomic.h>
#include <asm/cacheflush.h>
#include <asm/mmu_context.h>
#include <asm/pgtable.h>
#include <asm/pgalloc.h>
#include <asm/processor.h>
#include <asm/ptrace.h>
#include <asm/cpu.h>
#include <asm/time.h>
#include <linux/err.h>
#endif

#include <generated/autoconf.h>

#include <mach/defBF561.h>

//#include <asm/posix_types.h>
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

extern void evt_evt7(void );
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

#ifdef CONFIG_BFIN_EXTMEM_WRITETHROUGH
#endif
extern int vsprintf(char *buf, const char *fmt, va_list args);


void udelay(sm_uint32_t count)
{
	while(count--);
}

void delay(sm_uint32_t count)
{
	sm_uint32_t ncount = 1000 * count;
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

void coreb_msg(char *fmt, ...)
{
	va_list args;
	int i;
	char buf[64] = "COREB: ";
	struct sm_message_queue *queue = (struct sm_message_queue *)MSGQ_START_ADDR;
	struct sm_msg *msg = &queue->messages[0];
	sm_atomic_t sent;
	sent = sm_atomic_read(&queue->sent);
	void *p = (void *)DEBUG_MSG_BUF_ADDR + (sent % SM_MSGQ_LEN) * 64;
	va_start(args, fmt);
	i = vsprintf(buf + 7, fmt, args);
	va_end(args);
	memset(p, 0, 64);
	SSYNC();
	strcpy(p, buf);
	memset(&msg[sent%SM_MSGQ_LEN], 0, sizeof(struct sm_msg));
	msg[(sent % SM_MSGQ_LEN)].type = SM_BAD_MSG;
	msg[(sent % SM_MSGQ_LEN)].dst_ep = 1;
	msg[(sent % SM_MSGQ_LEN)].payload = p;
	sent++;
	sm_atomic_write(&queue->sent, sent);
	platform_send_ipi_cpu(0, IRQ_SUPPLE_0);
	delay(2);
}

void init_exception_vectors(void)
{
        /* cannot program in software:
         * evt0 - emulation (jtag)
         * evt1 - reset
         */
	/* ipi evt */
	bfin_write_EVT7(evt_evt7);
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
	sm_handle_control_message(cpu);
	return IRQ_HANDLED;
}

irqreturn_t ipi_handler_int1(int irq, void *dev_instance)
{
	sm_uint32_t cpu = blackfin_core_id();

	platform_clear_ipi(cpu, IRQ_SUPPLE_1);

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
	bfin_irq_flags = IMASK_IVG7 ;
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

#define coreb_idle() \
	__asm__ __volatile__( \
			".align 8;" \
			"nop;"	\
			"nop;"	\
			"idle;" \
			: \
			:  \
			)


void bfin_setup_caches(unsigned int cpu)
{
	unsigned long addr;
	int i;
	unsigned int *p = (int *) 0xFEB1FFFC;

//	coreb_msg("IPEND %X \n", readipend());

	addr = 4 * 1024 * 1024;
	i = 0;

#if 0
	i++;
	bfin_write32(ICPLB_ADDR0 + i * 4, COREB_L1_CODE_START);
	bfin_write32(ICPLB_DATA0 + i * 4, (L1_IMEMORY | PAGE_SIZE_4MB));
	bfin_write32(DCPLB_ADDR0 + i * 4, COREB_L1_DATA_A_START);
	bfin_write32(DCPLB_DATA0 + i * 4, (L1_DMEMORY | PAGE_SIZE_4MB));
	i++;
#endif

	bfin_write32(ICPLB_ADDR0 + i * 4, L2_START);
	bfin_write32(ICPLB_DATA0 + i * 4, (CPLB_COMMON | PAGE_SIZE_1MB));
	bfin_write32(DCPLB_ADDR0 + i * 4, L2_START);
	bfin_write32(DCPLB_DATA0 + i * 4, (CPLB_COMMON | PAGE_SIZE_1MB));
	i++;

	for(i = 1; i < 16; i++) {
		bfin_write32(ICPLB_ADDR0 + i * 4, addr + (i - 1) * 4 * 1024 * 1024);
		bfin_write32(ICPLB_DATA0 + i * 4 ,(SDRAM_IGENERIC | PAGE_SIZE_4MB));
		bfin_write32(DCPLB_ADDR0 + i * 4, addr + (i - 1) * 4 * 1024 * 1024);
		bfin_write32(DCPLB_DATA0 + i * 4, (CPLB_COMMON | PAGE_SIZE_4MB));
	}
	_enable_cplb(IMEM_CONTROL, (IMC | ENICPLB));

	delay(1);

//	coreb_msg("IMEM %X \n", (IMC | ENICPLB));

	_enable_cplb(DMEM_CONTROL, (DMEM_CNTR | PORT_PREF0 | PORT_PREF1 ));

}

void coreb_icc_dispatcher(void)
{

	int pending;
	int cpu = 1;
	int bfin_irq_flags;
	while (iccq_should_stop) {
		/*to do drop no control messages*/
		coreb_idle();
	}
	pending = iccqueue_getpending(cpu);
	if (!pending) {
		coreb_idle();

		coreb_msg("@@@ wake up\n");
		return;
	}
	{
		struct sm_message_queue *inqueue = &coreb_info.icc_info.icc_queue[1];
		sm_atomic_t sent = sm_atomic_read(&inqueue->sent);
		sm_atomic_t received = sm_atomic_read(&inqueue->received);
		sm_atomic_t pending;
//		coreb_msg("@@@sm msgq sent=%d received=%d\n", sent, received);
	}
	msg_handle();

}

void icc_init(void)
{
	unsigned long addr1, addr2 = 0;
	struct gen_pool *pool;
	pool = gen_pool_create(12);

	coreb_info.pool = pool;
	if (gen_pool_add(pool, 0x3c01000, 4 * 1024 * 64))
		coreb_msg("@@@add chunk fail\n");

	pool = gen_pool_create(4);
	if (!pool)
		coreb_msg("@@@ create pool failed\n");
	if (gen_pool_add(pool, 0x3c00000, 4 * 64))
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

	icc_init();

	while(1) {
restart:
		coreb_icc_dispatcher();
	}
}

