/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

namespace v3d::engine {
	/*
	Features are portions of engine configuration that can be opted into/out of on a per-application basis.

	*/
	enum class Feature : uint32_t {
		Window = (1 << 0),
		Config = (1 << 1),
		MouseInput = (1 << 2),
		KeyboardInput = (1 << 3)
	};

	constexpr Feature operator|(Feature lhs, Feature rhs) {
		return static_cast<Feature>(static_cast<int>(lhs) | static_cast<int>(rhs));
	}

	constexpr bool operator&(int lhs, Feature rhs) {
		return lhs & static_cast<int>(rhs);
	}

};  // namespace v3d::engine
