// https://github.com/trevor-makes/uMon.git
// Copyright (c) 2022 Trevor Makes

#pragma once

#include <stdint.h>

namespace uMon {

// This data structure allocates key-value pairs within a fixed size buffer
// TODO sort by address for binary search on dasm
// TODO maybe refactor with uCLI::History
class Labels {
  char* buffer_;
  uint8_t buf_size_;
  uint8_t entries_ = 0;

  char* get(uint8_t index, uint8_t& size) const;
  char* insert(uint8_t index, uint8_t size);

  bool remove_range(uint8_t first, uint8_t last);
  bool remove(uint8_t index) { return remove_range(index, index); }

public:
  template <uint8_t N>
  Labels(char (&buffer)[N]): Labels(buffer, N) {}
  Labels(char* buffer, uint8_t size): buffer_{buffer}, buf_size_{size} {}

  // Remove default copy ops
  Labels(const Labels&) = delete;
  Labels& operator=(const Labels&) = delete;

  uint8_t entries() const { return entries_; }

  bool get_index(uint8_t index, const char*& name, uint16_t& addr) const;
  bool get_addr(const char* name, uint16_t& addr) const;
  bool get_name(uint16_t addr, const char*& name) const;

  bool remove_label(const char* name);
  bool set_label(const char* name, uint16_t addr);
};

template <uint8_t SIZE>
class LabelsOwner : public Labels {
  char buffer_[SIZE];
public:
  LabelsOwner(): Labels(buffer_) {}
};

} // namespace uMon
