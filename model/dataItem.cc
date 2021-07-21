

#include "dataItem.h"
#include <string.h>

namespace rhpman {

DataItem::DataItem() {
  dataID = 0;
  size = 0;
  bytes = NULL;
}

DataItem::DataItem(uint32_t dataSize, const uint8_t* payload) {
  static uint64_t autoID = 1;
  dataID = ++autoID;
  size = dataSize;
  bytes = (uint8_t*)malloc(size * sizeof(uint8_t));

  memcpy(bytes, payload, size);
}

DataItem::DataItem(uint64_t id, uint32_t dataSize, const uint8_t* payload) {
  dataID = id;
  size = dataSize;
  bytes = (uint8_t*)malloc(size * sizeof(uint8_t));

  memcpy(bytes, payload, size);
}

DataItem::~DataItem() {
  if (bytes != NULL) {
    free(bytes);
  }
}

uint64_t DataItem::getID() const { return dataID; }

uint32_t DataItem::getSize() const { return size; }

// Note this Must be freed by the caller, it is a copy of the data item
uint8_t* DataItem::getPayload() const {
  if (size == 0 || bytes == NULL) return NULL;

  uint8_t* tmp;
  tmp = (uint8_t*)malloc(size * sizeof(uint8_t));
  memcpy(tmp, bytes, size);
  return tmp;
}

}  // namespace rhpman
