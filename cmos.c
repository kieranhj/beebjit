#include "cmos.h"

#include "state_sync.h"

#include "bbc_options.h"
#include "log.h"
#include "util.h"

#include <assert.h>
#include <string.h>

enum {
  k_cmos_port_b_address_strobe = 0x80,
  k_cmos_port_b_enable = 0x40,
};

enum {
  k_cmos_IC32_data = 4,
  k_cmos_IC32_read = 2,
};

/* From jsbeeb. */
static const uint8_t s_cmos_defaults[64] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0xc9, 0xff, 0xff, 0x12, 0x00,
    0x17, 0xca, 0x1e, 0x05, 0x00, 0x35, 0xa6, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

struct cmos_struct {
  int log;

  int enabled;
  int address_strobe;
  int data;
  int read;
  uint8_t addr;

  /* The 146818's 64 bytes: clock and status registers 0x00-0x0D, then RAM.
   * The clock is not emulated; its registers read back what was written.
   */
  uint8_t ram[k_cmos_size];
};

struct cmos_struct*
cmos_create(struct bbc_options* p_options) {
  struct cmos_struct* p_cmos = util_mallocz(sizeof(struct cmos_struct));

  p_cmos->log = util_has_option(p_options->p_log_flags, "cmos:all");

  p_cmos->enabled = 0;
  p_cmos->address_strobe = 0;
  p_cmos->data = 0;
  p_cmos->read = 0;
  (void) memcpy(p_cmos->ram, s_cmos_defaults, sizeof(p_cmos->ram));

  return p_cmos;
}

void cmos_destroy(struct cmos_struct* p_cmos) {
  util_free(p_cmos);
}

uint8_t
cmos_get_bus_value(struct cmos_struct* p_cmos) {
  uint8_t val = 0xFF;

  assert(p_cmos->addr < 64);

  if (p_cmos->enabled &&
      !p_cmos->address_strobe &&
      p_cmos->data &&
      p_cmos->read) {
    uint8_t addr = p_cmos->addr;
    val = p_cmos->ram[addr];

    if (p_cmos->log) {
      log_do_log(k_log_cmos,
                 k_log_info,
                 "address %.2X value %.2X on bus",
                 addr,
                 val);
    }
  }

  return val;
}

void
cmos_update_external_inputs(struct cmos_struct* p_cmos,
                            uint8_t port_b,
                            uint8_t port_a,
                            uint8_t IC32) {
  int enabled = !!(port_b & k_cmos_port_b_enable);
  int new_address_strobe = !!(port_b & k_cmos_port_b_address_strobe);
  int new_data = !!(IC32 & k_cmos_IC32_data);
  int new_read = !!(IC32 & k_cmos_IC32_read);

  p_cmos->enabled = enabled;
  if (!enabled) {
    return;
  }

  /* The address strobe high -> low edge triggers an address latch. */
  if (!new_address_strobe && p_cmos->address_strobe) {
    p_cmos->addr = (port_a & 0x3F);

    if (p_cmos->log) {
      log_do_log(k_log_cmos, k_log_info, "new address %.2X", p_cmos->addr);
    }
  }
  p_cmos->address_strobe = new_address_strobe;

  /* Data strobe high in write mode writes the byte on port A. MOS sets up
   * port A before raising the strobe; storing on every update while the
   * strobe is high keeps the last value, as the chip latches it on the
   * falling edge. Registers 0x0C and 0x0D are read-only.
   */
  if (new_data && !new_address_strobe && !new_read) {
    if (p_cmos->addr != 0x0C && p_cmos->addr != 0x0D) {
      p_cmos->ram[p_cmos->addr] = port_a;
      if (p_cmos->log) {
        log_do_log(k_log_cmos,
                   k_log_info,
                   "write address %.2X value %.2X",
                   p_cmos->addr,
                   port_a);
      }
    }
  }


  p_cmos->data = new_data;
  p_cmos->read = new_read;

  if (p_cmos->log) {
    log_do_log(k_log_cmos,
               k_log_info,
               "enabled %d address_strobe %d data %d read %d",
               p_cmos->enabled,
               p_cmos->address_strobe,
               p_cmos->data,
               p_cmos->read);
  }
}

uint8_t*
cmos_get_ram(struct cmos_struct* p_cmos) {
  return p_cmos->ram;
}

void
cmos_sync_state(struct cmos_struct* p, struct state_sync* p_sync) {
  state_sync_tag(p_sync, STATE_TAG('C', 'M', 'O', 'S'));
  STATE_SYNC(p_sync, p->enabled);
  STATE_SYNC(p_sync, p->address_strobe);
  STATE_SYNC(p_sync, p->data);
  STATE_SYNC(p_sync, p->read);
  STATE_SYNC(p_sync, p->addr);
  STATE_SYNC(p_sync, p->ram);
}
