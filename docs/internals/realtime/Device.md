# Device selection

How the renderer picks a GPU and what it requires of one.

## Device selection

The instance declares `VK_API_VERSION_1_3`. `Device::selectPhysical()` skips a physical device
that:

- reports an API version below 1.3,
- lacks a required extension (`VK_KHR_swapchain`, only when presenting),
- lacks the 1.3 features `dynamicRendering` and `synchronization2`, or the 1.2 feature
  `separateDepthStencilLayouts`, which every 1.2 device supports,
- lacks what the texture array needs: `shaderSampledImageArrayDynamicIndexing`, and the 1.2
  features `shaderSampledImageArrayNonUniformIndexing`, `runtimeDescriptorArray`,
  `descriptorBindingPartiallyBound` and `descriptorBindingUpdateUnusedWhilePending`, or
- has no graphics family, or no present family for the surface when presenting.

Each device skipped is logged at info with the first thing it lacks: its version, an extension,
or a feature by name. Of the devices left, the first discrete GPU wins; otherwise the first
acceptable device. If none qualifies, the constructor throws naming the requirement.

## Portability drivers

A portability driver implements Vulkan over another API, as MoltenVK does over Metal, and may
leave out parts of the specification. A loader hides one from an instance that has not asked for
it.

- `Instance::enable(requested, offered, validating)` chooses what the instance is created with.
  It adds `VK_KHR_portability_enumeration`, and sets
  `VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR`, only when the loader offers the extension.
  An older loader refuses an extension it does not implement, so one that does not offer it is
  not asked. `Instance::portability()` says which way it
  went, and the instance's log line says so too.
- `Device::extensions(presenting, offered)` chooses what the logical device is created with. It
  adds `VK_KHR_portability_subset` when the physical device offers it, because a device that offers
  it must have it enabled. The name is spelled in `Device.cxx`, since only Vulkan's beta header
  defines it.
- Both are functions of what is offered, and have unit tests of both branches. No machine here
  has a portability driver, so those tests are what check the choice. The device suite on a
  conformant driver checks that nothing else changed.
- A portability driver can report 1.3 and still lack a feature the renderer needs. Selection
  checks each feature by name, so such a device is passed over and the log says what it lacks.

The 1.3 features are requested explicitly through a `VkPhysicalDeviceVulkan13Features`, and
`separateDepthStencilLayouts` through a `VkPhysicalDeviceVulkan12Features`, both chained onto
`VkPhysicalDeviceFeatures2`. `separateDepthStencilLayouts` lets a barrier move only the depth
aspect of a combined depth and stencil format, and every depth barrier does that. A chained
features struct and `pEnabledFeatures` are mutually exclusive, so the base features travel in
the chain too. `wideLines` is not requested, so lines are one pixel wide.

`fillModeNonSolid` is requested only where the gpu offers it, and a device without it is still
chosen. It lets a pipeline draw a triangle as its edges or its corners, with
`VK_POLYGON_MODE_LINE` or `VK_POLYGON_MODE_POINT`. `Device::nonSolidFill()` says whether it is
on, and a pipeline built without it must draw filled, or validation reports it. A pipeline that
draws in point mode also needs its vertex stage to write `gl_PointSize`.

A device given no surface is headless: it selects on the graphics family alone, enables no
swapchain extension, and has no present queue. Everything that draws works on it; `Swapchain` and
`Presenter` need a presenting device.

`Device` also records the timestamp period and the graphics family's `timestampValidBits`, for
[timings](Frames.md#timings-and-statistics).

`Device::description()` names the selected device without a Vulkan type: its name, the Vulkan
version it supports split into major, minor and patch, its PCI vendor id, and its driver version
as the vendor encodes it. It is filled in `selectPhysical()` and never changes. A diagnostics
readout reads it rather than `physical()`, so it needs no Vulkan include.

**Validation.** `Instance` enables `VK_LAYER_KHRONOS_validation` when it is installed, with a
messenger that routes messages through the logger and counts errors and warnings
(`errors()`, `warnings()`, `firstError()`, `firstWarning()`). Without the messenger the layer
reports nowhere, which reads the same as a clean run. Check `validating()` alongside the
counts. How the tests use this is in [contributing/Testing.md](../../contributing/Testing.md).

A `VkResult` that is not a success throws through `device::check(result, what)`, naming the call
and the result. A destructor or a teardown must not throw, so it waits for the device to go idle
through `Ring::waitIdleNoThrow()`, which ignores the result. `Ring::waitIdle()` checks it.

Background: [ADR-0001](../../adr/0001-rendering-replace-opengl-with-vulkan.md),
[ADR-0002](../../adr/0002-vulkan-require-version-1-3.md)
