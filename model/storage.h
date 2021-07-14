

#ifndef __rhpman_storage_h
#define __rhpman_storage_h

#include <set>  // std::set
#include "ns3/uinteger.h"

#include "dataItem.h"

namespace rhpman {
using namespace ns3;

class Storage {
 private:
  /* data */

  uint32_t m_storageSpace;
  std::vector<DataItem*> m_storage;

 public:
  Storage();
  Storage(uint32_t capacity);
  ~Storage();

  void Init(uint32_t capacity);
  bool StoreItem(DataItem* data);
  DataItem* GetItem(uint64_t dataID);
  bool RemoveItem(uint64_t dataID);
  void ClearStorage();

  bool HasItem(uint64_t dataID);

  std::vector<DataItem*> GetAll();
  uint32_t GetFreeSpace();
};

};  // namespace rhpman

#endif
