workspace "PopHead"
    architecture "x86"
    location "../../"
    startproject "PopHead"
    
    configurations{
        "Debug",
        "Release",
        "Distribution",
        "Tests"
    }

project "PopHead"
    location "../../VS_Projects"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++17"
    
    targetdir ("../../bin/%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}/%{prj.name}")
	objdir ("../../bin-obj/%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}/%{prj.name}")
    
    debugdir "%{wks.location}"
    
    includedirs{
        "../../src",
        "../../vendor/SFML_2.5.1/include"
    }

    libdirs{"../../vendor/SFML_2.5.1/lib-VisualStudio"}

    files{
        "../../src/**.hpp",
        "../../src/**.cpp",
        "../../src/**.inl"
    }

    links{
        "opengl32.lib",
        "winmm.lib",
        "gdi32.lib",
        "freetype.lib",
        "flac.lib",
        "vorbisenc.lib",
        "vorbisfile.lib",
        "vorbis.lib",
        "ogg.lib",
        "openal32.lib"
    }

    defines{"SFML_STATIC"}

    filter "configurations:Debug"
        symbols "On"

        links{
            "sfml-graphics-s-d",
            "sfml-audio-s-d",
            "sfml-network-s-d",
            "sfml-window-s-d",
            "sfml-system-s-d"
        }

    filter{"configurations:Release or Distribution"}
        optimize "On"

        links{
            "sfml-graphics-s",
            "sfml-audio-s", 
            "sfml-network-s",
            "sfml-window-s",
            "sfml-system-s"
        }

    filter{"configurations:Distribution"}
        defines{"PH_DISTRIBUTION"}

    filter "system:Windows"
        defines{"PH_WINDOWS"}

    filter "system:Unix"
        defines{"PH_LINUX"}

    filter "system:Mac"
        defines{"PH_MAC"}

    filter{}

project "Tests"
    location "../../VS_Projects"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++17"

    targetdir ("../../bin/%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}/%{prj.name}")
	objdir ("../../bin-obj/%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}/%{prj.name}")
    
    debugdir "%{wks.location}"
    
    includedirs{
        "../../src",
        "../../vendor/SFML_2.5.1/include",
        "../../vendor/catch2"
    }

    libdirs{"../../vendor/SFML_2.5.1/lib-VisualStudio"}

    files{
        "../../src/**.hpp",
        "../../src/**.cpp",
        "../../src/**.inl",
        "../../tests/**.hpp",
        "../../tests/**.cpp",
        "../../tests/**.inl"
    }
    
    removefiles{
        "../../src/main.cpp"
    }

    links{
        "opengl32.lib",
        "winmm.lib",
        "gdi32.lib",
        "freetype.lib",
        "flac.lib",
        "vorbisenc.lib",
        "vorbisfile.lib",
        "vorbis.lib",
        "ogg.lib",
        "openal32.lib"
    }

    defines{"SFML_STATIC"}

    filter "configurations:Debug or Tests"
        symbols "On"

        links{
            "sfml-graphics-s-d",
            "sfml-audio-s-d",
            "sfml-network-s-d",
            "sfml-window-s-d",
            "sfml-system-s-d"
        }

    filter{"configurations:Release or Distribution"}
        optimize "On"

        links{
            "sfml-graphics-s",
            "sfml-audio-s", 
            "sfml-network-s",
            "sfml-window-s",
            "sfml-system-s"
        }
        
    filter{"configurations:Tests"}
        defines{"PH_TESTS"}
        
    filter "system:Windows"
        defines{"PH_WINDOWS"}

    filter "system:Unix"
        defines{"PH_LINUX"}

    filter "system:Mac"
        defines{"PH_MAC"}

    filter{}
    
printf("For now PopHead supports only new Visual Studio versions and Codeblocks.")
printf("If you have any problems with Premake or compiling PopHead contact Grzegorz \"Czapa\" Bednorz.")