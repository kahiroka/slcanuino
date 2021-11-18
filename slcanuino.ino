#include "src/canbus/mcp2515_defs.h"
#include "src/canbus/mcp2515.h"
#include "src/canbus/defaults.h"
#include "src/canbus/global.h"
#include "src/canbus/Canbus.h"

#define LED_OPEN 7
#define LED_ERR 8
#define CMD_LEN (sizeof("T12345678811223344556677881234\r")+1)

int g_can_speed = CANSPEED_500; // default: 500k
int g_ts_en = 0;

// the setup function runs once when you press reset or power the board
void setup() {
  pinMode(LED_OPEN, OUTPUT);
  pinMode(LED_ERR, OUTPUT);
  Serial.begin(1000000); // select from 115200,500000,1000000,2000000
  if (Canbus.init(g_can_speed)) {
    digitalWrite(LED_ERR, LOW);
  } else {
    digitalWrite(LED_ERR, HIGH);
  }
}

int b2ahex(char *p, uint8_t s, uint8_t n, void *v)
{
  const char *hex = "0123456789ABCDEF";

  if (s == 1) {
    uint8_t *tmp = (uint8_t *)v;
    for (int i=0; i<n; i++) {
      *p++ = hex[tmp[i] & 0x0f];
    }
  } else if (s == 2) {
    uint8_t *tmp = (uint8_t *)v;
    for (int i=0; i<n; i++) {
      *p++ = hex[(tmp[i] >> 4) & 0x0f];
      *p++ = hex[tmp[i] & 0x0f];
    }
  } else if (s == 3) {
    uint16_t *tmp = (uint16_t *)v;
    for (int i=0; i<n; i++) {
      *p++ = hex[(tmp[i] >> 8) & 0x0f];
      *p++ = hex[(tmp[i] >> 4) & 0x0f];
      *p++ = hex[tmp[i] & 0x0f];
    }
  } else if (s == 4) {
    uint16_t *tmp = (uint16_t *)v;
    for (int i=0; i<n; i++) {
      *p++ = hex[(tmp[i] >> 12) & 0x0f];
      *p++ = hex[(tmp[i] >> 8) & 0x0f];
      *p++ = hex[(tmp[i] >> 4) & 0x0f];
      *p++ = hex[tmp[i] & 0x0f];
    }
  }
  return s*n;
}

int a2bhex_sub(char a)
{
  int val = 0;

  if ('0' <= a && a <= '9') {
    val = a - '0';
  } else if ('A' <= a && a <= 'F') {
    val = a - 'A' + 10;
  } else if ('a' <= a && a <= 'f') {
    val = a - 'a' + 10;
  }
  return val;
}

int a2bhex(char *p, uint8_t s, uint8_t n, void *v)
{
  int i, j;
  char buf[4+1];
  int val;

  if (s == 1 || s == 2)  {
    uint8_t *tmp = (uint8_t *)v;
    for (i=0; i<n; i++) {
      val = 0;
      for (j=0; j<s; j++) {
        val = (val << 4) | a2bhex_sub(*p++);
      }
      *tmp++ = val;
    }
  } else if (s == 3 || s == 4) {
    uint16_t *tmp = (uint16_t *)v;
    for (i=0; i<n; i++) {
      val = 0;
      for (j=0; j<s; j++) {
        val = (val << 4) | a2bhex_sub(*p++);
      }
      *tmp++ = val;
    }
  }
  return n;
}

// transfer messages from CAN bus to host
void xfer_can2tty()
{
  tCAN msg;
  char buf[CMD_LEN];
  int i;
  static uint16_t ts = 0;
  char *p;
  uint8_t len;

  while (Canbus.message_rx(&msg)) {
    p = buf;
    if (msg.header.ide) {
      if (msg.header.rtr) {
        *p++ = 'R';
      } else {
        *p++ = 'T';
      }
      p += b2ahex(p, 4, 1, &msg.id);
      p += b2ahex(p, 4, 1, &msg.ide);
      len = msg.header.length % 10;
      p += b2ahex(p, 1, 1, &len);
    } else {
      if (msg.header.rtr) {
        *p++ = 'r';
      } else {
        *p++ = 't';
      }
      p += b2ahex(p, 3, 1, &msg.id);
      len = msg.header.length;
      p += b2ahex(p, 1, 1, &len);
    }

    p += b2ahex(p, 2, msg.header.length, msg.data);

    // insert timestamp if needed
    if (g_ts_en) {
      p += b2ahex(p, 4, 1, &ts); // up to 60,000ms
      ts++;
    }

    *p++ = '\r';
    *p++ = '\0';
    Serial.print(buf);
  }
}

void slcan_ack()
{
  Serial.write('\r'); // ACK
}

void slcan_nack()
{
  Serial.write('\a'); // NACK
}

void send_canmsg(char *buf)
{
  tCAN msg;
  int len = strlen(buf) - 1;
  uint16_t id[2];
  uint8_t hlen;
  int is_eff = buf[0] & 0x20 ? 0 : 1;
  int is_rtr = buf[0] & 0x02 ? 1 : 0;

  if (!is_eff && len >= 4) { // SFF
    a2bhex(&buf[1], 3, 1, id);
    msg.id = id[0];
    msg.header.rtr = is_rtr;
    msg.header.ide = 0;
    a2bhex(&buf[4], 1, 1, &hlen);
    msg.header.length = hlen;
    if (len - 4 - 1 == msg.header.length * 2) {
      a2bhex(&buf[5], 2, msg.header.length, msg.data);
      while (!Canbus.message_tx(&msg)) ;
    }

  } else if (is_eff && len >= 9) { // EFF
    a2bhex(&buf[1], 4, 2, id);
    msg.id = id[0] & 0x1fff;
    msg.ide = id[1];
    msg.header.rtr = is_rtr;
    msg.header.ide = 1;
    a2bhex(&buf[9], 1, 1, &hlen);
    msg.header.length = hlen;
    if (len - 9 - 1 == msg.header.length * 2) {
      a2bhex(&buf[10], 2, msg.header.length, msg.data);
      while (!Canbus.message_tx(&msg)) ;
    }
  }
}

void pars_slcancmd(char *buf)
{
  switch (buf[0]) {
    // common commands
    case 'O': // open channel
      digitalWrite(LED_OPEN, HIGH);
      if (Canbus.init(g_can_speed)) {
        digitalWrite(LED_ERR, LOW);
      } else {
        digitalWrite(LED_ERR, HIGH);
      }
      slcan_ack();
      break;
    case 'C': // close channel
      digitalWrite(LED_OPEN, LOW);
      digitalWrite(LED_ERR, LOW);
      slcan_ack();
      break;
    case 't': // SFF
    case 'T': // EFF
    case 'r': // RTR/SFF
    case 'R': // RTR/EFF
      send_canmsg(buf);
      slcan_ack();
      break;
    case 'Z': // turn timestamp on/off
      if (buf[1] == '0') {
        g_ts_en = 0;
      } else if (buf[1] == '1') {
        g_ts_en = 1;
      } else {
        slcan_nack();
      }
      slcan_ack();
      break;
    case 'M': // acceptance mask
      slcan_ack();
      break;
    case 'm': // acceptance value
      slcan_ack();
      break;

    // non-standard commands
    case 'S': // setup CAN bit-rates
      switch (buf[1]) {
        case '0': // 10k
        case '1': // 20k
        case '2': // 50k
          slcan_nack();
          break;
        case '3': // 100k
          g_can_speed = CANSPEED_100;
          slcan_ack();
          break;
        case '4': // 125k
          g_can_speed = CANSPEED_125;
          slcan_ack();
          break;
        case '5': // 250k
          g_can_speed = CANSPEED_250;
          slcan_ack();
          break;
        case '6': // 500k
          g_can_speed = CANSPEED_500;
          slcan_ack();
          break;
        case '7': // 800k
          slcan_nack();
          break;
        case '8': // 1000k
          g_can_speed = CANSPEED_1000;
          slcan_ack();
          break;
        default:
          slcan_nack();
          break;
      }
      break;
    case 's': // directly set bitrate register of mcp2515
      slcan_nack();
      break;
    case 'F': // status flag
      Serial.print("F12");
      slcan_ack();
      break;
    case 'V': // hw/sw version
      Serial.print("V1234");
      slcan_ack();
      break;
    case 'N': // serial number
      Serial.print("N1234");
      slcan_ack();
      break;
    default: // unknown command
      slcan_nack();
      break;
  }
}

// transfer messages from host to CAN bus
void xfer_tty2can()
{
  int length;
  static char cmdbuf[CMD_LEN];
  static int cmdidx = 0;

  if ((length = Serial.available()) > 0) {
    for (int i = 0; i < length; i++) {
      char val = Serial.read();
      cmdbuf[cmdidx++] = val;

      if (cmdidx == CMD_LEN) { // command is too long
        slcan_nack();
        cmdidx = 0;
      } else if (val == '\r') { // end of command
        cmdbuf[cmdidx] = '\0';
        pars_slcancmd(cmdbuf);
        cmdidx = 0;
      }
    }
  }
}

// the loop function runs over and over again forever
void loop() {
  xfer_can2tty();
  xfer_tty2can();
}
