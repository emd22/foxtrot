#include "Assert.hpp"

#include <Asset/AssetManager.hpp>
#include <Engine.hpp>

namespace fx {

void Terminate()
{
	FX_BREAKPOINT;
	std::terminate();
}

} // namespace fx
