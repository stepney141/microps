#include "intr.h"

#include <signal.h>
#include <string.h>

#include "platform.h"
#include "util.h"

struct irq_entry {
    struct irq_entry *next;  // Pointer to the next entry in the list
    unsigned int irq;        // IRQ number
    int (*handler)(
        unsigned int irq,
        void *dev);  // IRQ handler (called when the IRQ is triggered)
    int flags;       // Flags
    char name[16];   // Name of the IRQ (used in debugging)
    void *dev;       // Device associated with the IRQ
};

/* NOTE: if you want to add/delete the entries after intr_run(), you need to
 * protect these lists with a mutex. */
static struct irq_entry *irqs;  // Pointer to the head of the ist of IRQ entries

static sigset_t sigmask;  // Signal set for signal mask

static pthread_t tid;              // Thread ID of the interrupt handler thread
static pthread_barrier_t barrier;  // Barrier for the sync between the threads

static void *intr_thread(void *arg) {
    int terminate = 0, sig, err;
    struct irq_entry *entry;

    debugf("start...");
    pthread_barrier_wait(&barrier);
    while (!terminate) {
        err = sigwait(&sigmask, &sig);
        if (err) {
            errorf("sigwait() %s", strerror(err));
            break;
        }
        switch (sig) {
            case SIGHUP:
                terminate = 1;
                break;
            default:
                for (entry = irqs; entry; entry = entry->next) {
                    if (entry->irq == (unsigned int)sig) {
                        debugf("irq=%d, name=%s", entry->irq, entry->name);
                        entry->handler(entry->irq, entry->dev);
                    }
                }
                break;
        }
    }
    debugf("terminated");
    return NULL;
}

int intr_request_irq(unsigned int irq,
                     int (*handler)(unsigned int irq, void *dev), int flags,
                     const char *name, void *dev) {
    struct irq_entry *entry;

    debugf("irq=%u, flags=%d, name=%s", irq, flags, name);
    for (entry = irqs; entry; entry = entry->next) {
        if (entry->irq == irq) {
            // Check if sharing the same IRQ number is allowed
            if (entry->flags ^ INTR_IRQ_SHARED || flags ^ INTR_IRQ_SHARED) {
                errorf("conflicts with already registered IRQs");
                return -1;
            }
        }
    }

    /* Add new entry to the IRQ list */

    // Allocate memory for the new entry
    entry = memory_alloc(sizeof(*entry));
    if (!entry) {
        errorf("memory_alloc() failure");
        return -1;
    }

    // Initialize the new entry
    entry->irq = irq;
    entry->handler = handler;
    entry->flags = flags;
    strncpy(entry->name, name, sizeof(entry->name) - 1);
    entry->dev = dev;

    // Add the new entry to the head of the list
    entry->next = irqs;
    irqs = entry;

    // Add the IRQ to the signal mask
    sigaddset(&sigmask, irq);
    debugf("registered: irq=%u, name=%s", irq, name);

    return 0;
}

int intr_run(void) {
    int err;

    // Set the signal mask
    err = pthread_sigmask(SIG_BLOCK, &sigmask, NULL);
    if (err) {
        errorf("pthread_sigmask() %s", strerror(err));
        return -1;
    }

    // Activate the interrupt handler thread
    err = pthread_create(&tid, NULL, intr_thread, NULL);
    if (err) {
        errorf("pthread_create() %s", strerror(err));
        return -1;
    }

    // Wait for the interrupt handler thread to be ready
    pthread_barrier_wait(&barrier);
    return 0;
}

void intr_shutdown(void) {
    if (pthread_equal(tid, pthread_self()) != 0) {
        /* Thread not created. */
        return;
    }
    pthread_kill(tid,
                 SIGHUP);  // Send SIGHUP signal to the interrupt handler thread
    pthread_join(tid,
                 NULL);  // Wait for the interrupt handler thread to terminate
}

int intr_init(void) {
    tid = pthread_self();  // Set the thread ID of the main thread
    pthread_barrier_init(&barrier, NULL, 2);  // Initialize the barrier
    sigemptyset(&sigmask);                    // Initialize the signal mask
    sigaddset(&sigmask, SIGHUP);
    return 0;
}

int intr_raise_irq(unsigned int irq) { return pthread_kill(tid, (int)irq); }
