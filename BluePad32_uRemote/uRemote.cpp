#include "uRemote.h"

static const uint8_t PREAMBLE[] = { '<', '$', 'M', 'U' };
static const uint8_t PREAMBLE_LEN = 4;
static const uint8_t MIN_FRAME = 5;
static const uint8_t BYTE_TIMEOUT_MS = 10;

int uRemoteArg::toInt() const {
  if (type == UREMOTE_TYPE_BOOL) {
    return toBool() ? 1 : 0;
  }

  char buf[UREMOTE_MAX_ARG_LEN + 1];
  uint8_t n = length < UREMOTE_MAX_ARG_LEN ? length : UREMOTE_MAX_ARG_LEN;
  memcpy(buf, data, n);
  buf[n] = '\0';
  return atoi(buf);
}

bool uRemoteArg::toBool() const {
  if (length == 0) {
    return false;
  }
  if (type == UREMOTE_TYPE_BOOL) {
    return data[0] != 0;
  }
  if (type == UREMOTE_TYPE_NUM) {
    return toInt() != 0;
  }

  String value = toString();
  value.toLowerCase();
  return value != "0" && value != "false";
}

String uRemoteArg::toString() const {
  String out;
  out.reserve(length);
  for (uint8_t i = 0; i < length; i++) {
    out += (char)data[i];
  }
  return out;
}

void uRemoteResponse::clear() {
  _status = UREMOTE_STATUS_OK;
  _count = 0;
}

bool uRemoteResponse::addRaw(uint8_t type, const uint8_t *data, uint8_t length) {
  if (_count >= UREMOTE_MAX_ARGS || length > UREMOTE_MAX_ARG_LEN) {
    return false;
  }

  uRemoteArg &arg = _args[_count++];
  arg.type = type;
  arg.length = length;
  if (length > 0 && data != nullptr) {
    memcpy(arg.data, data, length);
  }
  return true;
}

bool uRemoteResponse::addInt(long value) {
  char buf[16];
  ltoa(value, buf, 10);
  return addRaw(UREMOTE_TYPE_NUM, (const uint8_t *)buf, strlen(buf));
}

bool uRemoteResponse::addBool(bool value) {
  uint8_t b = value ? 1 : 0;
  return addRaw(UREMOTE_TYPE_BOOL, &b, 1);
}

bool uRemoteResponse::add(int value) {
  return addInt(value);
}

bool uRemoteResponse::add(unsigned int value) {
  return add((unsigned long)value);
}

bool uRemoteResponse::add(long value) {
  return addInt(value);
}

bool uRemoteResponse::add(unsigned long value) {
  char buf[16];
  ultoa(value, buf, 10);
  return addRaw(UREMOTE_TYPE_NUM, (const uint8_t *)buf, strlen(buf));
}

bool uRemoteResponse::add(bool value) {
  return addBool(value);
}

bool uRemoteResponse::addString(const String &value) {
  if (value.length() > UREMOTE_MAX_ARG_LEN) {
    return false;
  }
  return addRaw(UREMOTE_TYPE_STR, (const uint8_t *)value.c_str(), value.length());
}

bool uRemoteResponse::addBytes(const uint8_t *data, uint8_t length) {
  return addRaw(UREMOTE_TYPE_BYTES, data, length);
}

bool uRemoteResponse::add(const String &value) {
  return addString(value);
}

bool uRemoteResponse::add(const char *value) {
  return addString(String(value));
}

bool uRemoteResponse::add(const uint8_t *data, uint8_t length) {
  return addBytes(data, length);
}

bool uRemoteResponse::setError(const String &message) {
  clear();
  _status = UREMOTE_STATUS_ERR;
  return addString(message);
}

uRemote::uRemote(Stream &stream, uRemoteHandler handler) {
  begin(stream, handler);
}

void uRemote::begin(Stream &stream, uRemoteHandler handler) {
  _stream = &stream;
  _handler = handler;
}

void uRemote::setHandler(uRemoteHandler handler) {
  _handler = handler;
}

bool uRemote::process() {
  if (_stream == nullptr || _stream->available() <= 0) {
    return false;
  }

  uint8_t payload[UREMOTE_MAX_FRAME - PREAMBLE_LEN];
  uint8_t payloadLen = 0;
  if (!readFrame(payload, payloadLen, 0)) {
    return false;
  }

  uint8_t status = UREMOTE_STATUS_OK;
  String cmd;
  uRemoteArg args[UREMOTE_MAX_ARGS];
  uint8_t argc = 0;

  if (!decode(payload, payloadLen, status, cmd, args, argc)) {
    return false;
  }
  if (status != UREMOTE_STATUS_OK) {
    return false;
  }

  uRemoteResponse response;
  if (_handler != nullptr) {
    _handler(cmd, args, argc, response);
  } else {
    response.setError(cmd + "() handler not found remotely");
  }

  return writeFrame(response.status(), cmd.c_str(), response._args, response.count());
}

bool uRemote::send(const char *cmd, const uRemoteResponse &response) {
  return writeFrame(response.status(), cmd, response._args, response.count());
}

bool uRemote::call(const char *cmd,
                   const uRemoteResponse &request,
                   uRemoteResponse &reply,
                   uint16_t timeoutMs) {
  reply.clear();

  if (!writeFrame(UREMOTE_STATUS_OK, cmd, request._args, request.count())) {
    return false;
  }

  uint8_t payload[UREMOTE_MAX_FRAME - PREAMBLE_LEN];
  uint8_t payloadLen = 0;
  if (!readFrame(payload, payloadLen, timeoutMs)) {
    return false;
  }

  uint8_t status = UREMOTE_STATUS_OK;
  String replyCmd;
  uRemoteArg args[UREMOTE_MAX_ARGS];
  uint8_t argc = 0;

  if (!decode(payload, payloadLen, status, replyCmd, args, argc)) {
    return false;
  }
  if (replyCmd != cmd) {
    fail("unexpected reply: " + replyCmd);
    return false;
  }

  reply.clear();
  reply._status = status;
  for (uint8_t i = 0; i < argc; i++) {
    if (!reply.addRaw(args[i].type, args[i].data, args[i].length)) {
      fail("reply too large");
      return false;
    }
  }

  if (status != UREMOTE_STATUS_OK) {
    _lastError = argc > 0 ? args[0].toString() : "remote error";
    return false;
  }

  _lastError = "";
  return true;
}

bool uRemote::readFrame(uint8_t *payload, uint8_t &payloadLen, uint16_t timeoutMs) {
  if (_stream == nullptr) {
    fail("stream not set");
    return false;
  }

  uint32_t start = millis();
  uint32_t deadline = timeoutMs == 0 ? start : start + timeoutMs;

  uint8_t length = 0;
  if (timeoutMs == 0) {
    if (_stream->available() <= 0) {
      return false;
    }
    length = _stream->read();
  } else if (!waitByte(length, deadline)) {
    fail("timeout: no length byte");
    return false;
  }

  if (length < MIN_FRAME || length > UREMOTE_MAX_FRAME) {
    flushInput();
    fail("invalid frame length");
    return false;
  }

  uint8_t total[UREMOTE_MAX_FRAME];
  uint8_t received = 0;
  uint32_t byteDeadline = millis() + BYTE_TIMEOUT_MS;

  while (received < length) {
    uint8_t value = 0;
    uint32_t waitUntil = timeoutMs == 0 ? byteDeadline : min(deadline, byteDeadline);
    if (!waitByte(value, waitUntil)) {
      flushInput();
      fail("timeout: incomplete frame");
      return false;
    }
    total[received++] = value;
    byteDeadline = millis() + BYTE_TIMEOUT_MS;
  }

  for (uint8_t i = 0; i < PREAMBLE_LEN; i++) {
    if (total[i] != PREAMBLE[i]) {
      flushInput();
      fail("preamble mismatch");
      return false;
    }
  }

  payloadLen = length - PREAMBLE_LEN;
  memcpy(payload, total + PREAMBLE_LEN, payloadLen);
  _lastError = "";
  return true;
}

bool uRemote::decode(const uint8_t *payload,
                     uint8_t payloadLen,
                     uint8_t &status,
                     String &cmd,
                     uRemoteArg *args,
                     uint8_t &argc) {
  if (payloadLen < 1) {
    fail("decode error: empty payload");
    return false;
  }

  uint8_t hdr = payload[0];
  status = hdr >> 5;
  uint8_t cmdLen = hdr & 0x1F;
  if (cmdLen == 0 || cmdLen > UREMOTE_MAX_CMD_LEN || 1 + cmdLen > payloadLen) {
    fail("decode error: invalid command length");
    return false;
  }

  cmd = "";
  cmd.reserve(cmdLen);
  for (uint8_t i = 0; i < cmdLen; i++) {
    cmd += (char)payload[1 + i];
  }

  argc = 0;
  uint8_t p = 1 + cmdLen;
  while (p < payloadLen) {
    if (argc >= UREMOTE_MAX_ARGS || p + 2 > payloadLen) {
      fail("decode error: invalid argument header");
      return false;
    }

    uint8_t type = payload[p++];
    uint8_t len = payload[p++];
    if (len > UREMOTE_MAX_ARG_LEN || p + len > payloadLen) {
      fail("decode error: invalid argument length");
      return false;
    }

    uRemoteArg &arg = args[argc++];
    arg.type = type;
    arg.length = len;
    if (len > 0) {
      memcpy(arg.data, payload + p, len);
    }
    p += len;
  }

  _lastError = "";
  return true;
}

bool uRemote::writeFrame(uint8_t status,
                         const char *cmd,
                         const uRemoteArg *args,
                         uint8_t argc) {
  if (_stream == nullptr) {
    fail("stream not set");
    return false;
  }

  uint8_t cmdLen = strlen(cmd);
  if (cmdLen > UREMOTE_MAX_CMD_LEN) {
    fail("command name too long");
    return false;
  }

  uint16_t payloadLen = 1 + cmdLen;
  for (uint8_t i = 0; i < argc; i++) {
    payloadLen += 2 + args[i].length;
  }

  uint16_t frameLen = PREAMBLE_LEN + payloadLen;
  if (frameLen > UREMOTE_MAX_FRAME) {
    fail("frame too large");
    return false;
  }

  uint8_t frame[UREMOTE_MAX_FRAME + 1];
  uint16_t p = 0;
  frame[p++] = frameLen;
  memcpy(frame + p, PREAMBLE, PREAMBLE_LEN);
  p += PREAMBLE_LEN;
  frame[p++] = (status << 5) | cmdLen;
  memcpy(frame + p, cmd, cmdLen);
  p += cmdLen;

  for (uint8_t i = 0; i < argc; i++) {
    frame[p++] = args[i].type;
    frame[p++] = args[i].length;
    if (args[i].length > 0) {
      memcpy(frame + p, args[i].data, args[i].length);
      p += args[i].length;
    }
  }

  _stream->write(frame, p);
  _stream->flush();
  _lastError = "";
  return true;
}

bool uRemote::waitByte(uint8_t &value, uint32_t deadline) {
  while ((int32_t)(millis() - deadline) <= 0) {
    if (_stream->available() > 0) {
      value = _stream->read();
      return true;
    }
    delay(1);
  }
  return false;
}

void uRemote::flushInput() {
  if (_stream == nullptr) {
    return;
  }
  while (_stream->available() > 0) {
    _stream->read();
  }
}

void uRemote::fail(const String &message) {
  _lastError = message;
}
