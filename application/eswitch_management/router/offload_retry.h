#ifndef ESW_OFFLOAD_RETRY_H
#define ESW_OFFLOAD_RETRY_H

#include <stdbool.h>
#include <stdint.h>

/* Shared resource pressure should suppress all promotions, not just one
 * session. Existing hardware hits and the Arm dataplane are unaffected. */
struct offload_retry {
  uint64_t until_ns;
  uint32_t delay_ms;
};

static inline bool offload_retry_ready(const struct offload_retry *retry,
                                       uint64_t now_ns) {
  return now_ns >= retry->until_ns;
}

static inline void offload_retry_failed(struct offload_retry *retry,
                                        uint64_t now_ns) {
  retry->delay_ms = retry->delay_ms == 0 ? 250 :
      (retry->delay_ms >= 4000 ? 8000 : retry->delay_ms * 2);
  retry->until_ns = now_ns + (uint64_t)retry->delay_ms * UINT64_C(1000000);
}

static inline void offload_retry_reset(struct offload_retry *retry) {
  *retry = (struct offload_retry){0};
}

#endif
