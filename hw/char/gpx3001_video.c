/*
 * GPX3001 video framebuffer emulation.
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
#include "ui/console.h"

struct gpx_video_state {
    SysBusDevice parent_obj;

    MemoryRegion iomem;
    qemu_irq irq;
    CharBackend chr;

    QemuConsole *con;
    int redraw;
};

#define TYPE_GPX_VIDEO "gpx3001-video"
OBJECT_DECLARE_SIMPLE_TYPE(gpx_video_state, GPX_VIDEO)

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

#define BUFFER_LEN ((0xf000 - 0xc000) / 2)
static char buf[BUFFER_LEN];

void gpx_video_write(void *opaque, hwaddr addr,
                    uint64_t val, unsigned size)
{
    unsigned i;
    unsigned offset;
    gpx_video_state *s = (gpx_video_state *)opaque;
#if 0
    hwaddr base_addr = 0xc000;
    static unsigned line_number, last_line_number = 0;
#endif

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

        s->redraw = 1;
    }
}

static void gpx_video_reset(DeviceState *dev)
{
    gpx_video_state *s = GPX_VIDEO(dev);

    (void)s;
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

static void gpx_update_display(void *opaque, console_ch_t *chardata)
{
    int i;
    char c;
    gpx_video_state *s = (gpx_video_state *)opaque;

    if (!s->redraw)
        return;

    dpy_text_cursor(s->con, -1, -1);
    dpy_text_resize(s->con, 0x51, BUFFER_LEN / 0x51 / 2);

    for (i = 0; i < BUFFER_LEN; i++) {
        if (!(i % 0x50)) {
            c = '\n';
            console_write_ch(chardata++, ATTR2CHTYPE(c, QEMU_COLOR_BLUE,
                                                     QEMU_COLOR_BLACK, 1));
        }

        if (buf[i] >= ' ' && buf[i] < '~')
            c = buf[i];
        else
            c = ' ';

        console_write_ch(chardata++, ATTR2CHTYPE(c, QEMU_COLOR_WHITE,
                                                 QEMU_COLOR_BLACK, 1));
    }

    s->redraw = 0;
    dpy_text_update(s->con, 0, 0, 0x51, BUFFER_LEN / 0x51 / 2);
}

static void gpx_invalidate_display(void * opaque)
{
    gpx_video_state *s = (gpx_video_state *)opaque;

    s->redraw = 1;
}

static const GraphicHwOps gpx_ops = {
    .invalidate  = gpx_invalidate_display,
    .text_update  = gpx_update_display,
};

static void gpx_video_realize(DeviceState *dev, Error **errp)
{
    gpx_video_state *s = GPX_VIDEO(dev);

    s->redraw = 0;
    s->con = graphic_console_init(dev, 0, &gpx_ops, s);
    qemu_console_resize(s->con, 0x51, BUFFER_LEN / 0x51 / 2);
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
