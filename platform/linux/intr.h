int intr_request_irq(unsigned int irq,
                     int (*handler)(unsigned int irq, void *dev), int flags,
                     const char *name, void *dev);
int intr_run(void);
void intr_shutdown(void);
int intr_init(void);
int intr_raise_irq(unsigned int irq);