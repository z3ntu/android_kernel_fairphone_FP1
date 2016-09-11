#ifndef __MT_MEMORY_H__
#define __MT_MEMORY_H__

/*
 * Define constants.
 */

#define PHYS_OFFSET 0x80000000

#if defined(CONFIG_KEXEC_HARDBOOT)
#if defined(CONFIG_ARCH_MT6589)
#define KEXEC_HB_PAGE_ADDR		UL(0x9DD00000)
#else
#error "Adress for kexec hardboot page not defined"
#endif
#endif

/*
 * Define macros.
 */

/* IO_VIRT = 0xF0000000 | IO_PHYS[27:0] */
#define IO_VIRT_TO_PHYS(v) (0x10000000 | ((v) & 0x0fffffff))
#define IO_PHYS_TO_VIRT(p) (0xf0000000 | ((p) & 0x0fffffff))

#endif  /* !__MT_MEMORY_H__ */
