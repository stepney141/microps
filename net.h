#include <stddef.h>
#include <stdint.h>

#ifndef IFNAMSIZ
#define IFNAMSIZ 16
#endif

#define NET_DEVICE_TYPE_DUMMY 0x0000
#define NET_DEVICE_TYPE_LOOPBACK 0x0001
#define NET_DEVICE_TYPE_ETHERNET 0x0002

#define NET_DEVICE_FLAG_UP 0x0001
#define NET_DEVICE_FLAG_LOOPBACK 0x0010
#define NET_DEVICE_FLAG_BROADCAST 0x0020
#define NET_DEVICE_FLAG_P2P 0x0040
#define NET_DEVICE_FLAG_NEED_ARP 0x0100

#define NET_DEVICE_ADDR_LEN 16

#define NET_DEVICE_IS_UP(x) ((x)->flags & NET_DEVICE_FLAG_UP)
#define NET_DEVICE_STATE(x) (NET_DEVICE_IS_UP(x) ? "up" : "down")

struct net_device {
    struct net_device *next;  // Pointer to the next device
    unsigned int index;
    char name[32];
    uint16_t type;  // device type
    uint16_t mtu;   // maximum transmission unit
    uint16_t flags;
    uint16_t hlen;  // header length
    uint16_t alen;  // address length
    uint8_t addr[NET_DEVICE_ADDR_LEN];
    union  //  hardware address of the device
    {
        uint8_t peer[NET_DEVICE_ADDR_LEN];
        uint8_t broadcast[NET_DEVICE_ADDR_LEN];
    };
    struct net_device_ops *ops;  // Pointer to the device driver operations
    void *priv;  // Pointer to the private data of the device driver
};

// device driver operations
struct net_device_ops {
    int (*open)(struct net_device *dev);   // optional
    int (*close)(struct net_device *dev);  // optional
    int (*transmit)(struct net_device *dev, uint16_t type, const uint8_t *data,
                    size_t len,
                    const void *dst);  // Transmit a packet (required)
};

struct net_device *net_device_alloc(void);

int net_device_register(struct net_device *dev);
int net_device_output(struct net_device *dev, uint16_t type,
                      const uint8_t *data, size_t len, const void *dst);
int net_device_input(uint16_t type, const uint8_t *data, size_t len,
                     struct net_device *dev);
int net_run(void);
void net_shutdown(void);
int net_init(void);