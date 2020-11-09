int openpilot_live = 0;
int mdps_bus = -1;
bool mdps_spoof_active = false;

void mdps_spoof_speed(CAN_FIFOMailBox_TypeDef *to_fwd){
  bool mph_speed = GET_BYTE(to_fwd, 2) & 0x2;
  int enable_speed = mph_speed ? 38 : 60;
  int speed = (GET_BYTE(to_fwd, 1) | (GET_BYTE(to_fwd, 2) & 0x1) << 8) * 0.5;
  if (speed < enable_speed) {
    to_fwd->RDLR &= 0xFFFE00FF;
    to_fwd->RDLR |= (enable_speed * 2) << 8;
  }
};

int default_rx_hook(CAN_FIFOMailBox_TypeDef *to_push) {
  int bus = GET_BUS(to_push);
  int addr = GET_ADDR(to_push);
  if (addr == 524 && bus != mdps_bus && mdps_bus != 0) {
    mdps_bus = bus;
  }
  if (!mdps_spoof_active && mdps_bus == 2) {mdps_spoof_active = true;}
  // TODO: Openpilot send de/activation cmd
  // if (bus == 0 && addr == 0x2AB) {
  //   openpilot_live = 2;
  //   if (GET_BYTE(to_push, 0) & 0x7) {
  //     mdps_spoof_active = true;
  //   }
  //   else if (GET_BYTE(to_push, 0) & 0x3){
  //     mdps_spoof_active = false;
  //   }
  // }
  return true;
}

// *** no output safety mode ***

static void nooutput_init(int16_t param) {
  UNUSED(param);
  controls_allowed = false;
  relay_malfunction_reset();
}

static int nooutput_tx_hook(CAN_FIFOMailBox_TypeDef *to_send) {
  UNUSED(to_send);
  return false;
}

static int nooutput_tx_lin_hook(int lin_num, uint8_t *data, int len) {
  UNUSED(lin_num);
  UNUSED(data);
  UNUSED(len);
  return false;
}

static int default_fwd_hook(int bus_num, CAN_FIFOMailBox_TypeDef *to_fwd) {
  int addr = GET_ADDR(to_fwd);
  int bus_fwd = -1;

  if (bus_num == 0) {
    bus_fwd = 2;
    if (addr == 1265 && mdps_spoof_active) {
      mdps_spoof_speed(to_fwd);
    }
  }
  if (bus_num == 2) {
    bus_fwd = 0;
  }
  return bus_fwd;
}

const safety_hooks nooutput_hooks = {
  .init = nooutput_init,
  .rx = default_rx_hook,
  .tx = nooutput_tx_hook,
  .tx_lin = nooutput_tx_lin_hook,
  .fwd = default_fwd_hook,
};

// *** all output safety mode ***

static void alloutput_init(int16_t param) {
  UNUSED(param);
  controls_allowed = true;
  relay_malfunction_reset();
}

static int alloutput_tx_hook(CAN_FIFOMailBox_TypeDef *to_send) {
  int addr = GET_ADDR(to_send);
  if (addr == 0x2AA) {
    to_send->RDLR &= mdps_spoof_active ? 0x7 : 0x3;
    if (openpilot_live < 1) {
      // mdps_spoof_active = false;
    } else {
      mdps_spoof_active--;
    }
  }
  return true;
}

static int alloutput_tx_lin_hook(int lin_num, uint8_t *data, int len) {
  UNUSED(lin_num);
  UNUSED(data);
  UNUSED(len);
  return true;
}

const safety_hooks alloutput_hooks = {
  .init = alloutput_init,
  .rx = default_rx_hook,
  .tx = alloutput_tx_hook,
  .tx_lin = alloutput_tx_lin_hook,
  .fwd = default_fwd_hook,
};
