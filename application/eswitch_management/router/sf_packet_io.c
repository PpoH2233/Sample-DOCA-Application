#include "sf_packet_io.h"

#include <arpa/inet.h>
#include <errno.h>
#include <net/if.h>
#include <netpacket/packet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <unistd.h>

#include <linux/if_ether.h>

doca_error_t sf_packet_io_start(const char *interface_name,
                                struct sf_packet_io *io) {
  struct sockaddr_ll local = {0};
  struct ifreq request = {0};
  size_t name_length;

  if (interface_name == NULL || io == NULL || io->started)
    return DOCA_ERROR_INVALID_VALUE;
  name_length = strlen(interface_name);
  if (name_length == 0 || name_length >= sizeof(io->interface_name) ||
      name_length >= IFNAMSIZ)
    return DOCA_ERROR_INVALID_VALUE;

  io->fd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
  if (io->fd < 0) {
    fprintf(stderr, "SF socket(%s) failed: %s\n", interface_name,
            strerror(errno));
    return DOCA_ERROR_DRIVER;
  }

  memcpy(request.ifr_name, interface_name, name_length + 1);
  if (ioctl(io->fd, SIOCGIFFLAGS, &request) < 0)
    goto driver_error;
  if ((request.ifr_flags & IFF_UP) == 0) {
    fprintf(stderr, "SF interface %s is DOWN; run: ip link set dev %s up\n",
            interface_name, interface_name);
    close(io->fd);
    *io = (struct sf_packet_io){.fd = -1};
    return DOCA_ERROR_BAD_STATE;
  }
  if ((request.ifr_flags & IFF_LOOPBACK) != 0) {
    fprintf(stderr, "SF interface %s resolves to a loopback device\n",
            interface_name);
    close(io->fd);
    *io = (struct sf_packet_io){.fd = -1};
    return DOCA_ERROR_INVALID_VALUE;
  }
  if (ioctl(io->fd, SIOCGIFHWADDR, &request) < 0)
    goto driver_error;

  io->ifindex = if_nametoindex(interface_name);
  if (io->ifindex == 0)
    goto driver_error;
  local.sll_family = AF_PACKET;
  local.sll_protocol = htons(ETH_P_ALL);
  local.sll_ifindex = (int)io->ifindex;
  if (bind(io->fd, (const struct sockaddr *)&local, sizeof(local)) < 0)
    goto driver_error;

  memcpy(io->interface_name, interface_name, name_length + 1);
  memcpy(io->mac, request.ifr_hwaddr.sa_data, sizeof(io->mac));
  io->started = true;
  printf("SF packet I/O ready: iface=%s ifindex=%u mac="
         "%02x:%02x:%02x:%02x:%02x:%02x\n",
         io->interface_name, io->ifindex, io->mac[0], io->mac[1], io->mac[2],
         io->mac[3], io->mac[4], io->mac[5]);
  return DOCA_SUCCESS;

driver_error:
  fprintf(stderr, "Failed to initialize SF interface %s: %s\n",
          interface_name, strerror(errno));
  close(io->fd);
  *io = (struct sf_packet_io){.fd = -1};
  return DOCA_ERROR_DRIVER;
}

doca_error_t sf_packet_io_send(struct sf_packet_io *io,
                               const uint8_t *frame, size_t length) {
  struct sockaddr_ll destination = {0};
  ssize_t written;

  if (io == NULL || !io->started || io->fd < 0 || frame == NULL ||
      length < ETH_HLEN)
    return DOCA_ERROR_INVALID_VALUE;
  destination.sll_family = AF_PACKET;
  destination.sll_protocol = (uint16_t)((uint16_t)frame[12] << 8 | frame[13]);
  destination.sll_protocol = htons(destination.sll_protocol);
  destination.sll_ifindex = (int)io->ifindex;
  destination.sll_halen = ETH_ALEN;
  memcpy(destination.sll_addr, frame, ETH_ALEN);

  written = sendto(io->fd, frame, length, 0,
                   (const struct sockaddr *)&destination,
                   sizeof(destination));
  if (written != (ssize_t)length) {
    fprintf(stderr, "SF send failed: iface=%s length=%zu result=%zd error=%s\n",
            io->interface_name, length, written, strerror(errno));
    return DOCA_ERROR_DRIVER;
  }
  return DOCA_SUCCESS;
}

doca_error_t sf_packet_io_send_context(struct sf_packet_io *io,
                                       const uint8_t *frame, size_t length,
                                       uint16_t context_tag) {
  uint8_t tagged[16];
  struct sockaddr_ll destination = {0};
  struct iovec vectors[2];
  struct msghdr message = {0};
  ssize_t written;

  if (io == NULL || !io->started || io->fd < 0 || frame == NULL || length < ETH_HLEN ||
      context_tag == 0 || context_tag > 4094 || length > SIZE_MAX - 4)
    return DOCA_ERROR_INVALID_VALUE;
  memcpy(tagged, frame, ETH_ALEN); /* Original VM destination. */
  memcpy(tagged + ETH_ALEN, io->mac, ETH_ALEN); /* SF-enforced source. */
  tagged[12] = 0x81;
  tagged[13] = 0x00;
  tagged[14] = (uint8_t)(context_tag >> 8);
  tagged[15] = (uint8_t)context_tag;
  /* sendmsg consumes both buffers synchronously. This removes the temporary
   * allocation and payload copy, not the kernel's socket copy. */
  vectors[0] = (struct iovec){.iov_base = tagged, .iov_len = sizeof(tagged)};
  vectors[1] = (struct iovec){.iov_base = (void *)(frame + 12),
                            .iov_len = length - 12};
  destination.sll_family = AF_PACKET;
  destination.sll_protocol = htons(ETH_P_8021Q);
  destination.sll_ifindex = (int)io->ifindex;
  destination.sll_halen = ETH_ALEN;
  memcpy(destination.sll_addr, frame, ETH_ALEN);
  message.msg_name = &destination;
  message.msg_namelen = sizeof(destination);
  message.msg_iov = vectors;
  message.msg_iovlen = 2;
  written = sendmsg(io->fd, &message, 0);
  if (written != (ssize_t)(length + 4))
    return DOCA_ERROR_DRIVER;
  return DOCA_SUCCESS;
}

void sf_packet_io_stop(struct sf_packet_io *io) {
  if (io == NULL)
    return;
  if (io->fd >= 0)
    close(io->fd);
  *io = (struct sf_packet_io){.fd = -1};
}
