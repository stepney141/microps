/* a dummy device (mock) */

#include "dummy.h"

#include "intr.h"
#include "net.h"
#include "platform.h"
#include "util.h"

#define DUMMY_IRQ INTR_IRQ_BASE  // IRQ number for the dummy device
#define DUMMY_MTU UINT16_MAX

static int dummy_transmit(struct net_device *dev, uint16_t type,
                          const uint8_t *data, size_t len, const void *dst) {
    debugf("dev=%s, type=0x%04x, len=%zu", dev->name, type, len);
    debugdump(data, len);
    /* drop data */
    intr_raise_irq(DUMMY_IRQ);
    return 0;
}

static struct net_device_ops dummy_ops = {
    .transmit = dummy_transmit,
};

static int dummy_isr(unsigned int irq, void *id) {
    debugf("irq=%u, dev=%s", irq, ((struct net_device *)id)->name);
    return 0;
}

struct net_device *dummy_init(void) {
    struct net_device *dev;

    dev = net_device_alloc();  // Generate a dummy device
    if (!dev) {
        errorf("net_device_alloc() failed");
        return NULL;
    }
    dev->type = NET_DEVICE_TYPE_DUMMY;
    dev->mtu = DUMMY_MTU;
    dev->hlen = 0; /* no header */
    dev->alen = 0; /* no address */
    dev->ops = &dummy_ops;
    if (net_device_register(dev) == -1) {
        errorf("net_device_register() failure");
        return NULL;
    }
    intr_request_irq(DUMMY_IRQ, dummy_isr, INTR_IRQ_SHARED, dev->name, dev);
    debugf("initialized, dev=%s", dev->name);
    return dev;
}
