#ifndef OPEN_PCI_DEVICE_H
#define OPEN_PCI_DEVICE_H

#include <doca_dev.h>

/* Open a DOCA device by PCI address. Returns NULL on failure. */
struct doca_dev *open_pci_device(const char *pci_address);

#endif /* OPEN_PCI_DEVICE_H */
