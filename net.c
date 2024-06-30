/* NOTE: if you want to add/delete the entries after net_run(), you need to
 * protect these lists with a mutex. */

#include "net.h"

#include <stdint.h>

#include "intr.h"
#include "platform.h"
#include "util.h"

static struct net_device*
    devices;  // Pointer to the head of the list of network devices

struct net_device* net_device_alloc(void)  // Allocate a network device
{
    struct net_device* dev;

    dev = memory_alloc(sizeof(*dev));
    if (!dev) {
        errorf("malloc() failed\n");
        return NULL;
    }
    return dev;
}

/* NOTE: must not be call after net_run() */
// Register a network device
int net_device_register(struct net_device* dev) {
    static unsigned int index = 0;

    dev->index = index++;  // Assign an index to the device
    snprintf(dev->name, sizeof(dev->name), "net%d",
             dev->index);  // Generate a name for the device (net0, net1, ...)
    dev->next = devices;   // Insert the device to the head of the list
    devices = dev;
    infof("registered, dev=%s, type=0x%04x", dev->name, dev->type);
    return 0;
}

// Open a network device
static int net_device_open(struct net_device* dev) {
    if (NET_DEVICE_IS_UP(dev)) {
        errorf("device is already opened, dev=%s", dev->name);
        return -1;
    }
    if (dev->ops->open) {
        if (dev->ops->open(dev) == -1) {
            errorf("open failed, dev=%s", dev->name);
            return -1;
        }
    }
    dev->flags |= NET_DEVICE_FLAG_UP;  // Set the UP flag
    infof("dev=%s, state=%s", dev->name, NET_DEVICE_STATE(dev));
    return 0;
}

// Close a network device
static int net_device_close(struct net_device* dev) {
    if (!NET_DEVICE_FLAG_UP) {
        errorf("device is already down, dev=%s", dev->name);
        return -1;
    }
    if (dev->ops->close) {
        if (dev->ops->close(dev) == -1) {
            errorf("close failed, dev=%s", dev->name);
            return -1;
        }
    }
    dev->flags &= ~NET_DEVICE_FLAG_UP;  // Clear the UP flag
    infof("dev=%s, state=%s", dev->name, NET_DEVICE_STATE(dev));
    return 0;
}

// Output to a network device
int net_device_output(struct net_device* dev, uint16_t type,
                      const uint8_t* data, size_t len, const void* dst) {
    if (!NET_DEVICE_IS_UP(dev)) {  // Check the device state
        errorf("device is down, dev=%s", dev->name);
        return -1;
    }
    if (len > dev->mtu) {  // Check the packet size
        errorf("too large packet, dev=%s, len=%zu, mtu=%u", dev->name, dev->mtu,
               len);
        return -1;
    }

    debugf("dev=%s, type=0x%04x, len=%zu", dev->name, type, len);
    debugdump(data, len);

    if (dev->ops->transmit(dev, type, data, len, dst) == -1) {
        errorf("failed to transmit, dev=%s, len=%zu", dev->name, len);
        return -1;
    }
    return 0;
}

// Catch some inputs from a network device
int net_input_handler(uint16_t type, const uint8_t* data, size_t len,
                      struct net_device* dev) {
    debugf("dev=%s, type=0x%04x, len=%zu", dev->name, type, len);
    debugdump(data, len);
    return 0;
}

// Run the protocol stack
int net_run(void) {
    struct net_device* dev;
    if (intr_run() == -1) {
        errorf("intr_run() failure");
        return -1;
    }
    debugf("open all devices...");
    for (dev = devices; dev; dev = dev->next) {
        net_device_open(dev);
    }
    debugf("running...");
    return 0;
}

// Shutdown the protocol stack
void net_shutdown(void) {
    struct net_device* dev;
    debugf("close all devices...");
    for (dev = devices; dev; dev = dev->next) {
        net_device_close(dev);
    }
    intr_shutdown();
    debugf("shutting down");
}

// Initialize the network stack
int net_init(void) {
    if (intr_init() == -1) {
        errorf("intr_init() failure");
        return -1;
    }
    infof("initialized");
    return 0;
}
