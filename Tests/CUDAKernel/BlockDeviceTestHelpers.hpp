#pragma once

bool run_block_exclusive_scan_i32();

bool run_block_inclusive_scan_i32();

bool run_block_compact_i32();

bool run_warp_reduce_sum_i32();

bool run_warp_prefix_sum_i32();

bool run_warp_vote_and_compact();

bool run_wgmma_descriptor_smoke();

bool run_wgmma_descriptor_encode_device();

bool run_wgmma_shape_f32(int n);

bool run_wgmma_shape_f16(int n);
