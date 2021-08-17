/*
 * ColdFire UART emulation.
 *
 * Copyright (c) 2007 CodeSourcery.
 *
 * This code is licensed under the GPL
 */

#include "qemu/osdep.h"
#include "hw/irq.h"
#include "hw/sysbus.h"
#include "qemu/module.h"
#include "qapi/error.h"
#include "hw/m68k/gpx.h"
#include "hw/qdev-properties.h"
#include "hw/qdev-properties-system.h"
#include "chardev/char-fe.h"
#include "qom/object.h"

struct gpx_video_state {
    SysBusDevice parent_obj;

    MemoryRegion iomem;
    uint8_t mr[2];
    uint8_t sr;
    uint8_t isr;
    uint8_t imr;
    uint8_t bg1;
    uint8_t bg2;
    uint8_t fifo[4];
    uint8_t tb;
    int current_mr;
    int fifo_len;
    int tx_enabled;
    int rx_enabled;
    qemu_irq irq;
    CharBackend chr;
};

#define TYPE_GPX_VIDEO "gpx3001-video"
OBJECT_DECLARE_SIMPLE_TYPE(gpx_video_state, GPX_VIDEO)

/* UART Status Register bits.  */
#define MCF_UART_RxRDY  0x01
#define MCF_UART_FFULL  0x02
#define MCF_UART_TxRDY  0x04
#define MCF_UART_TxEMP  0x08
#define MCF_UART_OE     0x10
#define MCF_UART_PE     0x20
#define MCF_UART_FE     0x40
#define MCF_UART_RB     0x80

/* Interrupt flags.  */
#define MCF_UART_TxINT  0x01
#define MCF_UART_RxINT  0x02
#define MCF_UART_DBINT  0x04
#define MCF_UART_COSINT 0x80

/* UMR1 flags.  */
#define MCF_UART_BC0    0x01
#define MCF_UART_BC1    0x02
#define MCF_UART_PT     0x04
#define MCF_UART_PM0    0x08
#define MCF_UART_PM1    0x10
#define MCF_UART_ERR    0x20
#define MCF_UART_RxIRQ  0x40
#define MCF_UART_RxRTS  0x80

static void gpx_video_update(gpx_video_state *s)
{
#if 0
    s->isr &= ~(MCF_UART_TxINT | MCF_UART_RxINT);
    if (s->sr & MCF_UART_TxRDY)
        s->isr |= MCF_UART_TxINT;
    if ((s->sr & ((s->mr[0] & MCF_UART_RxIRQ)
                  ? MCF_UART_FFULL : MCF_UART_RxRDY)) != 0)
        s->isr |= MCF_UART_RxINT;

    qemu_set_irq(s->irq, (s->isr & s->imr) != 0);
#endif
}

uint64_t gpx_video_read(void *opaque, hwaddr addr,
                       unsigned size)
{
    //gpx_video_state *s = (gpx_video_state *)opaque;
//    fprintf(stdout, "read addr: 0x%08lx\n", addr);
//    fprintf(stdout, "read size: %u\n", size);

    switch (addr & 0x1ffff) {
    default:
        return 0;
    }
}

#if 0
/* Update TxRDY flag and set data if present and enabled.  */
static void gpx_video_do_tx(gpx_video_state *s)
{
    if (s->tx_enabled && (s->sr & MCF_UART_TxEMP) == 0) {
        /* XXX this blocks entire thread. Rewrite to use
         * qemu_chr_fe_write and background I/O callbacks */
        qemu_chr_fe_write_all(&s->chr, (unsigned char *)&s->tb, 1);
        s->sr |= MCF_UART_TxEMP;
    }
    if (s->tx_enabled) {
        s->sr |= MCF_UART_TxRDY;
    } else {
        s->sr &= ~MCF_UART_TxRDY;
    }
}
#endif

#if 0
static void gpx_do_command(gpx_video_state *s, uint8_t cmd)
{
    /* Misc command.  */
    switch ((cmd >> 4) & 7) {
    case 0: /* No-op.  */
        break;
    case 1: /* Reset mode register pointer.  */
        s->current_mr = 0;
        break;
    case 2: /* Reset receiver.  */
        s->rx_enabled = 0;
        s->fifo_len = 0;
        s->sr &= ~(MCF_UART_RxRDY | MCF_UART_FFULL);
        break;
    case 3: /* Reset transmitter.  */
        s->tx_enabled = 0;
        s->sr |= MCF_UART_TxEMP;
        s->sr &= ~MCF_UART_TxRDY;
        break;
    case 4: /* Reset error status.  */
        break;
    case 5: /* Reset break-change interrupt.  */
        s->isr &= ~MCF_UART_DBINT;
        break;
    case 6: /* Start break.  */
    case 7: /* Stop break.  */
        break;
    }

    /* Transmitter command.  */
    switch ((cmd >> 2) & 3) {
    case 0: /* No-op.  */
        break;
    case 1: /* Enable.  */
        s->tx_enabled = 1;
        gpx_video_do_tx(s);
        break;
    case 2: /* Disable.  */
        s->tx_enabled = 0;
        gpx_video_do_tx(s);
        break;
    case 3: /* Reserved.  */
        fprintf(stderr, "gpx_video: Bad TX command\n");
        break;
    }

    /* Receiver command.  */
    switch (cmd & 3) {
    case 0: /* No-op.  */
        break;
    case 1: /* Enable.  */
        s->rx_enabled = 1;
        break;
    case 2:
        s->rx_enabled = 0;
        break;
    case 3: /* Reserved.  */
        fprintf(stderr, "gpx_video: Bad RX command\n");
        break;
    }
}
#endif

#define BUFFER_LEN ((0xf000 - 0xc000) / 2)

static char buf[BUFFER_LEN];

static void update_screen(void)
{
	int i;

	for (i = 0; i < BUFFER_LEN; i++) {
		if (buf[i] >= ' ' && buf[i] < '~')
			fprintf(stdout, "%c", buf[i]);
		else
			//fprintf(stdout, " 0x%02x ", buf[i]);
			fprintf(stdout, " ");

		if (!(i % 0x50))
			fprintf(stdout, "\n");
	}

	fflush(stdout);
}

void gpx_video_write(void *opaque, hwaddr addr,
                    uint64_t val, unsigned size)
{
    unsigned i;
    unsigned offset;
#if 0
    hwaddr base_addr = 0xc000;
    static unsigned line_number, last_line_number = 0;
#endif
    //gpx_video_state *s = (gpx_video_state *)opaque;

//    fprintf(stdout, "write addr: 0x%08lx\n", addr);
//    fprintf(stdout, "write size: %u\n", size);

    if (addr >= 0xc000 && addr < 0xf000) {
        /* Data is multiple of u16_t. Last byte is an attribute (blinking, cursor, etc.)
         * Just discard it.
         */
        for (i = 0; i < size / 2; i+=2) {
            offset = ((addr - 0xc000) + i)/2;
            if (offset > BUFFER_LEN) {
                fprintf(stdout, "\n*********** BUFFER OVERFLOW (%u / 0x%08lx)!!! ************\n", offset, addr);
                abort();
            }
            buf[offset] = (uint8_t)((val >> i*8) & 0xff);
        }

        update_screen();
    }
    //gpx_video_update(s);
}

static void gpx_video_reset(DeviceState *dev)
{
    gpx_video_state *s = GPX_VIDEO(dev);

    s->fifo_len = 0;
    s->mr[0] = 0;
    s->mr[1] = 0;
    s->sr = MCF_UART_TxEMP;
    s->tx_enabled = 0;
    s->rx_enabled = 0;
    s->isr = 0;
    s->imr = 0;
}

static void gpx_video_push_byte(gpx_video_state *s, uint8_t data)
{
    /* Break events overwrite the last byte if the fifo is full.  */
    if (s->fifo_len == 4)
        s->fifo_len--;

    s->fifo[s->fifo_len] = data;
    s->fifo_len++;
    s->sr |= MCF_UART_RxRDY;
    if (s->fifo_len == 4)
        s->sr |= MCF_UART_FFULL;

    gpx_video_update(s);
}

static void gpx_video_event(void *opaque, QEMUChrEvent event)
{
    gpx_video_state *s = (gpx_video_state *)opaque;

    switch (event) {
    case CHR_EVENT_BREAK:
        s->isr |= MCF_UART_DBINT;
        gpx_video_push_byte(s, 0);
        break;
    default:
        break;
    }
}

static int gpx_video_can_receive(void *opaque)
{
    gpx_video_state *s = (gpx_video_state *)opaque;

    return s->rx_enabled && (s->sr & MCF_UART_FFULL) == 0;
}

static void gpx_video_receive(void *opaque, const uint8_t *buf, int size)
{
    gpx_video_state *s = (gpx_video_state *)opaque;

    gpx_video_push_byte(s, buf[0]);
}

static const MemoryRegionOps gpx_video_ops = {
    .read = gpx_video_read,
    .write = gpx_video_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static void gpx_video_instance_init(Object *obj)
{
    SysBusDevice *dev = SYS_BUS_DEVICE(obj);
    gpx_video_state *s = GPX_VIDEO(dev);

    memory_region_init_io(&s->iomem, obj, &gpx_video_ops, s, "gpx-display", 0x20000);
    sysbus_init_mmio(dev, &s->iomem);

    sysbus_init_irq(dev, &s->irq);
}

static void gpx_video_realize(DeviceState *dev, Error **errp)
{
    gpx_video_state *s = GPX_VIDEO(dev);

    qemu_chr_fe_set_handlers(&s->chr, gpx_video_can_receive, gpx_video_receive,
                             gpx_video_event, NULL, s, NULL, true);
}

static Property gpx_video_properties[] = {
    DEFINE_PROP_CHR("chardev", gpx_video_state, chr),
    DEFINE_PROP_END_OF_LIST(),
};

static void gpx_video_class_init(ObjectClass *oc, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(oc);

    dc->realize = gpx_video_realize;
    dc->reset = gpx_video_reset;
    device_class_set_props(dc, gpx_video_properties);
    set_bit(DEVICE_CATEGORY_INPUT, dc->categories);
}

static const TypeInfo gpx_video_info = {
    .name          = TYPE_GPX_VIDEO,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(gpx_video_state),
    .instance_init = gpx_video_instance_init,
    .class_init    = gpx_video_class_init,
};

static void gpx_video_register(void)
{
    type_register_static(&gpx_video_info);
}

type_init(gpx_video_register)

void *gpx_video_init(qemu_irq irq, Chardev *chrdrv)
{
    DeviceState  *dev;

    dev = qdev_new(TYPE_GPX_VIDEO);
    if (chrdrv) {
        qdev_prop_set_chr(dev, "chardev", chrdrv);
    }
    sysbus_realize_and_unref(SYS_BUS_DEVICE(dev), &error_fatal);

    sysbus_connect_irq(SYS_BUS_DEVICE(dev), 0, irq);

    return dev;
}

void gpx_video_mm_init(hwaddr base, qemu_irq irq, Chardev *chrdrv)
{
    DeviceState  *dev;

    dev = gpx_video_init(irq, chrdrv);
    sysbus_mmio_map(SYS_BUS_DEVICE(dev), 0, base);
}
