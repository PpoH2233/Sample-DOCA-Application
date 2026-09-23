#include "router_pending.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

int main(void) {
  struct router_pending_queue queue = {0};
  struct router_pending_packet packet = {0};
  uint8_t frame[60];

  memset(frame, 0xa5, sizeof(frame));
  assert(router_pending_enqueue(&queue, 1, 2, 0xa1f606feU, 3, 4, 0, 0,
                                frame, sizeof(frame), 1));
  frame[0] = 0;
  assert(queue.count == 1 && queue.enqueued == 1);
  assert(router_pending_take(&queue, 1, 2, 0xa1f606feU, &packet));
  assert(packet.length == sizeof(frame) && packet.frame[0] == 0xa5);
  assert(queue.count == 0 && queue.replayed == 1);
  free(packet.frame);

  for (unsigned i = 0; i < ROUTER_PENDING_PER_NEIGHBOR; i++)
    assert(router_pending_enqueue(&queue, 1, 2, 0xa1f606feU, 3, 4, 0, 0,
                                  frame, sizeof(frame), 10 + i));
  assert(!router_pending_enqueue(&queue, 1, 2, 0xa1f606feU, 3, 4, 0, 0,
                                 frame, sizeof(frame), 100));
  assert(queue.overflow == 1);
  assert(router_pending_expire(
             &queue, ROUTER_PENDING_TIMEOUT_NS + 1000) ==
         ROUTER_PENDING_PER_NEIGHBOR);
  assert(queue.count == 0 && queue.expired == ROUTER_PENDING_PER_NEIGHBOR);

  for (unsigned i = 0; i < ROUTER_PENDING_MAX_PACKETS; i++)
    assert(router_pending_enqueue(&queue, 1, 2, 0x0a000001U + i, 3, 4,
                                  0, 0, frame, sizeof(frame), 1));
  assert(!router_pending_enqueue(&queue, 1, 2, 0x0b000001U, 3, 4, 0, 0,
                                 frame, sizeof(frame), 1));
  assert(queue.count == ROUTER_PENDING_MAX_PACKETS && queue.overflow == 2);
  router_pending_destroy(&queue);
  puts("PASS: bounded pending-neighbor queue, ownership transfer and expiry");
  return 0;
}
