#ifndef ESW_ROUTER_PENDING_H
#define ESW_ROUTER_PENDING_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define ROUTER_PENDING_MAX_PACKETS 128U
#define ROUTER_PENDING_PER_NEIGHBOR 16U
#define ROUTER_PENDING_MAX_FRAME 10240U
#define ROUTER_PENDING_TIMEOUT_NS UINT64_C(3000000000)

struct router_pending_packet {
  bool used;
  uint16_t vr_id;
  uint16_t neighbor_interface_id;
  uint32_t next_hop_ip;
  uint16_t ingress_interface_id;
  uint16_t ingress_port;
  uint16_t expected_egress_interface;
  uint8_t link_hops;
  size_t length;
  uint64_t queued_ns;
  uint8_t *frame;
};

struct router_pending_queue {
  struct router_pending_packet entries[ROUTER_PENDING_MAX_PACKETS];
  size_t count;
  uint64_t enqueued;
  uint64_t replayed;
  uint64_t expired;
  uint64_t overflow;
  uint64_t allocation_failures;
};

/* Copy a pre-NAT Ethernet frame into a bounded unresolved-neighbor queue. */
bool router_pending_enqueue(struct router_pending_queue *queue,
                            uint16_t vr_id,
                            uint16_t neighbor_interface_id,
                            uint32_t next_hop_ip,
                            uint16_t ingress_interface_id,
                            uint16_t ingress_port,
                            uint8_t link_hops,
                            uint16_t expected_egress_interface,
                            const uint8_t *frame, size_t length,
                            uint64_t now_ns);

/* Transfer ownership of one matching frame to the caller. */
bool router_pending_take(struct router_pending_queue *queue,
                         uint16_t vr_id, uint16_t neighbor_interface_id,
                         uint32_t next_hop_ip,
                         struct router_pending_packet *packet);

/* Free expired frames and return the number discarded. */
size_t router_pending_expire(struct router_pending_queue *queue,
                             uint64_t now_ns);
void router_pending_destroy(struct router_pending_queue *queue);

#endif /* ESW_ROUTER_PENDING_H */
