#ifndef OPEN_REP_VF_DEVICE_H
#define OPEN_REP_VF_DEVICE_H

#include <stdint.h>

#include <doca_dev.h>

/*
 * Open a network VF representor belonging to parent_device.
 *
 * The representor must match all three indexes. Returns NULL when no matching
 * representor is found or when discovery/opening fails.
 */
struct doca_dev_rep *open_rep_vf_device(struct doca_dev *parent_device,
                                        uint32_t host_index,
                                        uint32_t pf_index,
                                        uint32_t vf_index);

#endif /* OPEN_REP_VF_DEVICE_H */
