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

#include "tent/platform/furiosa.h"

#include <cstring>
#include <glog/logging.h>

#include "tent/platform/cpu.h"
#include "furiosa_npu.h"

namespace mooncake {
namespace tent {

Status FuriosaPlatform::probe(std::vector<Topology::NicEntry> &nic_list,
                              std::vector<Topology::MemEntry> &mem_list) {
    furiosa::initRanges();
    return CpuPlatform(conf).probe(nic_list, mem_list);
}

Status FuriosaPlatform::allocate(void **pptr, size_t size,
                                 MemoryOptions &options) {
    (void)pptr;
    (void)size;
    (void)options;
    return Status::NotImplemented(
        "Furiosa allocation is owned by device-runtime" LOC_MARK);
}

Status FuriosaPlatform::free(void *ptr, size_t size) {
    (void)ptr;
    (void)size;
    return Status::NotImplemented(
        "Furiosa free is owned by device-runtime" LOC_MARK);
}

Status FuriosaPlatform::copy(void *dst, void *src, size_t length) {
    if (furiosa::contains((uint64_t)dst) || furiosa::contains((uint64_t)src)) {
        return Status::NotImplemented(
            "Furiosa NPU buffers support RDMA transport only; copy-based "
            "transports are not supported for NPU memory" LOC_MARK);
    }
    memcpy(dst, src, length);
    return Status::OK();
}

MemoryType FuriosaPlatform::getMemoryType(void *addr) {
    if (furiosa::contains((uint64_t)addr)) return MTYPE_FURIOSA;
    return MTYPE_CPU;
}

const std::vector<RangeLocation> FuriosaPlatform::getLocation(
    void *start, size_t len, bool skip_prefault) {
    int device = furiosa::deviceOf((uint64_t)start);
    if (device >= 0) {
        return {{(uint64_t)start, len, "furiosa:" + std::to_string(device)}};
    }
    return CpuPlatform(conf).getLocation(start, len, skip_prefault);
}

}  // namespace tent
}  // namespace mooncake
