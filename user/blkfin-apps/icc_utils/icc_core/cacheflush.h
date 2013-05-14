#ifndef _CACHEFLUSH_H_
#define _CACHEFLUSH_H_

#define invalidate_dcache_range(start,end)	blackfin_dcache_invalidate_range((start), (end))
#define flush_icache_range(start,end)		blackfin_icache_flush_range((start), (end))
#define flush_dcache_range(start,end)		blackfin_dcache_flush_range((start), (end))
#define ARCH_IMPLEMENTS_FLUSH_DCACHE_PAGE 1
#define flush_dcache_page(page)			blackfin_dflush_page(page_address(page))

#endif
