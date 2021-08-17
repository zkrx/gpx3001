#ifndef HW_GPX_H
#define HW_GPX_H
/* Motorola ColdFire device prototypes.  */

#include "target/m68k/cpu-qom.h"

/* gpx_uart.c */
uint64_t gpx_video_read(void *opaque, hwaddr addr,
                       unsigned size);
void gpx_video_write(void *opaque, hwaddr addr,
                    uint64_t val, unsigned size);
void *gpx_video_init(qemu_irq irq, Chardev *chr);
void gpx_video_mm_init(hwaddr base, qemu_irq irq, Chardev *chr);

/* mcf_intc.c */
qemu_irq *mcf_intc_init(struct MemoryRegion *sysmem,
                        hwaddr base,
                        M68kCPU *cpu);

/* mcf5206.c */
#define TYPE_MCF5206_MBAR "mcf5206-mbar"

#endif
