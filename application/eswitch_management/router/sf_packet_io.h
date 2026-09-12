#ifndef ESWITCH_SF_PACKET_IO_H
#define ESWITCH_SF_PACKET_IO_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <doca_error.h>

#define ESWITCH_DEFAULT_SF_INTERFACE "enp3s0f0s0"

struct sf_packet_io {
  int fd;
  unsigned int ifindex;
  char interface_name[64];
  uint8_t mac[6];
  bool started;
};

/* Open and bind one raw Layer-2 socket to the actual Arm SF endpoint. */
doca_error_t sf_packet_io_start(const char *interface_name,
                                struct sf_packet_io *io);

/* Transmit one complete Ethernet frame. No IP stack or routing is involved. */
doca_error_t sf_packet_io_send(struct sf_packet_io *io,
                               const uint8_t *frame, size_t length);

void sf_packet_io_stop(struct sf_packet_io *io);

#endif /* ESWITCH_SF_PACKET_IO_H */
