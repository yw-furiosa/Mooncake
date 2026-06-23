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

#include <gtest/gtest.h>

#include "furiosa_npu.h"

namespace mooncake {
namespace {

constexpr uint64_t kRawBase = 0x3000000000ULL;
constexpr uint64_t kDramBase = 256ULL * 1024 * 1024;
constexpr uint64_t kAvailBase = kRawBase + kDramBase;
constexpr uint64_t kAvailSize = 4ULL * 1024 * 1024 * 1024;
constexpr int kFd0 = 7;
constexpr int kFd1 = 9;

std::vector<furiosa::DramRange> oneDevice() {
    return {{0, kRawBase, kAvailBase, kAvailSize, kFd0}};
}

constexpr uint64_t kDev1Gap = 8ULL * 1024 * 1024 * 1024;

std::vector<furiosa::DramRange> twoDevices() {
    return {{0, kRawBase, kAvailBase, kAvailSize, kFd0},
            {1, kRawBase + kDev1Gap, kAvailBase + kDev1Gap, kAvailSize, kFd1}};
}

TEST(FuriosaRangeTest, DeviceOfBoundaries) {
    auto ranges = oneDevice();
    EXPECT_EQ(furiosa::deviceOfIn(ranges, kAvailBase), 0);
    EXPECT_EQ(furiosa::deviceOfIn(ranges, kAvailBase + kAvailSize - 1), 0);
    EXPECT_EQ(furiosa::deviceOfIn(ranges, kAvailBase - 1), -1);
    EXPECT_EQ(furiosa::deviceOfIn(ranges, kAvailBase + kAvailSize), -1);
}

TEST(FuriosaRangeTest, DeviceOfMultiAndGap) {
    auto ranges = twoDevices();
    EXPECT_EQ(furiosa::deviceOfIn(ranges, kAvailBase + 0x1000), 0);
    EXPECT_EQ(furiosa::deviceOfIn(ranges, kAvailBase + kDev1Gap + 0x1000), 1);
    EXPECT_EQ(furiosa::deviceOfIn(ranges, kAvailBase + kAvailSize + 0x1000), -1);
}

TEST(FuriosaRangeTest, ExportOffsetIsRelativeToRawBase) {
    auto ranges = oneDevice();
    int idx = -1;
    uint64_t export_offset = 0;
    ASSERT_TRUE(furiosa::toExportRegionIn(ranges, kAvailBase + 0x2000, 0x1000,
                                          &idx, &export_offset));
    EXPECT_EQ(idx, 0);
    EXPECT_EQ(export_offset, kDramBase + 0x2000);
}

TEST(FuriosaRangeTest, ExportOffsetAtBaseEqualsDramBase) {
    auto ranges = oneDevice();
    int idx = -1;
    uint64_t export_offset = 0;
    ASSERT_TRUE(furiosa::toExportRegionIn(ranges, kAvailBase, 0x1000, &idx,
                                          &export_offset));
    EXPECT_EQ(idx, 0);
    EXPECT_EQ(export_offset, kDramBase);
}

TEST(FuriosaRangeTest, ExportSelectsOwningDevice) {
    auto ranges = twoDevices();
    int idx = -1;
    uint64_t export_offset = 0;
    ASSERT_TRUE(furiosa::toExportRegionIn(ranges, kAvailBase + kDev1Gap + 0x3000,
                                          0x1000, &idx, &export_offset));
    EXPECT_EQ(idx, 1);
    EXPECT_EQ(export_offset, kDramBase + 0x3000);
}

TEST(FuriosaRangeTest, ExportRejectsUnalignedAddr) {
    auto ranges = oneDevice();
    int idx = -1;
    uint64_t export_offset = 0;
    EXPECT_FALSE(furiosa::toExportRegionIn(ranges, kAvailBase + 0x1800, 0x1000,
                                           &idx, &export_offset));
}

TEST(FuriosaRangeTest, ExportRejectsUnalignedLength) {
    auto ranges = oneDevice();
    int idx = -1;
    uint64_t export_offset = 0;
    EXPECT_FALSE(furiosa::toExportRegionIn(ranges, kAvailBase, 0x800, &idx,
                                           &export_offset));
}

TEST(FuriosaRangeTest, ExportRejectsOutOfRange) {
    auto ranges = oneDevice();
    int idx = -1;
    uint64_t export_offset = 0;
    EXPECT_FALSE(furiosa::toExportRegionIn(ranges, kAvailBase - 0x1000, 0x1000,
                                           &idx, &export_offset));
    EXPECT_FALSE(furiosa::toExportRegionIn(ranges, kAvailBase + kAvailSize,
                                           0x1000, &idx, &export_offset));
    EXPECT_FALSE(furiosa::toExportRegionIn(ranges, kAvailBase,
                                           kAvailSize + 0x1000, &idx,
                                           &export_offset));
}

TEST(FuriosaRangeTest, ExportRejectsZeroLength) {
    auto ranges = oneDevice();
    int idx = -1;
    uint64_t export_offset = 0;
    EXPECT_FALSE(
        furiosa::toExportRegionIn(ranges, kAvailBase, 0, &idx, &export_offset));
}

}  // namespace
}  // namespace mooncake
