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
    return {{0, kAvailBase, kAvailSize, kFd0}};
}

constexpr uint64_t kDev1Gap = 8ULL * 1024 * 1024 * 1024;

std::vector<furiosa::DramRange> twoDevices() {
    return {{0, kAvailBase, kAvailSize, kFd0},
            {1, kAvailBase + kDev1Gap, kAvailSize, kFd1}};
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

TEST(FuriosaRangeTest, DmabufOffsetIsRelativeToAvailableBase) {
    auto ranges = oneDevice();
    int fd = -1;
    uint64_t offset = 0;
    ASSERT_TRUE(furiosa::toDmabufOffsetIn(ranges, kAvailBase + 0x2000, 0x1000,
                                          &fd, &offset));
    EXPECT_EQ(fd, kFd0);
    EXPECT_EQ(offset, 0x2000ULL);
}

TEST(FuriosaRangeTest, DmabufOffsetZeroAtBase) {
    auto ranges = oneDevice();
    int fd = -1;
    uint64_t offset = 0;
    ASSERT_TRUE(
        furiosa::toDmabufOffsetIn(ranges, kAvailBase, 0x1000, &fd, &offset));
    EXPECT_EQ(fd, kFd0);
    EXPECT_EQ(offset, 0ULL);
}

TEST(FuriosaRangeTest, DmabufSelectsOwningDeviceFd) {
    auto ranges = twoDevices();
    int fd = -1;
    uint64_t offset = 0;
    ASSERT_TRUE(furiosa::toDmabufOffsetIn(ranges, kAvailBase + kDev1Gap + 0x3000,
                                          0x1000, &fd, &offset));
    EXPECT_EQ(fd, kFd1);
    EXPECT_EQ(offset, 0x3000ULL);
}

TEST(FuriosaRangeTest, DmabufRejectsUnalignedAddr) {
    auto ranges = oneDevice();
    int fd = -1;
    uint64_t offset = 0;
    EXPECT_FALSE(furiosa::toDmabufOffsetIn(ranges, kAvailBase + 0x1800, 0x1000,
                                           &fd, &offset));
}

TEST(FuriosaRangeTest, DmabufRejectsUnalignedLength) {
    auto ranges = oneDevice();
    int fd = -1;
    uint64_t offset = 0;
    EXPECT_FALSE(furiosa::toDmabufOffsetIn(ranges, kAvailBase, 0x800, &fd,
                                           &offset));
}

TEST(FuriosaRangeTest, DmabufRejectsOutOfRange) {
    auto ranges = oneDevice();
    int fd = -1;
    uint64_t offset = 0;
    EXPECT_FALSE(
        furiosa::toDmabufOffsetIn(ranges, kAvailBase - 0x1000, 0x1000, &fd,
                                  &offset));
    EXPECT_FALSE(furiosa::toDmabufOffsetIn(ranges, kAvailBase + kAvailSize,
                                           0x1000, &fd, &offset));
    EXPECT_FALSE(furiosa::toDmabufOffsetIn(ranges, kAvailBase,
                                           kAvailSize + 0x1000, &fd, &offset));
}

TEST(FuriosaRangeTest, DmabufRejectsZeroLength) {
    auto ranges = oneDevice();
    int fd = -1;
    uint64_t offset = 0;
    EXPECT_FALSE(
        furiosa::toDmabufOffsetIn(ranges, kAvailBase, 0, &fd, &offset));
}

}  // namespace
}  // namespace mooncake
