#ifndef TTABLE_HPP
#define TTABLE_HPP

#include <vector>
#include <cstring>
#include <cassert>
#include <cstdint>

/**
 * Transposition Table is a simple hash map with fixed storage size.
 * In case of collision we keep the last entry and overide the previous one.
 *
 * We use 56-bit keys and 8-bit non-null values
 */
class TranspositionTable {
  private:

  struct Entry {
    uint64_t key: 56; // use 56-bit keys
    uint8_t val;      // use 8-bit values
  };                  // overall sizeof(Entry) = 8 bytes

  std::vector<Entry> T;

  unsigned int index(uint64_t key) const {
    return key%T.size();
  }

  public:

  TranspositionTable(unsigned int size): T(size) {
    assert(size > 0);
  }

  /*
   * Empty the Transition Table.
   */
  void reset() { // fill everything with 0, because 0 value means missing data
    memset(&T[0], 0, T.size()*sizeof(Entry));
  }

  /**
   * Store a value for a given key
   * @param key: 56-bit key
   * @param value: non-null 8-bit value. null (0) value are used to encode missing data.
   */
  void put(uint64_t key, uint8_t val) {
    assert(key < (1LL << 56));
    unsigned int i = index(key); // compute the index position
    T[i].key = key;              // and overide any existing value.
    T[i].val = val;       
  }


  /** 
   * Get the value of a key
   * @param key
   * @return 8-bit value associated with the key if present, 0 otherwise.
   */
  uint8_t get(uint64_t key) const {
    assert(key < (1LL << 56));
    unsigned int i = index(key);  // compute the index position
    if(T[i].key == key) 
      return T[i].val;            // and return value if key matches
    else 
      return 0;                   // or 0 if missing entry
  }

};

#endif