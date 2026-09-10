#pragma once
#include <Arduino.h>

struct LwsFrame {
  uint8_t sender;
  uint8_t cmd;
  uint8_t len;
  uint8_t data[255];
};

static const uint8_t LWS_END1 = '&';
static const uint8_t LWS_END2 = '!';

inline void lws_send_frame(Stream &s, uint8_t sender, uint8_t cmd, const uint8_t* payload, uint8_t len) {
  s.write(sender);
  s.write(cmd);
  s.write(len);
  for (uint8_t i = 0; i < len; i++) s.write(payload[i]);
  s.write(LWS_END1);
  s.write(LWS_END2);
}

inline bool lws_parse_byte(uint8_t b, LwsFrame &out) {
  enum { ST_SENDER, ST_CMD, ST_LEN, ST_PAYLOAD, ST_END1, ST_END2 };
  static uint8_t st = ST_SENDER;
  static LwsFrame f;
  static uint8_t idx = 0;

  switch (st) {
    case ST_SENDER:
      f.sender = b; st = ST_CMD; break;
    case ST_CMD:
      f.cmd = b; st = ST_LEN; break;
    case ST_LEN:
      f.len = b; idx = 0;
      st = (f.len == 0) ? ST_END1 : ST_PAYLOAD;
      break;
    case ST_PAYLOAD:
      f.data[idx++] = b;
      if (idx >= f.len) st = ST_END1;
      break;
    case ST_END1:
      if (b == LWS_END1) st = ST_END2;
      else st = ST_SENDER;
      break;
    case ST_END2:
      st = ST_SENDER;
      if (b == LWS_END2) { out = f; return true; }
      break;
  }
  return false;
}

inline void lws_pack_i32_le(int32_t v, uint8_t out[4]) {
  out[0] = (uint8_t)(v & 0xFF);
  out[1] = (uint8_t)((v >> 8) & 0xFF);
  out[2] = (uint8_t)((v >> 16) & 0xFF);
  out[3] = (uint8_t)((v >> 24) & 0xFF);
}
inline int32_t lws_unpack_i32_le(const uint8_t in[4]) {
  return (int32_t)(
    ((uint32_t)in[0]) |
    ((uint32_t)in[1] << 8) |
    ((uint32_t)in[2] << 16) |
    ((uint32_t)in[3] << 24)
  );
}