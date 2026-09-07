package("thorvg")
    set_homepage("https://www.thorvg.org")
    set_description("Thor Vector Graphics is a lightweight portable library used for drawing vector-based scenes and animations including SVG and Lottie. It can be freely utilized across various software platforms and applications to visualize graphical contents.")
    set_license("MIT")

    add_urls("https://github.com/thorvg/thorvg/archive/refs/tags/$(version).tar.gz",
             "https://github.com/thorvg/thorvg.git")

    add_versions("v1.1.0", "73943bcf918c9ffdf11acc53052ba5cb60a7047b1cfb2067afeb62e0c4d9417d")

    add_configs("engines", {description = "Enable Rasterizer Engine in thorvg", default = {"cpu"}, type = "table"})
    add_configs("loaders", {description = "Enable File Loaders", type = "table"})
    add_configs("savers", {description = "Enable File Savers", type = "table"})
    add_configs("tools", {description = "Enable building thorvg tools", type = "table"})
    add_configs("extra", {description = "Enable support for exceptionally advanced features", type = "table"})
    add_configs("threads", {description = "Enable the multi-threading task scheduler in thorvg", default = false, type = "boolean"})
    add_configs("simd", {description = "Enable CPU Vectorization(SIMD) in thorvg", default = false, type = "boolean"})
    add_configs("log", {description = "Enable log message", default = false, type = "boolean"})
    add_configs("partial", {description = "Enable Partial Rendering in thorvg", default = true, type = "boolean"})
    add_configs("file", {description = "Enable File IO calls in thorvg", default = true, type = "boolean"})
    add_configs("c_api", {description = "Enable API bindings", default = false, type = "boolean"})
    if is_plat("wasm") then
        add_configs("shared", {description = "Build shared library.", default = false, type = "boolean", readonly = true})
    end

    if is_plat("linux", "bsd") then
        add_syslinks("pthread", "m")
    end

    add_deps("meson", "ninja")
    if is_plat("windows") then
        add_deps("pkgconf")
    end

    if on_check then
        on_check("android", function (package)
            local ndk = package:toolchain("ndk")
            local ndkver = ndk:config("ndkver")
            assert(ndkver and tonumber(ndkver) > 22, "package(thorvg) require ndk version > 22")
            if package:is_arch("armeabi-v7a") then
                local ndk_sdkver = ndk:config("ndk_sdkver")
                assert(ndk_sdkver and tonumber(ndk_sdkver) > 21, "package(thorvg/armeabi-v7a) require ndk api level > 21")
            end
        end)
    end

    on_load(function (package)
        import("core.base.hashset")

        local loaders = package:config("loaders")
        if loaders then
            local loaders = hashset.from(loaders)
            if loaders then
                local deps = {jpg = "libjpeg-turbo", png = "libpng", webp = "libwebp"}
                if loaders:has("all") then
                    for _, dep in pairs(deps) do
                        package:add("deps", dep)
                    end
                else
                    for name, dep in pairs(deps) do
                        if loaders:has(name) then
                            package:add("deps", dep)
                        end
                    end
                end
            end
        end

        if not package:config("shared") then
            package:add("defines", "TVG_STATIC")
        end
    end)

    on_install(function (package)
        if package:is_plat("mingw") then
            io.replace("src/loaders/svg/tvgSvgLoader.cpp", "float_t", "float", {plain = true})
        end

        local configs = {}
        table.insert(configs, "-Ddefault_library=" .. (package:config("shared") and "shared" or "static"))

        local loaders = package:config("loaders")
        local savers = package:config("savers")
        local tools = package:config("tools")
        local extra = package:config("extra")
        table.insert(configs, "-Dengines=" .. table.concat(package:config("engines"), ","))
        table.insert(configs, "-Dloaders=" .. (loaders and table.concat(loaders, ",") or ""))
        table.insert(configs, "-Dsavers=" .. (savers and table.concat(savers, ",") or ""))
        table.insert(configs, "-Dtools=" .. (tools and table.concat(tools, ",") or ""))
        table.insert(configs, "-Dextra=" .. (extra and table.concat(extra, ",") or ""))
        if package:config("c_api") then
            table.insert(configs, "-Dbindings=capi")
        end
        table.insert(configs, "-Dthreads=" .. (package:config("threads") and "true" or "false"))
        table.insert(configs, "-Dsimd=" .. (package:config("simd") and "true" or "false"))
        table.insert(configs, "-Dlog=" .. (package:config("log") and "true" or "false"))
        table.insert(configs, "-Dpartial=" .. (package:config("partial") and "true" or "false"))
        table.insert(configs, "-Dfile=" .. (package:config("file") and "true" or "false"))
        import("package.tools.meson").install(package, configs)

        local incdir = path.join(package:installdir("include"), "thorvg-1")
        if os.isdir(incdir) then
            os.cp(path.join(incdir, "*"), package:installdir("include"))
            os.rmdir(incdir)
        end
        for _, libfile in ipairs(os.files(path.join(package:installdir("lib"), "libthorvg-1.*"))) do
            os.mv(libfile, (libfile:gsub("libthorvg%-1", "libthorvg")))
        end
    end)

    on_test(function (package)
        assert(package:check_cxxsnippets({test = [[
            #include <thorvg.h>
            void test() {
                auto canvas = tvg::SwCanvas::gen();
            }
        ]]}, {configs = {languages = "c++14"}}))
        if package:config("c_api") then
            assert(package:has_cxxfuncs("tvg_engine_init", {includes = "thorvg_capi.h"}))
        end
    end)
