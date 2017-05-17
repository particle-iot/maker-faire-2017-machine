// Byte array encoding and decoding

#pragma once
#include <cstdint>

inline bool getBit(uint8_t *data, uint8_t bytePos, uint8_t bitPos) {
  return (data[bytePos] >> bitPos) & 0x1;
}
inline uint8_t getU8(uint8_t *data, uint8_t bytePos, uint8_t bitLength = 8, uint8_t bitPos = 0) {
  return (data[bytePos] >> bitPos) & ((1 << bitLength) - 1);
}
inline uint16_t getU16(uint8_t *data, uint8_t bytePos) {
  return (data[bytePos] << 8) | data[bytePos + 1];
}
inline uint32_t getU32(uint8_t *data, uint8_t bytePos) {
  return (data[bytePos] << 24) | (data[bytePos + 1] << 16) | (data[bytePos + 2] << 8) | data[bytePos + 3];
}
inline int8_t getS8(uint8_t *data, uint8_t bytePos, uint8_t bitLength = 8, uint8_t bitPos = 0) {
  return (int8_t)getU8(data, bytePos, bitLength, bitPos);
}
inline int16_t getS16(uint8_t *data, uint8_t bytePos) {
  return (int16_t)getU16(data, bytePos);
}
inline int32_t getS32(uint8_t *data, uint8_t bytePos) {
  return (int32_t)getU32(data, bytePos);
}
inline float getFloat(uint8_t *data, uint8_t bytePos) {
  union {
    uint32_t i;
    float f;
  } tmp;
  tmp.i = getU32(data, bytePos);
  return tmp.f;
}

inline void setBit(uint8_t *data, bool value, uint8_t bytePos, uint8_t bitPos) {
  uint8_t mask = (1 << bitPos);
  if (value) {
    data[bytePos] |= mask;
  } else {
    data[bytePos] &= ~mask;
  }
}
inline void setU8(uint8_t *data, uint8_t value, uint8_t bytePos, uint8_t bitLength = 8, uint8_t bitPos = 0) {
  uint8_t mask = (1 << bitLength) - 1;
  data[bytePos] &= ~(mask << bitPos);
  data[bytePos] |= (value & mask) << bitPos;
}

inline void setU16(uint8_t *data, uint16_t value, uint8_t bytePos) {
  data[bytePos] = (uint8_t)(value >> 8);
  data[bytePos + 1] = (uint8_t) value;
}
inline void setU32(uint8_t *data, uint32_t value, uint8_t bytePos) {
  data[bytePos] = (uint8_t)(value >> 24);
  data[bytePos + 1] = (uint8_t)(value >> 16);
  data[bytePos + 2] = (uint8_t)(value >> 8);
  data[bytePos + 3] = (uint8_t) value;
}
inline void setS8(uint8_t *data, int8_t value, uint8_t bytePos, uint8_t bitLength = 8, uint8_t bitPos = 0) {
  setU8(data, (uint8_t)value, bytePos, bitLength, bitPos);
}

inline void setS16(uint8_t *data, int16_t value, uint8_t bytePos) {
  setU16(data, (uint16_t)value, bytePos);
}
inline void setS32(uint8_t *data, int32_t value, uint8_t bytePos) {
  setU32(data, (uint32_t)value, bytePos);
}
inline void setFloat(uint8_t *data, float value, uint8_t bytePos) {
  union {
    uint32_t i;
    float f;
  } tmp;
  tmp.f = value;
  setU32(data, tmp.i, bytePos);
}
