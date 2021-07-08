

#ifndef __rhpman_data_h
#define __rhpman_data_h


#include "ns3/uinteger.h"


namespace rhpman {

class DataItem {
 private:
  /* data */
  uint64_t dataID;
  uint8_t* bytes;
  uint32_t size;

 public:
  DataItem();
  DataItem(uint64_t id, uint32_t size, const uint8_t* payload);
  ~DataItem();

  uint64_t getID();
  uint32_t getSize();
  uint8_t* getPayload();
};

}  // namespace rhpman

#endif
