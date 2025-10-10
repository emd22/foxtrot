#include "FxShaderCompiler.hpp"

#include <ThirdParty/Slang/slang-com-ptr.h>
#include <ThirdParty/Slang/slang.h>

#include <Core/FxBasicDb.hpp>
#include <Core/FxFile.hpp>
#include <Core/FxStackArray.hpp>
#include <string>

static FxBasicDb sShaderCompileDb;

static Slang::ComPtr<slang::IGlobalSession>& GetSlangGlobalSession()
{
    thread_local Slang::ComPtr<slang::IGlobalSession> global_session;

    // If the session has not been created, create it
    if (!global_session.get()) {
        slang::createGlobalSession(global_session.writeRef());
    }

    return global_session;
}

static void CreateSlangSession(Slang::ComPtr<slang::ISession>& local_session)
{
    Slang::ComPtr<slang::IGlobalSession>& global_session = GetSlangGlobalSession();

    slang::TargetDesc target_desc {
        .format = SLANG_SPIRV,
        .profile = global_session->findProfile("spirv_1_5"),
    };

    FxStackArray<slang::PreprocessorMacroDesc, 0> preprocessor_macros = {};

    FxStackArray<slang::CompilerOptionEntry, 2> compiler_options = {
        {
            slang::CompilerOptionName::EmitSpirvDirectly,
            { slang::CompilerOptionValueKind::Int, 1, 0, nullptr, nullptr },
        },
        {
            slang::CompilerOptionName::LanguageVersion,
            { slang::CompilerOptionValueKind::String, 0, 0, "2026", nullptr },
        }
    };

    slang::SessionDesc session_desc {
        .targets = &target_desc,
        .targetCount = 1,

        .preprocessorMacros = preprocessor_macros.Data,
        .preprocessorMacroCount = preprocessor_macros.Size,

        .compilerOptionEntries = compiler_options.Data,
        .compilerOptionEntryCount = compiler_options.Size,
    };

    global_session->createSession(session_desc, local_session.writeRef());
}

static Slang::ComPtr<slang::ISession>& GetSlangSession()
{
    thread_local Slang::ComPtr<slang::ISession> local_session;

    if (!local_session.get()) {
        CreateSlangSession(local_session);
    }

    return local_session;
}

static void PrintSlangDiagnostics(Slang::ComPtr<slang::IBlob>& diagnostic_blob)
{
    if (diagnostic_blob != nullptr) {
        FxLogError("Diagnostic Info: {}", diagnostic_blob->getBufferPointer());
    }
}

void RecordShaderCompileTime(const char* path)
{
    FxLogInfo("Logging compile time for {}", path);

    uint64 modification_time = FxFile::GetLastModifiedTime(path).time_since_epoch().count();
    sShaderCompileDb.WriteEntry(
        FxBasicDbEntry { .KeyHash = FxHashStr(path), .Value = std::to_string(modification_time) });

    sShaderCompileDb.SaveToFile();
}

bool IsShaderUpToDate(const char* path)
{
    FxBasicDbEntry* entry = sShaderCompileDb.FindEntry(FxHashStr(path));
    if (!entry) {
        return false;
    }

    uint64 latest_modification_time = FxFile::GetLastModifiedTime(path).time_since_epoch().count();

    if (std::stol(entry->Value) != latest_modification_time) {
        return false;
    }

    return true;
}


void FxShaderCompiler::Compile(const char* path, const char* output_path)
{
    if (!sShaderCompileDb.IsOpen()) {
        sShaderCompileDb.Open("../Shaders/ShaderCompileDb.fxdb");
    }

    if (IsShaderUpToDate(path)) {
        FxLogInfo("Shader {} is up to date!", path);
        return;
    }

    bool has_vertex_shader = false;
    bool has_fragment_shader = false;

    slang::IModule* module = nullptr;
    Slang::ComPtr<slang::ISession>& session = GetSlangSession();

    Slang::ComPtr<slang::IBlob> diagnostic_blob;

    module = session->loadModule(path, diagnostic_blob.writeRef());

    if (diagnostic_blob != nullptr) {
        FxLogError("Error loading Slang module for {}", path);
        PrintSlangDiagnostics(diagnostic_blob);
        return;
    }

    Slang::ComPtr<slang::IEntryPoint> vertex_entry_point;
    Slang::ComPtr<slang::IEntryPoint> fragment_entry_point;

    module->findEntryPointByName("VertexMain", vertex_entry_point.writeRef());
    module->findEntryPointByName("FragmentMain", fragment_entry_point.writeRef());

    FxStackArray<slang::IComponentType*, 3> component_types;
    component_types.Insert(module);

    // Add each entry point if they are defined
    if (vertex_entry_point.get()) {
        component_types.Insert(vertex_entry_point);
        has_vertex_shader = true;
    }

    if (fragment_entry_point.get()) {
        component_types.Insert(fragment_entry_point);
        has_fragment_shader = true;
    }

    Slang::ComPtr<slang::IComponentType> composed_program;

    SlangResult result = session->createCompositeComponentType(component_types.Data, component_types.Size,
                                                               composed_program.writeRef(), diagnostic_blob.writeRef());

    if (SLANG_FAILED(result) || diagnostic_blob != nullptr) {
        PrintSlangDiagnostics(diagnostic_blob);
    }

    Slang::ComPtr<slang::IBlob> spirv_code;

    // Write the vertex shader code
    if (has_vertex_shader) {
        result = composed_program->getEntryPointCode(0, 0, spirv_code.writeRef(), diagnostic_blob.writeRef());

        if (SLANG_FAILED(result)) {
            FxLogError("Could not get vertex shader compiled SPIRV(Path={})", path);
            PrintSlangDiagnostics(diagnostic_blob);
            return;
        }

        {
            std::string vertex_path = output_path;
            vertex_path += "_vs";

            FxFile output_file(vertex_path.c_str(), "wb");

            output_file.Write(spirv_code->getBufferPointer(), spirv_code->getBufferSize());

            output_file.Close();
        }

        FxLogInfo("Compiled vertex shader {}", path);
    }

    // Clear the spirv_code
    spirv_code.setNull();

    // Write the fragment shader code
    if (has_fragment_shader) {
        result = composed_program->getEntryPointCode(1, 0, spirv_code.writeRef(), diagnostic_blob.writeRef());

        if (SLANG_FAILED(result)) {
            FxLogError("Could not get fragment shader compiled SPIRV(Path={})", path);
            PrintSlangDiagnostics(diagnostic_blob);
            return;
        }

        {
            std::string fragment_path = output_path;
            fragment_path += "_fs";

            FxFile output_file(fragment_path.c_str(), "wb");

            output_file.Write(spirv_code->getBufferPointer(), spirv_code->getBufferSize());

            output_file.Close();
        }

        FxLogInfo("Compiled fragment shader {}", path);
    }

    RecordShaderCompileTime(path);
}

void FxShaderCompiler::Destroy()
{
    sShaderCompileDb.Close();

    GetSlangSession().setNull();
    GetSlangGlobalSession().setNull();
}
