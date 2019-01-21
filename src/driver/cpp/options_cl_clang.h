// Copyright (C) 2017-2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "options_cl.h"
#include "options_cl_vs.h"

namespace sw
{

namespace gnu
{

struct Optimizations
{
    bool Disable = false;
    std::optional<int> Level;
    bool SmallCode = false;
    bool FastCode = false;
};

} // namespace gnu

DECLARE_OPTION_SPECIALIZATION(gnu::Optimizations);

namespace clang
{

enum class ArchType
{
    m32,
    m64,
};

} // namespace clang

DECLARE_OPTION_SPECIALIZATION(clang::ArchType);

struct SW_DRIVER_CPP_API GNUClangCommonOptions
{
    COMMAND_LINE_OPTION(Language, String)
    {
        cl::CommandFlag{ "x" },
    };

    COMMAND_LINE_OPTION(CPPStandard, CPPLanguageStandard)
    {
    };

    COMMAND_LINE_OPTION(InputFile, path)
    {
        cl::InputDependency{},
    };

    COMMAND_LINE_OPTION(OutputFile, path)
    {
        cl::CommandFlag{ "o" },
            cl::OutputDependency{},
    };

    COMMAND_LINE_OPTION(ForcedIncludeFiles, FilesOrdered)
    {
        cl::CommandFlag{ "include" },
            cl::CommandFlagBeforeEachValue{},
            cl::InputDependency{},
    };

    COMMAND_LINE_OPTION(VisibilityHidden, bool)
    {
        cl::CommandFlag{ "fvisibility=hidden" },
            true
    };

    COMMAND_LINE_OPTION(Optimizations, gnu::Optimizations);

    COMMAND_LINE_OPTION(PositionIndependentCode, bool)
    {
        cl::CommandFlag{ "fPIC" }, true
    };

    COMMAND_LINE_OPTION(GenerateDebugInfo, bool)
    {
        cl::CommandFlag{ "g" },
    };

    COMMAND_LINE_OPTION(CompileWithoutLinking, bool)
    {
        cl::CommandFlag{ "c" }, true,
    };

    COMMAND_LINE_OPTION(Permissive, bool)
    {
        cl::CommandFlag{ "fpermissive" }, true,
    };
};

// keep structure as in
// https://clang.llvm.org/docs/ClangCommandLineReference.html
struct SW_DRIVER_CPP_API ClangOptions : GNUClangCommonOptions
{
    // Introduction
    COMMAND_LINE_OPTION(NoStdIncludesC, bool)
    {
        cl::CommandFlag{ "nostdinc" },
            true
    };

    //COMMAND_LINE_OPTION(NoStdIncludesCPP, bool){ cl::CommandFlag{ "nostdinc++" }, true };
    COMMAND_LINE_OPTION(Verbose, bool)
    {
        cl::CommandFlag{ "v" }
    };

    // Actions
    COMMAND_LINE_OPTION(PreprocessOnly, bool)
    {
        cl::CommandFlag{ "E" },
    };

    // Preprocessor flags

    // Dependency file generation
    COMMAND_LINE_OPTION(WriteDependencies, bool)
    {
        cl::CommandFlag{ "MD" },
            true
    };

    COMMAND_LINE_OPTION(DependenciesFile, path)
    {
        cl::CommandFlag{ "MF" }
    };

    COMMAND_LINE_OPTION(PrecompiledHeader, path)
    {
        cl::CommandFlag{ "include-pch" }
    };

    COMMAND_LINE_OPTION(EmitPCH, bool)
    {
        cl::CommandFlag{ "Xclang -emit-pch" },
    };
};
DECLARE_OPTION_SPECIALIZATION(ClangOptions);

struct SW_DRIVER_CPP_API ClangClOptions// : ClangCommonOptions
{
    // Preprocessor flags

    // Include path management
    /*COMMAND_LINE_OPTION(ForcedIncludeFiles, FilesOrdered)
    {
        cl::CommandFlag{ "include" },
        cl::CommandFlagBeforeEachValue{}
    };*/

    COMMAND_LINE_OPTION(Arch, clang::ArchType);

    //COMMAND_LINE_OPTION(MsCompatibility, bool){ cl::CommandFlag{ "fms-compatibility" }, true };
};
DECLARE_OPTION_SPECIALIZATION(ClangClOptions);

// https://gcc.gnu.org/onlinedocs/gcc/Option-Summary.html
struct SW_DRIVER_CPP_API GNUOptions : GNUClangCommonOptions
{
    COMMAND_LINE_OPTION(DisableWarnings, bool)
    {
        cl::CommandFlag{ "w" }, true,
    };

    // Dependency file generation
    COMMAND_LINE_OPTION(WriteDependenciesNearOutputWithoutSystemFiles, bool)
    {
        cl::CommandFlag{ "MMD" },
            true
    };
};
DECLARE_OPTION_SPECIALIZATION(GNUOptions);

struct SW_DRIVER_CPP_API GNUAssemblerOptions
{
    // goes last
    COMMAND_LINE_OPTION(InputFile, path)
    {
        cl::InputDependency{},
    };

    COMMAND_LINE_OPTION(OutputFile, path)
    {
        cl::CommandFlag{ "o" },
        cl::OutputDependency{},
    };
};
DECLARE_OPTION_SPECIALIZATION(GNUAssemblerOptions);

// common for ld and ar
// https://linux.die.net/man/1/ld
struct SW_DRIVER_CPP_API GNULibraryToolOptions
{
};

// ld
// https://docs.microsoft.com/en-us/cpp/build/reference/linker-options
struct SW_DRIVER_CPP_API GNULinkerOptions
{
    COMMAND_LINE_OPTION(rdynamic, bool)
    {
        cl::CommandFlag{ "rdynamic" },
        true
    };

    COMMAND_LINE_OPTION(InputFiles, Files)
    {
        cl::InputDependency{},
    };

    COMMAND_LINE_OPTION(InputLibraryDependencies, FilesOrdered)
    {
        cl::InputDependency{},
    };

    // these are ordered
    COMMAND_LINE_OPTION(LinkDirectories, FilesOrdered)
    {
        cl::CommandFlag{ "L" },
            cl::CommandFlagBeforeEachValue{},
    };

    COMMAND_LINE_OPTION(LinkLibraries, FilesOrdered)
    {
        cl::CommandFlag{ "l" },
            cl::CommandFlagBeforeEachValue{},
            cl::InputDependency{},
    };

    /*COMMAND_LINE_OPTION(ImportLibrary, path)
    {
        cl::CommandFlag{ "IMPLIB:" },
        cl::IntermediateFile{},
    };*/

    COMMAND_LINE_OPTION(Output, path)
    {
        cl::CommandFlag{ "o" },
            cl::OutputDependency{},
            cl::SeparatePrefix{},
    };

    COMMAND_LINE_OPTION(PositionIndependentCode, bool)
    {
        cl::CommandFlag{ "fPIC" }, true
    };

    COMMAND_LINE_OPTION(SharedObject, bool)
    {
        cl::CommandFlag{ "shared" },
    };
};
DECLARE_OPTION_SPECIALIZATION(GNULinkerOptions);

// https://linux.die.net/man/1/ar
struct SW_DRIVER_CPP_API GNULibrarianOptions
{
    COMMAND_LINE_OPTION(Options, bool)
    {
        cl::CommandFlag{ "rcs" },
            true
    };

    COMMAND_LINE_OPTION(Output, path)
    {
        cl::OutputDependency{},
    };

    COMMAND_LINE_OPTION(InputFiles, Files)
    {
        cl::InputDependency{},
    };
};
DECLARE_OPTION_SPECIALIZATION(GNULibrarianOptions);

}
