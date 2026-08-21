#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <doca_error.h>

#include "open_pci_device.h"
#include "open_rep_vf_device.h"

static int parse_index(const char *text, uint32_t *index) {
  char *end;
  unsigned long value;

  errno = 0;
  value = strtoul(text, &end, 10);

  if (errno != 0 || *text == '\0' || *end != '\0' || value > UINT32_MAX)
    return 0;

  *index = (uint32_t)value;
  return 1;
}

int main(int argc, char **argv) {
  struct doca_dev *device;
  struct doca_dev_rep *representor;
  uint32_t host_index;
  uint32_t pf_index;
  uint32_t vf_index;
  doca_error_t result;
  int exit_status = EXIT_SUCCESS;

  if (argc != 5) {
    fprintf(stderr,
            "Usage: %s <DEVICE_PCI> <HOST_INDEX> <PF_INDEX> <VF_INDEX>\n",
            argv[0]);
    return EXIT_FAILURE;
  }

  if (!parse_index(argv[2], &host_index) ||
      !parse_index(argv[3], &pf_index) ||
      !parse_index(argv[4], &vf_index)) {
    fprintf(stderr,
            "HOST_INDEX, PF_INDEX and VF_INDEX must be unsigned integers\n");
    return EXIT_FAILURE;
  }

  device = open_pci_device(argv[1]);
  if (device == NULL) {
    fprintf(stderr, "Could not open DOCA device %s\n", argv[1]);
    return EXIT_FAILURE;
  }

  printf("Opened DOCA device %s successfully\n", argv[1]);

  representor = open_rep_vf_device(device, host_index, pf_index, vf_index);
  if (representor == NULL) {
    fprintf(stderr, "Could not open VF representor host=%u pf=%u vf=%u\n",
            host_index, pf_index, vf_index);
    (void)doca_dev_close(device);
    return EXIT_FAILURE;
  }

  printf("Opened VF representor host=%u pf=%u vf=%u successfully\n",
         host_index, pf_index, vf_index);

  /* Use the opened DOCA device and representor here. */

  result = doca_dev_rep_close(representor);
  if (result != DOCA_SUCCESS) {
    fprintf(stderr, "Failed to close representor: %s\n",
            doca_error_get_descr(result));
    exit_status = EXIT_FAILURE;
  }

  result = doca_dev_close(device);
  if (result != DOCA_SUCCESS) {
    fprintf(stderr, "Failed to close device: %s\n",
            doca_error_get_descr(result));
    exit_status = EXIT_FAILURE;
  }

  return exit_status;
}
