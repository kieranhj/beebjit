#ifndef BEEBJIT_STATE_SYNC_H
#define BEEBJIT_STATE_SYNC_H

/* Save state plumbing. Each module has one *_sync_state() function that walks
 * its state in a fixed order. The same walk saves (copies fields out), loads
 * (copies fields in) or measures (counts bytes), so save and load cannot drift
 * apart. Fields are fixed-size types and the targets are all little-endian, so
 * a state taken on one host loads on another.
 */

#include <stddef.h>
#include <stdint.h>
#include <string.h>

struct state_sync {
  uint8_t* p_buf;
  size_t size;
  size_t pos;
  int is_load;
  int is_error;
};

static inline void
state_sync_init(struct state_sync* p_sync,
                void* p_buf,
                size_t size,
                int is_load) {
  p_sync->p_buf = (uint8_t*) p_buf;
  p_sync->size = (p_buf == NULL) ? SIZE_MAX : size;
  p_sync->pos = 0;
  p_sync->is_load = is_load;
  p_sync->is_error = 0;
}

static inline int
state_sync_is_load(struct state_sync* p_sync) {
  return p_sync->is_load;
}

static inline void
state_sync_bytes(struct state_sync* p_sync, void* p_data, size_t len) {
  if (p_sync->is_error) {
    return;
  }
  if (len > (p_sync->size - p_sync->pos)) {
    p_sync->is_error = 1;
    return;
  }
  if (p_sync->p_buf != NULL) {
    if (p_sync->is_load) {
      (void) memcpy(p_data, (p_sync->p_buf + p_sync->pos), len);
    } else {
      (void) memcpy((p_sync->p_buf + p_sync->pos), p_data, len);
    }
  }
  p_sync->pos += len;
}

/* A marker between sections, checked on load to catch a misaligned walk. */
static inline void
state_sync_tag(struct state_sync* p_sync, uint32_t tag) {
  uint32_t val = tag;
  state_sync_bytes(p_sync, &val, sizeof(val));
  if (p_sync->is_load && (val != tag)) {
    p_sync->is_error = 1;
  }
}

#define STATE_SYNC(p_sync, field) \
    state_sync_bytes((p_sync), &(field), sizeof(field))

#define STATE_TAG(a, b, c, d) \
    ((uint32_t) (a) | ((uint32_t) (b) << 8) | ((uint32_t) (c) << 16) | \
     ((uint32_t) (d) << 24))

#endif /* BEEBJIT_STATE_SYNC_H */
