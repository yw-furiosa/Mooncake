// Copyright 2024 KVCache.AI
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef FURIOSA_NPU_H
#define FURIOSA_NPU_H

#include <cstddef>
#include <cstdint>
#include <vector>

namespace mooncake {
namespace furiosa {

struct DramRange {
    int device_id;
    uint64_t raw_base;
    uint64_t available_base;
    uint64_t available_size;
    int bar_fd;
};

int deviceOfIn(const std::vector<DramRange> &ranges, uint64_t addr);

// Pure validation + offset math (no ioctl) for unit testing. Computes the
// BAR4-relative export offset for a buffer, and returns the owning range index.
bool toExportRegionIn(const std::vector<DramRange> &ranges, uint64_t addr,
                      size_t length, int *out_idx, uint64_t *out_export_offset);

bool initRanges();

int deviceOf(uint64_t addr);

bool contains(uint64_t addr);

bool dmabufFor(uint64_t addr, size_t length, int *out_fd, uint64_t *out_offset);

}  // namespace furiosa
}  // namespace mooncake

#endif  // FURIOSA_NPU_H
