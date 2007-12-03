/* GNU/Linux/Nios2 specific low level interface for the remote server for GDB */

#include "server.h"
#include "linux-low.h"

#ifdef HAVE_SYS_REG_H
#include <sys/reg.h>
#endif

#include <asm/ptrace.h>

/* FIXME: Definitions in gdb/nios2-tdep.c and asm/ptrace.h does not quite agree... */
static int nios2_regmap[] =
{
  PTR_R0  * 4, PTR_R1  * 4, PTR_R2  * 4, PTR_R3  * 4,
  PTR_R4  * 4, PTR_R5  * 4, PTR_R6  * 4, PTR_R7  * 4,
  PTR_R8  * 4, PTR_R9  * 4, PTR_R10 * 4, PTR_R11 * 4,
  PTR_R12 * 4, PTR_R13 * 4, PTR_R14 * 4, PTR_R15 * 4,

  PTR_R16 * 4, PTR_R17 * 4, PTR_R18 * 4, PTR_R19 * 4,
  PTR_R20 * 4, PTR_R21 * 4, PTR_R22 * 4, PTR_R23 * 4,
  PTR_R24 * 4, PTR_R25 * 4,

  PTR_GP  * 4, PTR_SP  * 4, PTR_FP  * 4, PTR_EA  * 4,
  PTR_BA  * 4, PTR_RA  * 4,
  (PTR_RA + 1) * 4, (PTR_RA + 2) * 4, (PTR_RA + 3) * 4,
  (PTR_RA + 4) * 4, (PTR_RA + 5) * 4, (PTR_RA + 6) * 4
};

#define nios2_num_regs (sizeof(nios2_regmap) / sizeof(nios2_regmap[0]))

static int
nios2_cannot_store_register (int regno)
{
  return (regno >= nios2_num_regs);
}

static int
nios2_cannot_fetch_register (int regno)
{
  return (regno >= nios2_num_regs);
}

static CORE_ADDR
nios2_get_pc ()
{
  unsigned long pc;
  collect_register_by_name ("pc", &pc);
  return pc;
}

static void
nios2_set_pc (CORE_ADDR pc)
{
  unsigned long newpc = pc;
  supply_register_by_name ("pc", &newpc);
}

static const unsigned long nios2_breakpoint = 0x003da03a;
#define nios2_breakpoint_len 4

static int
nios2_breakpoint_at (CORE_ADDR where)
{
  unsigned long insn;

  (*the_target->read_memory) (where, (char *) &insn, nios2_breakpoint_len);
  if (insn == nios2_breakpoint)
    return 1;

  /* If necessary, recognize more trap instructions here.  GDB only uses the
     one.  */
  return 0;
}

struct linux_target_ops the_low_target = {
  nios2_num_regs,
  nios2_regmap,
  nios2_cannot_fetch_register,
  nios2_cannot_store_register,
  nios2_get_pc,
  nios2_set_pc,
  (const char *) &nios2_breakpoint,
  nios2_breakpoint_len,
  NULL,
  0,
  nios2_breakpoint_at,
};
