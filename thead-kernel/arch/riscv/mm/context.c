// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2012 Regents of the University of California
 * Copyright (C) 2017 SiFive
 */

#include <linux/mm.h>
#include <asm/tlbflush.h>
#include <asm/cacheflush.h>
#include <asm/mmu_context.h>

/*
 * When necessary, performs a deferred icache flush for the given MM context,
 * on the local CPU.  RISC-V has no direct mechanism for instruction cache
 * shoot downs, so instead we send an IPI that informs the remote harts they
 * need to flush their local instruction caches.  To avoid pathologically slow
 * behavior in a common case (a bunch of single-hart processes on a many-hart
 * machine, ie 'make -j') we avoid the IPIs for harts that are not currently
 * executing a MM context and instead schedule a deferred local instruction
 * cache flush to be performed before execution resumes on each hart.  This
 * actually performs that local instruction cache flush, which implicitly only
 * refers to the current hart.
 */
static inline void flush_icache_deferred(struct mm_struct *mm)
{
#ifdef CONFIG_SMP
	unsigned int cpu = smp_processor_id();
	cpumask_t *mask = &mm->context.icache_stale_mask;

	if (cpumask_test_cpu(cpu, mask)) {
		cpumask_clear_cpu(cpu, mask);
		/*
		 * Ensure the remote hart's writes are visible to this hart.
		 * This pairs with a barrier in flush_icache_mm.
		 */
		smp_mb();
		local_flush_icache_all();
	}

#endif
}

void switch_mm(struct mm_struct *prev, struct mm_struct *next,
	struct task_struct *task)
{
	unsigned int cpu;
	unsigned long asid;

	if (unlikely(prev == next))
		return;

	/*
	 * Mark the current MM context as inactive, and the next as
	 * active.  This is at least used by the icache flushing
	 * routines in order to determine who should be flushed.
	 */
	cpu = smp_processor_id();

	cpumask_set_cpu(cpu, mm_cpumask(next));

#ifdef CONFIG_MMU
	__asm__ __volatile__(
			"jal t0,1f\n\t"
			"1: \n\t"
			"jal t0,2f\n\t"
			"2: \n\t"
			"jal t0,3f\n\t"
			"3: \n\t"
			"jal t0,4f\n\t"
			"4: \n\t"
			"jal t0,5f\n\t"
			"5: \n\t"
			"jal t0,6f\n\t"
			"6: \n\t"
			"jal t0,7f\n\t"
			"7: \n\t"
			"jal t0,8f\n\t"
			"8: \n\t"
			"jal t0,9f\n\t"
			"9: \n\t"
			"jal t0,10f\n\t"
			"10: \n\t"
			"jal t0,11f\n\t"
			"11: \n\t"
			"jal t0,12f\n\t"
			"12: \n\t"
			::: "memory", "t0");

	check_and_switch_context(next, cpu);
	asid = (next->context.asid.counter & SATP_ASID_MASK)
		<< SATP_ASID_SHIFT;

	local_flush_tlb_page(0);

	/* flush utlb before setting satp */
	__asm__ __volatile__(
			"li t0, 0\n\t"
			"sfence.vma t0, t0\n\t"
			::: "memory", "t0");

	csr_write(sptbr, virt_to_pfn(next->pgd) | SATP_MODE | asid);
#endif

	flush_icache_deferred(next);
}

static DEFINE_PER_CPU(atomic64_t, active_asids);
static DEFINE_PER_CPU(u64, reserved_asids);

struct asid_info asid_info;

void check_and_switch_context(struct mm_struct *mm, unsigned int cpu)
{
	asid_check_context(&asid_info, &mm->context.asid, cpu, mm);
}

static void asid_flush_cpu_ctxt(void)
{
	local_flush_tlb_all();
}

static int asids_init(void)
{
	BUG_ON(((1 << SATP_ASID_BITS) - 1) <= num_possible_cpus());

	if (asid_allocator_init(&asid_info, SATP_ASID_BITS, 1,
				asid_flush_cpu_ctxt))
		panic("Unable to initialize ASID allocator for %lu ASIDs\n",
		      NUM_ASIDS(&asid_info));

	asid_info.active = &active_asids;
	asid_info.reserved = &reserved_asids;

	pr_info("ASID allocator initialised with %lu entries\n",
		NUM_CTXT_ASIDS(&asid_info));

	local_flush_tlb_all();
	return 0;
}
early_initcall(asids_init);