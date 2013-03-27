#ifndef _CACHEFLUSH_H_
#define _CACHEFLUSH_H_

#if defined(CONFIG_BFIN_DCACHE)
# define invalidate_dcache_range(start,end)     blackfin_dcache_invalidate_range((start), (end))
#else
# define invalidate_dcache_range(start,end)     do { } while (0)
#endif

#if defined(CONFIG_BFIN_EXTMEM_WRITEBACK) || defined(CONFIG_BFIN_L2_WRITEBACK)
# define flush_dcache_range(start,end)          blackfin_dcache_flush_range((start), (end))
#define ARCH_IMPLEMENTS_FLUSH_DCACHE_PAGE 1
# define flush_dcache_page(page)                blackfin_dflush_page(page_address(page))
#else
# define flush_dcache_range(start,end)          do { } while (0)
#define ARCH_IMPLEMENTS_FLUSH_DCACHE_PAGE 0
# define flush_dcache_page(page)                do { } while (0)
#endif

#endif
