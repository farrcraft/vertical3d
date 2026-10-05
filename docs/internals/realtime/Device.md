# Device selection

How the renderer picks a GPU and what it requires of one.

## Device selection

The instance declares `VK_API_VERSION_1_3`. `Device::selectPhysical()` skips a physical device
that:

- reports an API version below 1.3,
- lacks a required extension (`VK_KHR_swapchain`, only when presenting),
- lacks the 1.3 features `dynamicRendering` and `synchronization2`, or
- has no graphics family, or no present family for the surface when presenting.

Of the devices left, the first discrete GPU wins; otherwise the first acceptable device. If none
qualifies, the constructor throws naming the requirement.

Both features are requested explicitly through a `VkPhysicalDeviceVulkan13Features` chained
onto `VkPhysicalDeviceFeatures2`. A chained features struct and `pEnabledFeatures` are mutually
exclusive, so the base features travel in the chain too. `wideLines` is not requested, which is
why lines are one pixel wide.

A device given no surface is headless: it selects on the graphics family alone, enables no
swapchain extension, and has no present queue. Everything that draws works on it; `Swapchain` and
`Presenter` need a presenting device.

`Device` also records the timestamp period and the graphics family's `timestampValidBits`, for
[timings](Frames.md#timings-and-statistics).

**Validation.** `Instance` enables `VK_LAYER_KHRONOS_validation` when it is installed, with a
messenger that routes messages through the logger and counts errors and warnings
(`errors()`, `warnings()`, `firstError()`). Without the messenger the layer reports nowhere,
which reads the same as a clean run. Check `validating()` alongside the counts. How the tests use
this is in [contributing/Testing.md](../../contributing/Testing.md).

A `VkResult` that is not a success throws through `device::check(result, what)`, naming the call
and the result.

Background: [ADR-0001](../../adr/0001-rendering-replace-opengl-with-vulkan.md),
[ADR-0002](../../adr/0002-vulkan-require-version-1-3.md)
