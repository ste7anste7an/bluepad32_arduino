#pragma once

#include <Arduino.h>

static const uint8_t UREMOTE_STATUS_OK = 0;
static const uint8_t UREMOTE_STATUS_ERR = 1;

static const uint8_t UREMOTE_TYPE_BYTES = 'A';
static const uint8_t UREMOTE_TYPE_BOOL = 'B';
static const uint8_t UREMOTE_TYPE_NUM = 'N';
static const uint8_t UREMOTE_TYPE_STR = 'S';

static const uint8_t UREMOTE_MAX_FRAME = 255;
static const uint8_t UREMOTE_MAX_CMD_LEN = 31;
static const uint8_t UREMOTE_MAX_ARGS = 8;
static const uint8_t UREMOTE_MAX_ARG_LEN = 128;

struct uRemoteArg {
  uint8_t type = UREMOTE_TYPE_BYTES;
  uint8_t length = 0;
  uint8_t data[UREMOTE_MAX_ARG_LEN];

  int toInt() const;
  bool toBool() const;
  String toString() const;

  operator int() const { return toInt(); }
  operator uint8_t() const { return (uint8_t)toInt(); }
  operator bool() const { return toBool(); }
  operator String() const { return toString(); }
};

class uRemoteResponse {
public:
  void clear();
  bool addInt(long value);
  bool addBool(bool value);
  bool addString(const String &value);
  bool addBytes(const uint8_t *data, uint8_t length);
  bool add(int value);
  bool add(unsigned int value);
  bool add(long value);
  bool add(unsigned long value);
  bool add(bool value);
  bool add(const String &value);
  bool add(const char *value);
  bool add(const uint8_t *data, uint8_t length);
  bool setError(const String &message);

  uint8_t status() const { return _status; }
  uint8_t count() const { return _count; }
  const uRemoteArg &arg(uint8_t index) const { return _args[index]; }

private:
  friend class uRemote;

  bool addRaw(uint8_t type, const uint8_t *data, uint8_t length);

  uint8_t _status = UREMOTE_STATUS_OK;
  uint8_t _count = 0;
  uRemoteArg _args[UREMOTE_MAX_ARGS];
};

typedef void (*uRemoteHandler)(const String &cmd,
                               const uRemoteArg *args,
                               uint8_t argc,
                               uRemoteResponse &response);

class uRemote {
public:
  uRemote(Stream &stream, uRemoteHandler handler = nullptr);

  void begin(Stream &stream, uRemoteHandler handler = nullptr);
  void setHandler(uRemoteHandler handler);

  bool process();
  bool send(const char *cmd, const uRemoteResponse &response);
  bool call(const char *cmd,
            const uRemoteResponse &request,
            uRemoteResponse &reply,
            uint16_t timeoutMs = 1000);

  const String &lastError() const { return _lastError; }

private:
  Stream *_stream = nullptr;
  uRemoteHandler _handler = nullptr;
  String _lastError;

  bool readFrame(uint8_t *payload, uint8_t &payloadLen, uint16_t timeoutMs);
  bool decode(const uint8_t *payload,
              uint8_t payloadLen,
              uint8_t &status,
              String &cmd,
              uRemoteArg *args,
              uint8_t &argc);
  bool writeFrame(uint8_t status,
                  const char *cmd,
                  const uRemoteArg *args,
                  uint8_t argc);
  bool waitByte(uint8_t &value, uint32_t deadline);
  void flushInput();
  void fail(const String &message);
};
