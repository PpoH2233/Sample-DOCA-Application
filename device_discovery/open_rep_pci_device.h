#ifndef OPEN_REP_PCI_DEVICE_H
#define OPEN_REP_PCI_DEVICE_H

#include <doca_dev.h>

/* Open a network representor by PCI address. Returns NULL on failure. */
struct doca_dev_rep *open_rep_pci_device(struct doca_dev *parent_device,
                                         const char *pci_address);

#endif /* OPEN_REP_PCI_DEVICE_H */
