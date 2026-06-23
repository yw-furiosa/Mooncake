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

#include "furiosa_npu.h"

#ifdef USE_FURIOSA

#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <mutex>

#include <glog/logging.h>

namespace mooncake {
namespace furiosa {

namespace {

constexpr unsigned int kNpuBarIocMagic = 'N';

struct NpuBarInfo {
    uint64_t bar_phy_addr;
    uint64_t bar_size;
} __attribute__((packed));

struct NpuDmabufRegion {
    uint64_t offset;
    uint64_t size;
    int fd;
} __attribute__((packed));

#define NPU_BAR_GET_INFO _IOWR(kNpuBarIocMagic, 0x00, struct NpuBarInfo)
#define NPU_BAR_EXPORT_DMABUF _IOWR(kNpuBarIocMagic, 0x01, struct NpuDmabufRegion)
#define NPU_GET_AVAILABLE_DRAM _IOWR(kNpuBarIocMagic, 0x02, struct NpuBarInfo)

constexpr int kMaxNpu = 64;
constexpr uint64_t kPageSize = 4096;

std::vector<DramRange> g_ranges;
std::once_flag g_init_flag;

void doInitRanges() {
    for (int id = 0; id < kMaxNpu; ++id) {
        char path[64];
        std::snprintf(path, sizeof(path), "/dev/rngd/npu%dbar4", id);
        int fd = ::open(path, O_RDWR | O_CLOEXEC);
        if (fd < 0) {
            if (id == 0) {
                PLOG(WARNING) << "Furiosa: cannot open " << path
                              << "; NPU support inactive";
            }
            break;
        }

        NpuBarInfo raw_info{};
        if (::ioctl(fd, NPU_BAR_GET_INFO, &raw_info) != 0) {
            PLOG(ERROR) << "Furiosa: NPU_BAR_GET_INFO failed on " << path;
            ::close(fd);
            continue;
        }

        NpuBarInfo avail_info{};
        if (::ioctl(fd, NPU_GET_AVAILABLE_DRAM, &avail_info) != 0) {
            PLOG(ERROR) << "Furiosa: NPU_GET_AVAILABLE_DRAM failed on " << path;
            ::close(fd);
            continue;
        }

        DramRange range;
        range.device_id = id;
        range.raw_base = raw_info.bar_phy_addr;
        range.available_base = avail_info.bar_phy_addr;
        range.available_size = avail_info.bar_size;
        range.bar_fd = fd;
        g_ranges.push_back(range);

        LOG(INFO) << "Furiosa: npu" << id << " raw_base=0x" << std::hex
                  << range.raw_base << " available_base=0x"
                  << range.available_base << " size=0x" << range.available_size
                  << std::dec << " bar_fd=" << range.bar_fd;
    }
}

}  // namespace

int deviceOfIn(const std::vector<DramRange> &ranges, uint64_t addr) {
    for (const auto &range : ranges) {
        if (addr >= range.available_base &&
            addr < range.available_base + range.available_size) {
            return range.device_id;
        }
    }
    return -1;
}

bool toExportRegionIn(const std::vector<DramRange> &ranges, uint64_t addr,
                      size_t length, int *out_idx, uint64_t *out_export_offset) {
    int idx = -1;
    for (size_t i = 0; i < ranges.size(); ++i) {
        if (addr >= ranges[i].available_base &&
            addr < ranges[i].available_base + ranges[i].available_size) {
            idx = static_cast<int>(i);
            break;
        }
    }
    if (idx < 0) return false;

    const DramRange &range = ranges[idx];
    if (length == 0) return false;
    if (addr + length < addr) return false;
    if (addr + length > range.available_base + range.available_size) {
        return false;
    }
    if ((addr & (kPageSize - 1)) != 0 || (length & (kPageSize - 1)) != 0) {
        return false;
    }

    *out_idx = idx;
    *out_export_offset = addr - range.raw_base;
    return true;
}

bool initRanges() {
    std::call_once(g_init_flag, doInitRanges);
    return !g_ranges.empty();
}

int deviceOf(uint64_t addr) {
    initRanges();
    return deviceOfIn(g_ranges, addr);
}

bool contains(uint64_t addr) { return deviceOf(addr) >= 0; }

bool dmabufFor(uint64_t addr, size_t length, int *out_fd, uint64_t *out_offset) {
    initRanges();

    int idx = -1;
    uint64_t export_offset = 0;
    if (!toExportRegionIn(g_ranges, addr, length, &idx, &export_offset)) {
        LOG(ERROR) << "Furiosa: no dmabuf for 0x" << std::hex << addr << std::dec
                   << " len=" << length
                   << " (unowned, out-of-range, or not page-aligned)";
        return false;
    }

    NpuDmabufRegion region{};
    region.offset = export_offset;
    region.size = length;
    region.fd = -1;
    if (::ioctl(g_ranges[idx].bar_fd, NPU_BAR_EXPORT_DMABUF, &region) != 0) {
        PLOG(ERROR) << "Furiosa: NPU_BAR_EXPORT_DMABUF failed for 0x" << std::hex
                    << addr << " export_offset=0x" << export_offset << std::dec
                    << " len=" << length;
        return false;
    }

    *out_fd = region.fd;
    *out_offset = 0;
    return true;
}

}  // namespace furiosa
}  // namespace mooncake

#endif  // USE_FURIOSA
