

#include "storage.h"

namespace rhpman {
using namespace ns3;

Storage::Storage() {
  m_storageSpace = 0;
  m_storage.resize(0);
}

Storage::Storage(uint32_t capacity) { Init(capacity); }

Storage::~Storage() { ClearStorage(); }

// this will do the actual storage. will store the item not a copy
// true if there was space false otherwise
bool Storage::StoreItem(DataItem* data) {
  bool saved = false;
  for (uint32_t i = 0; i < m_storageSpace; i++) {
    if (m_storage[i] == NULL) {
      m_storage[i] = data;
      saved = true;
      break;
    }
  }

  return saved;
}

void Storage::Init(uint32_t capacity) {
  m_storageSpace = capacity;
  m_storage.resize(capacity);
  for (uint32_t i = 0; i < capacity; i++) {
    m_storage[i] = NULL;
  }
}

// this will return a pointer to the data item if it is found or NULL if it is not
DataItem* Storage::GetItem(uint64_t dataID) {
  DataItem* found = NULL;

  for (uint32_t i = 0; i < m_storageSpace; i++) {
    if (m_storage[i] != NULL && m_storage[i]->getID() == dataID) {
      found = m_storage[i];
      break;
    }
  }

  return found;
}

bool Storage::HasItem(uint64_t dataID) {
  for (uint32_t i = 0; i < m_storageSpace; i++) {
    if (m_storage[i] != NULL && m_storage[i]->getID() == dataID) {
      return true;
    }
  }
  return false;
}

// return true if the item was removed from storage, false if it was not found
bool Storage::RemoveItem(uint64_t dataID) {
  for (uint32_t i = 0; i < m_storageSpace; i++) {
    if (m_storage[i] != NULL && m_storage[i]->getID() == dataID) {
      free(m_storage[i]);
      m_storage[i] = NULL;
      return true;
    }
  }

  return false;
}

// this will empty all data items from storage
void Storage::ClearStorage() {
  for (uint32_t i = 0; i < m_storageSpace; i++) {
    if (m_storage[i] != NULL) {
      free(m_storage[i]);
      m_storage[i] = NULL;
    }
  }
}

std::vector<DataItem*> Storage::GetAll() {
  std::vector<DataItem*> items;

  for (uint32_t i = 0; i < m_storageSpace; i++) {
    if (m_storage[i] != NULL) items.push_back(m_storage[i]);
  }

  return items;
}

// this is a helper and will return the number of data items that can be stored in local storage
uint32_t Storage::GetFreeSpace() {
  uint32_t count = 0;

  for (uint32_t i = 0; i < m_storageSpace; i++) {
    if (m_storage[i] != NULL) {
      count++;
    }
  }

  return m_storageSpace - count;
}

};  // namespace rhpman
