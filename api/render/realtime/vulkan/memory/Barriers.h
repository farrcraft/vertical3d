/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <vulkan/vulkan.h>

#include <initializer_list>

namespace v3d::render::realtime::vulkan::memory {

/**
 * The layout transitions the engine makes, each named for what the image is being made ready
 * for. Every one is the whole of a single level, single layer image, and the stages and access
 * either side are decided here rather than by whoever records it, so that two places moving an
 * image the same way cannot drift apart.
 **/

/**
 * Into COLOR_ATTACHMENT for a pass to draw into. What the image held is discarded.
 **/
VkImageMemoryBarrier2 colourForDrawing(VkImage image);

/**
 * Out of COLOR_ATTACHMENT once the last pass has drawn: SHADER_READ_ONLY for a later pass to
 * sample, or anything else - PRESENT_SRC for the presentation engine - with only the writes
 * made visible.
 **/
VkImageMemoryBarrier2 colourAfterDrawing(VkImage image, VkImageLayout to);

/**
 * Into DEPTH_ATTACHMENT for a pass to test against. What the image held is discarded, which
 * is why the first pass to use it in a frame has to clear.
 **/
VkImageMemoryBarrier2 depthForDrawing(VkImage image);

/**
 * Out of DEPTH_ATTACHMENT for a later pass to sample.
 *
 * DEPTH_READ_ONLY_OPTIMAL rather than SHADER_READ_ONLY_OPTIMAL, because it is the layout a depth
 * aspect can be both sampled and tested in. A pass sampling the image relies on nothing writing
 * it while it is read. A pass that both samples a target's depth and draws into it breaks that
 * rule; the validation layer reports it, and nothing here detects it.
 **/
VkImageMemoryBarrier2 depthForSampling(VkImage image);

/**
 * Into TRANSFER_DST for an upload to copy into. What the image held is discarded.
 **/
VkImageMemoryBarrier2 forUpload(VkImage image);

/**
 * Out of TRANSFER_DST once an upload has copied in, into SHADER_READ_ONLY.
 **/
VkImageMemoryBarrier2 uploadedForSampling(VkImage image);

/**
 * Into TRANSFER_SRC for a readback to copy out of, from whatever layout the frame left it in.
 * @param aspect colour or depth, which decides what writes are waited on
 **/
VkImageMemoryBarrier2 forReadback(VkImage image, VkImageAspectFlags aspect, VkImageLayout from);

/**
 * Out of TRANSFER_SRC once a readback has copied out, back into the layout it came from.
 **/
VkImageMemoryBarrier2 afterReadback(VkImage image, VkImageAspectFlags aspect, VkImageLayout to);

/**
 * Record the barriers as one dependency.
 **/
void record(VkCommandBuffer commands, std::initializer_list<VkImageMemoryBarrier2> barriers);

};  // namespace v3d::render::realtime::vulkan::memory
