
#define VMA_DEBUG_LOG(...) LogWarning(LC_MEMORY, __VA_ARGS__)

#include "FoxtrotGame.hpp"

#include <Asset/AssetManager.hpp>
#include <Asset/ConfigFile.hpp>
#include <Asset/DataPack.hpp>
#include <Asset/Font/Font.hpp>
#include <Asset/MipmapGen.hpp>
#include <Asset/ShaderCompiler.hpp>
#include <Asset/ShaderPreproc.hpp>
#include <Core/Defer.hpp>
#include <Core/FilesystemIO.hpp>
#include <Core/FreeArray.hpp>
#include <Core/MemPool/MemPool.hpp>
#include <Core/Path.hpp>
#include <Core/Queue.hpp>
#include <Core/String.hpp>
#include <Engine.hpp>
#include <FoxScript/FoxScript.hpp>
#include <Math/MathConsts.hpp>
#include <Math/MathUtil.hpp>
#include <Renderer/Globals.hpp>

// #define FX_RUN_TEST
// #define FX_TEST_SCRIPT

FX_SET_MODULE_NAME("Main")

using namespace fx;
using namespace fx::renderer;


#ifdef FX_BUILD_DEBUG
#include <crtdbg.h>
#include <Windows.h>
#include <dbghelp.h>
#pragma comment(lib, "dbghelp")

static void SafeWrite(const char* text, size_t len)
{
	HANDLE err = GetStdHandle(STD_ERROR_HANDLE);
	DWORD written = 0;
	WriteFile(err, text, static_cast<DWORD>(len), &written, nullptr);
}

// Allocation-free (no fprintf/SymFromAddr - both recurse into the heap we are reporting on).
static int CrtReportHook(int report_type, char* message, int* ret)
{
	(void)report_type;

	char out[2048];
	int len = _snprintf_s(out, sizeof(out), _TRUNCATE, "CRT: %s\n  base=%p\n", message, GetModuleHandleW(nullptr));

	// The reported address is the first guard byte (= user data end). Scan back for the block header
	// (ucrt x64: next,prev,file,line@24,data_size@32,request@40,gap[4],user@48) and decode it.
	unsigned long long block_addr = 0;
	sscanf_s(message, "HEAP CORRUPTION DETECTED: after Normal block (#%*d) at %llX", &block_addr);
	if (block_addr) {
		const uint8* guard = reinterpret_cast<const uint8*>(block_addr);
		for (uint32_t data_size = 0; data_size <= 8192; data_size += 8) {
			const uint8* hdr = guard - 48 - data_size;
			const uint64* f = reinterpret_cast<const uint64*>(hdr);
			const bool size_ok = f[4] == data_size;
			const uint32_t request = *reinterpret_cast<const uint32_t*>(hdr + 40);
			const bool req_ok = request > 0 && request < 1000000;
			const bool ptrs_ok = f[0] > 0x10000 && f[0] < 0x7FFFFFFFFFFF;
			if (size_ok && req_ok && ptrs_ok) {
				len += _snprintf_s(out + len, sizeof(out) - len, _TRUNCATE,
								   "victim: data_size=%llu request=%lu\n", (unsigned long long)f[4],
								   *reinterpret_cast<const long*>(hdr + 40));
				break;
			}
		}
	}

	void* stack[16] = {};
	const WORD count = CaptureStackBackTrace(1, 16, stack, nullptr);
	for (WORD i = 0; i < count && len > 0 && len < (int)sizeof(out) - 32; i++) {
		len += _snprintf_s(out + len, sizeof(out) - len, _TRUNCATE, "  #%u %p\n", i, stack[i]);
	}

	SafeWrite(out, len);

	if (ret) {
		*ret = 1;
	}
	return TRUE;
}

static int CrtAllocHook(int alloc_type, void* user_data, size_t size, int block_type, long request,
						const unsigned char*, int)
{
	if (alloc_type == _HOOK_ALLOC) {
		if (request == 1) {
			fprintf(stderr, ">>> alloc hook alive\n");
		}
		if (request >= 700 && request <= 950) {
			fprintf(stderr, "ALLOC #%ld size=%zu\n", request, size);
		}
	}
	return TRUE;
}
#endif

int main()
{
#ifdef FX_BUILD_DEBUG
	// Route debug-CRT error/assert reports (heap corruption, invalid params) to stderr with a
	// symbolized stack, instead of OutputDebugString + break.
	_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDERR);
	_CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
	_CrtSetReportHook2(_CRT_RPTHOOK_INSTALL, CrtReportHook);
	// Don't lose the log tail if we crash before a flush.
	setvbuf(stdout, nullptr, _IONBF, 0);
	// Validate the entire heap on every alloc/free so a buffer overrun is reported on the first
	// heap operation after the offending write.
	_CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_CHECK_ALWAYS_DF | _CRTDBG_DELAY_FREE_MEM_DF);
#endif

	fx::gEnginePool = new fx::MemPool;
	fx::gEnginePool->Create(FX_MEMORY_ENGINE_POOL_SIZE);

	fx::gScriptMemPool = new fx::MemPool;
	fx::gScriptMemPool->Create(1024 * 64);


#ifdef FX_TEST_SCRIPT
	script::FoxScript fs;
	fs.Load("./Scripts/GlobalTest.fox");

	script::FoxSymbol* sym = fs.GetSymbol("Default");
	if (!sym) {
		LogError("Cannot find symbol!");
	}

	script::FoxValue value = fs.CallProc(sym, {});

	LogInfo("Value: {}", value);
#endif

#ifndef FX_RUN_TEST
	fx::renderer::Globals::Init();

	{
		fx::FoxtrotGame game {};
	}

	fx::Globals::Destroy();
	fx::renderer::Globals::Destroy();

	if (gAssetManager) {
		delete gAssetManager;
		gAssetManager = nullptr;
	}

	Defer(
		[]()
		{
			delete fx::gEnginePool;
			fx::gEnginePool = nullptr;
		});
#endif
	return 0;
}
