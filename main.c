#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef uint32_t packet_typ_t;
typedef uint16_t datasize_typ_t;

packet_typ_t *bit_stuffing(packet_typ_t *data, datasize_typ_t dataSize);
packet_typ_t *nrzi(packet_typ_t *data, datasize_typ_t dataSize);
uint32_t crc(packet_typ_t *data, datasize_typ_t dataSize, uint32_t pollynomial);
uint8_t logOf2(uint32_t n);
void transfer(packet_typ_t *data, datasize_typ_t dataSize, uint8_t packetSize);

int main(void) {
  packet_typ_t rData[20] = {0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF};
  packet_typ_t *data = bit_stuffing(rData, 640);
  datasize_typ_t dataSize = (datasize_typ_t)data[0];
  data = nrzi(data + 1, dataSize);
  transfer(data, dataSize, 32);
  uint16_t crcData = (uint16_t)crc(data, dataSize, 0x8005);
  printf("crc: %x\n", crcData);
  transfer((packet_typ_t *)(&crcData), 16, 16);
  return 0;
}

packet_typ_t *bit_stuffing(packet_typ_t *data, datasize_typ_t dataSize) {
  uint8_t buffer = 0x00;
  uint8_t insertedBits = 0;
  packet_typ_t *dataBuffer = (packet_typ_t *)malloc((size_t)(ceil((datasize_typ_t)(dataSize / (8 * sizeof(packet_typ_t)))) + 1) * sizeof(packet_typ_t));

  for (datasize_typ_t bIdx = 0; bIdx < dataSize; bIdx++) {

    uint8_t bit = (*(data + (bIdx / (8 * sizeof(packet_typ_t)))) >> ((8 * sizeof(packet_typ_t) - 1) - (bIdx % (8 * sizeof(packet_typ_t))))) & 0x01;
    buffer = (uint8_t)(packet_typ_t)(buffer << 1) | (bit);

    packet_typ_t *dataPointer = (dataBuffer + ((datasize_typ_t)(bIdx + insertedBits) / (8 * sizeof(packet_typ_t))) + 1);

    if ((buffer & 0x7F) == 0x7F) {
      buffer &= 0xFE;
      *dataPointer = (*dataPointer << 1) | (0x00);
      buffer = (uint8_t)(packet_typ_t)(buffer << 1) | (0x01);
      dataBuffer = (packet_typ_t *)realloc(dataBuffer, ((size_t)ceil((uint8_t)((datasize_typ_t)(dataSize + (++insertedBits)) / (8 * sizeof(packet_typ_t)))) + 1) * sizeof(packet_typ_t));
      dataPointer = (dataBuffer + ((datasize_typ_t)(bIdx + insertedBits) / (8 * sizeof(packet_typ_t))) + 1);
    }
    *dataPointer = (packet_typ_t)((*dataPointer << 1) | (buffer & 0x01));
  }

  dataBuffer[0] = dataSize + (packet_typ_t)insertedBits;
  return dataBuffer;
}

packet_typ_t *nrzi(packet_typ_t *data, datasize_typ_t dataSize) {
  packet_typ_t *dataBuffer = (packet_typ_t *)malloc((size_t)(ceil((datasize_typ_t)(dataSize / (8 * sizeof(packet_typ_t))))) * sizeof(packet_typ_t));
  uint8_t coparator = 0x01;

  for (datasize_typ_t bIdx = 0; bIdx < (dataSize - dataSize % 8); bIdx++) {
    uint8_t bit = (*(data + (bIdx / (8 * sizeof(packet_typ_t)))) >> ((8 * sizeof(packet_typ_t) - 1) - (bIdx % (8 * sizeof(packet_typ_t))))) & 0x01;

    if (bit == 0x00)
      coparator = (~coparator & 0x01);

    packet_typ_t *dataPointer = (dataBuffer + (bIdx / (8 * sizeof(packet_typ_t))));
    *dataPointer = (packet_typ_t)((*dataPointer << 1) | coparator);
  }
  for (datasize_typ_t bIdx = 0; bIdx < dataSize % (8 * sizeof(packet_typ_t)); bIdx++) {
    uint8_t bit = (*(data + (uint8_t)ceil((datasize_typ_t)(dataSize / (8 * sizeof(packet_typ_t))))) >> ((dataSize % (8 * (datasize_typ_t)sizeof(packet_typ_t))) - (bIdx + 1))) & 0x01;

    if (bit == 0x00)
      coparator = (~coparator & 0x01);

    packet_typ_t *dataPointer = (dataBuffer + (bIdx / (8 * sizeof(packet_typ_t))) + 1);
    *dataPointer = (packet_typ_t)((*dataPointer << 1) | coparator);
  }
  return dataBuffer;
}

// up to crc32
uint32_t crc(packet_typ_t *data, datasize_typ_t dataSize, uint32_t pollynomial) {
  uint32_t buffer = 0x00;
  uint32_t crc = 0x00;
  uint8_t highesExp = logOf2(pollynomial);

  for (datasize_typ_t bIdx = 0; bIdx < (dataSize - dataSize % 8); bIdx++) {
    uint8_t bit = (*(data + (bIdx / (8 * sizeof(uint32_t)))) >> ((8 * sizeof(packet_typ_t) - 1) - (bIdx % (8 * sizeof(uint32_t))))) & 0x01;
    buffer = (uint32_t)(buffer << 1) | (bit);

    if ((buffer & (uint32_t)(1 << highesExp)) == (uint32_t)(1 << highesExp)) {
      buffer = buffer ^ pollynomial;
    }
  }

  for (uint8_t bIdx = 0; bIdx < dataSize % (8 * sizeof(uint32_t)); bIdx++) {
    uint8_t bit = (*(data + (uint8_t)ceil((datasize_typ_t)(dataSize / (8 * sizeof(uint32_t))))) >> ((dataSize % (8 * (datasize_typ_t)sizeof(uint32_t))) - (bIdx + 1))) & 0x01;
    buffer = (uint32_t)(buffer << 1) | (bit);

    if ((buffer & (uint32_t)(1 << highesExp)) == (uint32_t)(1 << highesExp)) {
      buffer = buffer ^ pollynomial;
    }
  }

  for (uint8_t i = 0; i < highesExp; i++) {
    buffer = (uint32_t)(buffer << 1);
    crc = (uint32_t)(crc << 1);
    if ((buffer & (uint32_t)(1 << highesExp)) == (uint32_t)(1 << highesExp)) {
      buffer = buffer ^ pollynomial;
      crc = crc ^ pollynomial;
    }
  }
  crc = (crc & (0xFFFFFFFF >> (32 - highesExp)));
  return crc;
}

uint8_t logOf2(uint32_t n) {
  uint8_t counter = 0;
  while (n > 1) {
    n = (n >> 1);
    counter++;
  }
  return counter;
}

void transfer(packet_typ_t *data, datasize_typ_t dataSize, uint8_t packetSize) {
  for (datasize_typ_t i = 0; i < (dataSize - dataSize % 8); i++) {
    uint8_t bit = (*(data + (i / packetSize)) >> ((packetSize - 1) - (i % packetSize))) & 0x01;
    printf("%u", bit);
  }
  for (uint8_t i = 0; i < dataSize % packetSize; i++) {
    uint8_t bit = (*(data + (datasize_typ_t)ceil((datasize_typ_t)(dataSize / packetSize))) >> ((dataSize % packetSize) - (i + 1))) & 0x01;
    printf("%u", bit);
  }
  printf("\n");
}

// void transfer(packet_typ_t *data, datasize_typ_t dataSize) {
//   for (datasize_typ_t i = 0; i < (dataSize - dataSize % 8); i++) {
//     uint8_t bit = (*(data + (i / (8 * sizeof(packet_typ_t)))) >> ((8 * sizeof(packet_typ_t) - 1) - (i % (8 * sizeof(packet_typ_t))))) & 0x01;
//     printf("%u", bit);
//   }
//   for (uint8_t i = 0; i < dataSize % (8 * sizeof(packet_typ_t)); i++) {
//     uint8_t bit = (*(data + (datasize_typ_t)ceil((datasize_typ_t)(dataSize / (8 * sizeof(packet_typ_t))))) >> ((dataSize % (8 * (datasize_typ_t)sizeof(packet_typ_t))) - (i + 1))) & 0x01;
//     printf("secound: %u", bit);
//   }
//   printf("\n");
// }
