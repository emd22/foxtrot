#pragma once

#include <Core/Types.hpp>

namespace fx {

struct ThreadID
{
	using IDType = uint32;

	static const ThreadID scMainThread;

public:
	ThreadID() = default;
	ThreadID(IDType id) : ID(id) {}
	ThreadID(const ThreadID& other) : ID(other.ID) {}

	ThreadID& operator=(uint32 value) = delete;
	ThreadID& operator=(const ThreadID& other)
	{
		ID = other.ID;
		return *this;
	}

	IDType operator()() const { return ID; }

	bool operator==(const ThreadID& other) const { return (ID == other.ID); }
	bool operator<(const ThreadID& other) const { return (ID < other.ID); }

	FX_FORCE_INLINE IDType GetID() const { return ID; }
	FX_FORCE_INLINE bool IsMainThread() const { return ID == scMainThread; }

public:
	IDType ID = UINT32_MAX;
};

} // namespace fx
