/*
 * SW - Build System and Package Manager
 * Copyright (C) 2017-2019 Egor Pugin
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "../commands.h"
#include "../inserts.h"

#include <boost/algorithm/string.hpp>
#include <primitives/emitter.h>
#include <sw/core/input.h>
#include <sw/core/sw_context.h>
#include <sw/driver/build_settings.h>
#include <sw/driver/types.h>

struct CMakeEmitter : primitives::Emitter
{
    void if_(const String &s)
    {
        addLine("if (" + s + ")");
        increaseIndent();
    }

    void elseif(const String &s)
    {
        decreaseIndent();
        emptyLines(0);
        addLine("elseif(" + s + ")");
        increaseIndent();
    }

    void else_()
    {
        decreaseIndent();
        emptyLines(0);
        addLine("else()");
        increaseIndent();
    }

    void endif()
    {
        decreaseIndent();
        emptyLines(0);
        addLine("endif()");
        emptyLines();
    }
};

static String toCmakeString(sw::ConfigurationType t)
{
    switch (t)
    {
    case sw::ConfigurationType::Debug: return "DEBUG";
    case sw::ConfigurationType::MinimalSizeRelease: return "MINSIZEREL";
    case sw::ConfigurationType::ReleaseWithDebugInformation: return "RELWITHDEBINFO";
    case sw::ConfigurationType::Release: return "RELEASE";
    default:
        SW_UNIMPLEMENTED;
    }
}

static String toCmakeStringCapital(sw::ConfigurationType t)
{
    auto s = toCmakeString(t);
    boost::to_lower(s);
    s[0] = toupper(s[0]);
    return s;
}

static String pkg2string(const String &p)
{
    return boost::to_lower_copy(p);
}

static String pkg2string(const sw::PackagePath &p)
{
    return pkg2string(p.toString());
}

static String pkg2string(const sw::PackageId &p)
{
    return pkg2string(p.toString());
}

const String &getCmakeConfig();

int getSwCmakeConfigVersion()
{
    auto &cfg = getCmakeConfig();
    static std::regex r("set\\(SW_CMAKE_VERSION (\\d+)\\)");
    std::smatch m;
    if (!std::regex_search(cfg, m, r))
        return 0;
    return std::stoi(m[1].str());
}

SUBCOMMAND_DECL(integrate)
{
    bool cygwin = false;
    auto fix_path = [&cygwin](const auto &p)
    {
        if (!cygwin)
            return p;
        if (p.size() < 3 || p[1] != ':')
            return p;
        String s2 = "x";
        s2[0] = tolower(p[0]);
        if (p[2] != '/')
            s2 += "/";
        s2 = "cygdrive/" + s2;
        return "/"s + s2 + p.substr(2);
    };

    auto create_build = [this, &cygwin](const Strings &lines, const Strings &configs = {})
    {
        auto build = getContext().createBuild();
        auto &b = *build;

        auto settings = createSettings();
        if (settings.size() > 1)
            throw SW_RUNTIME_ERROR("size() must be 1");

        cygwin = settings[0]["os"]["kernel"] == "org.cygwin";

        for (auto &l : lines)
        {
            for (auto c : l)
            {
                if (isalpha(c) && isupper(c))
                    throw SW_RUNTIME_ERROR("Package name must be in lower case for now. Sorry for inconvenience.");
            }
            for (auto &i : getContext().addInput(l))
            {
                sw::InputWithSettings s(*i);
                if (!configs.empty())
                {
                    for (String cfg : configs)
                    {
                        auto cfgl = boost::to_lower_copy(cfg);
                        settings[0]["native"]["configuration"] = cfgl;
                        s.addSettings(settings[0]);
                    }
                }
                else
                {
                    s.addSettings(settings[0]);
                }
                b.addInput(s);
            }
        }
        b.loadInputs();
        b.setTargetsToBuild();
        b.resolvePackages();
        b.loadPackages();
        b.prepare();

        return build;
    };

    if (!getOptions().options_integrate.integrate_cmake_deps.empty())
    {
        if (getOptions().options_integrate.cmake_file_version < getSwCmakeConfigVersion())
            throw SW_RUNTIME_ERROR("Old cmake integration file detected. Run 'sw setup' to upgrade it.");

        const Strings configs
        {
            "Debug",
            "MinimalSizeRelease",
            "ReleaseWithDebugInformation",
            "Release",
        };

        auto lines = read_lines(getOptions().options_integrate.integrate_cmake_deps);
        auto build = create_build(lines, configs);
        auto &b = *build;

        CMakeEmitter ctx;
        ctx.addLine("#");
        ctx.addLine("# sw autogenerated file");
        ctx.addLine("#");
        ctx.emptyLines();

        // targets
        ctx.addLine("# targets");
        for (auto &[pkg, tgts] : b.getTargets())
        {
            if (tgts.empty())
            {
                continue;
                //throw SW_RUNTIME_ERROR("No targets in " + pkg2string(pkg));
            }
            // filter out predefined targets
            if (b.getContext().getPredefinedTargets().find(pkg) != b.getContext().getPredefinedTargets().end())
                continue;

            auto &t = **tgts.begin();
            const auto &s = t.getInterfaceSettings();

            if (s["type"] == "native_executable")
                continue;

            ctx.if_("NOT TARGET " + pkg2string(pkg));

            // tgt
            auto st = "STATIC";
            if (s["type"] == "native_static_library")
                ;
            else if (s["type"] == "native_shared_library")
                st = "SHARED";
            if (s["header_only"] == "true")
                st = "INTERFACE";
            ctx.addLine("add_library(" + pkg2string(pkg) + " " + st + " IMPORTED GLOBAL)");
            ctx.emptyLines();

            // imported configs
            for (auto &tgt : tgts)
            {
                const auto &s = tgt->getInterfaceSettings();
                sw::BuildSettings bs(tgt->getSettings());

                auto cmake_cfg = "$<$<CONFIG:" + toCmakeStringCapital(bs.Native.ConfigurationType) + ">: \"";
                auto cmake_cfg_end = "\" >";

                // defs
                ctx.increaseIndent("target_compile_definitions(" + pkg2string(pkg) + " INTERFACE");
                for (auto &[k,v] : s["definitions"].getSettings())
                {
                    ctx.addLine(cmake_cfg);
                    if (v.getValue().empty())
                        ctx.addText(k);
                    else
                        ctx.addText(k + "=" + primitives::command::Argument::quote(v.getValue(), primitives::command::QuoteType::Escape));
                    ctx.addText(cmake_cfg_end);
                }
                ctx.decreaseIndent(")");
                ctx.emptyLines();

                // idirs
                ctx.increaseIndent("target_include_directories(" + pkg2string(pkg) + " INTERFACE");
                for (auto &d : s["include_directories"].getArray())
                    ctx.addLine(cmake_cfg + fix_path(std::get<sw::TargetSetting::Value>(d)) + cmake_cfg_end);
                ctx.decreaseIndent(")");
                ctx.emptyLines();

                if (s["header_only"] != "true")
                {
                    // libs
                    ctx.increaseIndent("target_link_libraries(" + pkg2string(pkg) + " INTERFACE");
                    for (auto &d : s["link_libraries"].getArray())
                        ctx.addLine(cmake_cfg + fix_path(std::get<sw::TargetSetting::Value>(d)) + cmake_cfg_end);
                    for (auto &d : s["system_link_libraries"].getArray())
                        ctx.addLine(cmake_cfg + fix_path(std::get<sw::TargetSetting::Value>(d)) + cmake_cfg_end);
                    ctx.decreaseIndent(")");
                    ctx.emptyLines();

                    // not needed?
                    ctx.addLine("set_property(TARGET " + pkg2string(pkg) + " APPEND PROPERTY IMPORTED_CONFIGURATIONS " +
                        toCmakeString(bs.Native.ConfigurationType) + ")");
                    // props
                    ctx.increaseIndent("set_target_properties(" + pkg2string(pkg) + " PROPERTIES");

                    // TODO: detect C/CXX language from target files
                    //ctx.addLine("IMPORTED_LINK_INTERFACE_LANGUAGES_" + toCmakeString(bs.Native.ConfigurationType) + " \"CXX\"");

                    // IMPORTED_LOCATION = path to .dll/.so or static .lib/.a
                    ctx.addLine("IMPORTED_LOCATION_" + toCmakeString(bs.Native.ConfigurationType) + " \"" +
                        fix_path(normalize_path(s[st == "SHARED" ? "output_file" : "import_library"].getValue())) + "\"");
                    // IMPORTED_IMPLIB = path to .lib (import)
                    ctx.addLine("IMPORTED_IMPLIB_" + toCmakeString(bs.Native.ConfigurationType) + " \"" +
                        fix_path(normalize_path(s["import_library"].getValue())) + "\"");

                    ctx.decreaseIndent(")");
                    ctx.emptyLines();
                }
            }
            //

            ctx.emptyLines();

            // build dep
            ctx.addLine("add_dependencies(" + pkg2string(pkg) + " sw_build_dependencies)");
            ctx.emptyLines();

            if (pkg.getVersion().isVersion())
            {
                for (auto i = pkg.getVersion().getLevel() - 1; i >= 0; i--)
                {
                    if (i)
                        ctx.addLine("add_library(" + pkg2string(pkg.getPath()) + "-" + pkg.getVersion().toString(i) + " ALIAS " + pkg2string(pkg) + ")");
                    else
                        ctx.addLine("add_library(" + pkg2string(pkg.getPath()) + " ALIAS " + pkg2string(pkg) + ")");
                }
            }

            ctx.endif();
        }

        // deps
        ctx.addLine("# dependencies");
        for (auto &[pkg, tgts] : b.getTargets())
        {
            if (tgts.empty())
            {
                continue;
                //throw SW_RUNTIME_ERROR("No targets in " + pkg2string(pkg));
            }
            // filter out predefined targets
            if (b.getContext().getPredefinedTargets().find(pkg) != b.getContext().getPredefinedTargets().end())
                continue;

            auto &t = **tgts.begin();
            const auto &s = t.getInterfaceSettings();

            if (s["type"] == "native_executable")
                continue;

            for (auto &[k,v] : s["dependencies"]["link"].getSettings())
                ctx.addLine("target_link_libraries(" + pkg2string(pkg) + " INTERFACE " + k + ")");
        }
        write_file_if_different(getOptions().options_integrate.integrate_cmake_deps.parent_path() / "CMakeLists.txt", ctx.getText());

        return;
    }

    if (!getOptions().options_integrate.integrate_waf_deps.empty())
    {
        auto lines = read_lines(getOptions().options_integrate.integrate_waf_deps);
        auto build = create_build(lines);
        auto &b = *build;

        // https://waf.io/apidocs/_modules/waflib/Tools/c_config.html#parse_flags
        primitives::Emitter ctx;

        ctx.increaseIndent("def configure(ctx):");

        for (auto &[pkg, tgts] : b.getTargets())
        {
            if (tgts.empty())
            {
                continue;
                //throw SW_RUNTIME_ERROR("No targets in " + pkg2string(pkg));
            }
            // filter out predefined targets
            if (b.getContext().getPredefinedTargets().find(pkg) != b.getContext().getPredefinedTargets().end())
                continue;

            auto &t = **tgts.begin();
            const auto &s = t.getInterfaceSettings();

            if (s["type"] == "native_executable")
                continue;

            ctx.addLine("# " + pkg2string(pkg));
            ctx.increaseIndent("for lib in [");
            for (auto i = pkg.getVersion().getLevel(); i >= 0; i--)
            {
                if (i)
                    ctx.addLine("'" + pkg2string(pkg.getPath()) + "-" + pkg.getVersion().toString(i) + "',");
                else
                    ctx.addLine("'" + pkg2string(pkg.getPath()) + "',");
            }
            ctx.decreaseIndent("]:");
            ctx.increaseIndent();

            using tgt_type = std::pair<sw::PackageId, sw::TargetSettings>;
            using f_param = const tgt_type &;
            std::function<void(f_param)> process;
            std::set<tgt_type> visited;
            process = [&process, &s, &b, &ctx, &visited](f_param nt)
            {
                if (visited.find(nt) != visited.end())
                    return;
                visited.insert(nt);

                auto t = b.getTargets().find(nt.first, nt.second);
                if (!t)
                    throw SW_RUNTIME_ERROR("no such target: " + pkg2string(nt.first));

                const auto &s = t->getInterfaceSettings();

                auto remove_ext = [](const path &p)
                {
                    return p.parent_path() / p.stem();
                };

                ctx.addLine("ctx.parse_flags('-l" + normalize_path(remove_ext(s["import_library"].getValue())) + "', lib)");

                // defs
                for (auto &[k,v] : s["definitions"].getSettings())
                {
                    if (v.getValue().empty())
                        ctx.addLine("ctx.parse_flags('-D" + k + "', lib)");
                    else
                        ctx.addLine("ctx.parse_flags('-D" + k + "=" + primitives::command::Argument::quote(v.getValue(), primitives::command::QuoteType::Escape) + "', lib)");
                }

                // idirs
                for (auto &d : s["include_directories"].getArray())
                    ctx.addLine("ctx.parse_flags('-I" + normalize_path(std::get<sw::TargetSetting::Value>(d)) + "', lib)");

                // libs
                for (auto &d : s["link_libraries"].getArray())
                    ctx.addLine("ctx.parse_flags('-l" + normalize_path(remove_ext(std::get<sw::TargetSetting::Value>(d))) + "', lib)");
                for (auto &d : s["system_link_libraries"].getArray())
                    ctx.addLine("ctx.parse_flags('-l" + normalize_path(remove_ext(std::get<sw::TargetSetting::Value>(d))) + "', lib)");

                // deps
                for (auto &[k,v] : s["dependencies"]["link"].getSettings())
                {
                    process({k, v.getSettings()});
                }
            };
            process({t.getPackage(), t.getSettings()});

            ctx.decreaseIndent();
            ctx.emptyLines();
            //
        }

        write_file_if_different("wscript", ctx.getText());

        return;
    }

    SW_UNIMPLEMENTED;
}
