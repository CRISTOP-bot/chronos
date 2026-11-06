#include <gtest/gtest.h>
#include "chronos/memory/checkpoint_restore.h"

using namespace chronos;

TEST(CheckpointRestore, RestoresCapturedBytesAndPermissions) {
    storage::CheckpointRecord checkpoint;
    checkpoint.id = 7;
    storage::CheckpointRegion region;
    region.base = 0x1000; region.size = 5; region.name = "data";
    region.readable = 1; region.writable = 0; region.executable = 1;
    region.bytes = {0x10, 0x20, 0xff};
    checkpoint.regions.push_back(region);
    auto result = memory::CheckpointRestorer::restore(checkpoint);
    ASSERT_TRUE(result.ok()) << result.error().message;
    const auto restored = result.value().regions();
    ASSERT_EQ(restored.size(), 1u);
    EXPECT_EQ(restored[0].base, 0x1000u);
    EXPECT_EQ(restored[0].size, 5u);
    EXPECT_EQ(restored[0].data, (std::vector<u8>{0x10, 0x20, 0xff}));
    EXPECT_TRUE(restored[0].readable);
    EXPECT_FALSE(restored[0].writable);
    EXPECT_TRUE(restored[0].executable);
}

TEST(CheckpointRestore, RejectsInvalidCapturedRegion) {
    storage::CheckpointRecord checkpoint;
    storage::CheckpointRegion too_many; too_many.base = 1; too_many.size = 1; too_many.bytes = {1, 2};
    checkpoint.regions.push_back(too_many);
    EXPECT_FALSE(memory::CheckpointRestorer::restore(checkpoint).ok());
    checkpoint.regions.clear();
    storage::CheckpointRegion overflow; overflow.base = UINT64_MAX; overflow.size = 1;
    checkpoint.regions.push_back(overflow);
    EXPECT_FALSE(memory::CheckpointRestorer::restore(checkpoint).ok());
}
