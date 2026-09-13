#pragma once
#include <Arduino.h>

/*
  LWSv1 frame format:
    [SENDER][CMD][LEN][PAYLOAD ...][&][!]

  - SENDER : 1 char, id del nodo mittente
  - CMD    : 1 char, codice comando
  - LEN    : 0..255
  - PAYLOAD: LEN byte
  - &!     : terminatori fissi

  Vincolo: '&' e '!' non devono comparire nel payload.
  Se serve, vanno codificati applicativamente.
*/

struct LwsFrame {
  uint8_t sender;
  uint8_t cmd;
  uint8_t len;
  uint8_t data[255];
};

static const uint8_t LWS_END1 = '&';
static const uint8_t LWS_END2 = '!';

// ---------------- TX ----------------
inline void lws_send_frame(Stream &s,
                           uint8_t sender,
                           uint8_t cmd,
                           const uint8_t* payload,
                           uint8_t len) {
  if (!payload) len = 0;
  s.write(sender);
  s.write(cmd);
  s.write(len);
  for (uint8_t i = 0; i < len; i++) s.write(payload[i]);
  s.write(LWS_END1);
  s.write(LWS_END2);
}

// ---------------- RX parser ----------------
struct LwsParser {
  enum State : uint8_t {
    ST_SENDER = 0,
    ST_CMD,
    ST_LEN,
    ST_PAYLOAD,
    ST_END1,
    ST_END2
  };

  State   st  = ST_SENDER;
  LwsFrame f  = {};
  uint8_t idx = 0;

  void reset() {
    st  = ST_SENDER;
    idx = 0;
  }

  bool feed(uint8_t b, LwsFrame &out) {
    switch (st) {
      case ST_SENDER: f.sender = b; st = ST_CMD; break;

      case ST_CMD:    f.cmd = b;    st = ST_LEN; break;

      case ST_LEN:
        f.len = b;
        idx = 0;
        st = (f.len == 0) ? ST_END1 : ST_PAYLOAD;
        break;

      case ST_PAYLOAD:
        if (idx < sizeof(f.data)) {
          f.data[idx++] = b;
          if (idx >= f.len) st = ST_END1;
        } else {
          reset(); // frame malformato
        }
        break;

      case ST_END1:
        st = (b == LWS_END1) ? ST_END2 : ST_SENDER;
        break;

      case ST_END2:
        st = ST_SENDER;
        if (b == LWS_END2) { out = f; return true; }
        break;
    }
    return false;
  }
};

// Retrocompatibilità con il vecchio codice
inline bool lws_parse_byte(uint8_t b, LwsFrame &out) {
  static LwsParser p;
  return p.feed(b, out);
}

// ---------------- i32 LE helpers ----------------
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