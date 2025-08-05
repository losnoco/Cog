
if(NOT GME_BUILD_SHARED AND NOT GME_BUILD_STATIC)
    message(FATAL_ERROR "Both shared and static builds disabled. Please enable build of shared and/or static build of the GME.")
endif()

if(GME_ZLIB)
    find_package(ZLIB QUIET)
endif()

# List of source files required by libgme and any emulators
# This is not 100% accurate (Fir_Resampler for instance) but
# you'll be OK.
set(libgme_SRCS Blip_Buffer.cpp
                Blip_Buffer.h
                Classic_Emu.cpp
                Classic_Emu.h
                Data_Reader.cpp
                Data_Reader.h
                Dual_Resampler.cpp
                Dual_Resampler.h
                Effects_Buffer.cpp
                Effects_Buffer.h
                Fir_Resampler.cpp
                Fir_Resampler.h
                gme.cpp
                gme.h
                gme_types.h
                Gme_File.cpp
                Gme_File.h
                M3u_Playlist.cpp
                M3u_Playlist.h
                Multi_Buffer.cpp
                Multi_Buffer.h
                Music_Emu.cpp
                Music_Emu.h
                blargg_common.h
                blargg_config.h
                blargg_endian.h
                blargg_source.h
                )

# Ay_Apu is very popular around here
if(USE_GME_AY OR USE_GME_KSS)
    list(APPEND libgme_SRCS
                Ay_Apu.cpp
                Ay_Apu.h
        )
endif()

# so is Ym2612_Emu
if(USE_GME_VGM OR USE_GME_GYM)
    if(GME_YM2612_EMU STREQUAL "Nuked")
        add_definitions(-DVGM_YM2612_NUKED)
        list(APPEND libgme_SRCS
                    Ym2612_Nuked.cpp
                    Ym2612_Nuked.h
            )
        message(STATUS "VGM/GYM: Nuked OPN2 emulator will be used")
    elseif(GME_YM2612_EMU STREQUAL "MAME")
        add_definitions(-DVGM_YM2612_MAME)
        list(APPEND libgme_SRCS
                    Ym2612_MAME.cpp
                    Ym2612_MAME.h
            )
        message(STATUS "VGM/GYM: MAME YM2612 emulator will be used")
    else()
        add_definitions(-DVGM_YM2612_GENS)
        list(APPEND libgme_SRCS
                    Ym2612_GENS.cpp
                    Ym2612_GENS.h
            )
        message(STATUS "VGM/GYM: GENS 2.10 emulator will be used")
    endif()
endif()

# But none are as popular as Sms_Apu
if(USE_GME_VGM OR USE_GME_GYM OR USE_GME_KSS)
    list(APPEND libgme_SRCS
                Sms_Apu.cpp
                Sms_Apu.h
        )
endif()

if(USE_GME_AY)
    list(APPEND libgme_SRCS
              # Ay_Apu.cpp included earlier
                Ay_Cpu.cpp
                Ay_Cpu.h
                Ay_Emu.cpp
                Ay_Emu.h
        )
endif()

if(USE_GME_GBS)
    list(APPEND libgme_SRCS
                Gb_Apu.cpp
                Gb_Apu.h
                Gb_Cpu.cpp
                Gb_Cpu.h
                Gb_Oscs.cpp
                Gb_Oscs.h
                Gbs_Emu.cpp
                Gbs_Emu.h
                gb_cpu_io.h
        )
endif()

if(USE_GME_GYM)
    list(APPEND libgme_SRCS
              # Sms_Apu.cpp included earlier
              # Ym2612_Emu.cpp included earlier
                Gym_Emu.cpp
                Gym_Emu.h
        )
endif()

if(USE_GME_HES)
    list(APPEND libgme_SRCS
                Hes_Apu.cpp
                Hes_Apu.h
                Hes_Cpu.cpp
                Hes_Cpu.h
                Hes_Emu.cpp
                Hes_Emu.h
                hes_cpu_io.h
        )
endif()

if(USE_GME_KSS)
    list(APPEND libgme_SRCS
              # Ay_Apu.cpp included earlier
              # Sms_Apu.cpp included earlier
                Kss_Cpu.cpp
                Kss_Cpu.h
                Kss_Emu.cpp
                Kss_Emu.h
                Kss_Scc_Apu.cpp
                Kss_Scc_Apu.h
        )
endif()

if(USE_GME_NSF OR USE_GME_NSFE)
    list(APPEND libgme_SRCS
                Nes_Apu.cpp
                Nes_Apu.h
                Nes_Cpu.cpp
                Nes_Cpu.h
                Nes_Fme7_Apu.cpp
                Nes_Fme7_Apu.h
                Nes_Namco_Apu.cpp
                Nes_Namco_Apu.h
                Nes_Oscs.cpp
                Nes_Oscs.h
                Nes_Vrc6_Apu.cpp
                Nes_Vrc6_Apu.h
                Nes_Fds_Apu.cpp
                Nes_Fds_Apu.h
                Nes_Vrc7_Apu.cpp
                Nes_Vrc7_Apu.h
                ext/emu2413.c
                ext/emu2413.h
                ext/panning.c
                ext/panning.h
                ext/emutypes.h
                ext/2413tone.h
                ext/vrc7tone.h
                Nsf_Emu.cpp
                Nsf_Emu.h
        )
endif()

if (USE_GME_NSFE)
    set(libgme_SRCS ${libgme_SRCS}
                Nsfe_Emu.cpp
                Nsfe_Emu.h
        )
endif()

if (USE_GME_SAP)
    set(libgme_SRCS ${libgme_SRCS}
                Sap_Apu.cpp
                Sap_Cpu.cpp
                Sap_Emu.cpp
                sap_cpu_io.h
        )
endif()

if (USE_GME_SPC)
    set(libgme_SRCS ${libgme_SRCS}
                ../higan/processor/spc700/spc700.cpp
                ../higan/smp/memory.cpp
                ../higan/smp/timing.cpp
                ../higan/smp/smp.cpp
                ../higan/dsp/dsp.cpp
                ../higan/dsp/SPC_DSP.cpp
                Snes_Spc.cpp
                Snes_Spc.h
                Spc_Cpu.cpp
                Spc_Cpu.h
                Spc_Dsp.cpp
                Spc_Dsp.h
                Spc_Emu.cpp
                Spc_Emu.h
                Spc_Filter.cpp
                Spc_Filter.h
                Bml_Parser.cpp
                Bml_Parser.h
                Spc_Sfm.cpp
                Spc_Sfm.h
        )
    if (GME_SPC_ISOLATED_ECHO_BUFFER)
        add_definitions(-DSPC_ISOLATED_ECHO_BUFFER)
    endif()
endif()

if (USE_GME_VGM)
    set(libgme_SRCS ${libgme_SRCS}
              # Sms_Apu.cpp included earlier
              # Ym2612_Emu.cpp included earlier
                Vgm_Emu.cpp
                Vgm_Emu.h
                Vgm_Emu_Impl.cpp
                Vgm_Emu_Impl.h
                Ym2413_Emu.cpp
                Ym2413_Emu.h
        )
endif()

# These headers are part of the generic gme interface.
set (EXPORTED_HEADERS gme.h)

# while building a macOS framework, exported headers must be in the source
# list, or the header files aren't copied to the bundle.
if(GME_BUILD_FRAMEWORK)
    set(libgme_SRCS ${libgme_SRCS} ${EXPORTED_HEADERS})
endif()

# Extract symbols from gme.exports
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/gme.exports" gme_exports_strings)
set(gme_exports)
foreach(gme_export_string ${gme_exports_strings})
    string(STRIP "${gme_export_string}" gme_export_string)
    string(SUBSTRING "${gme_export_string}" 0 1 gme_export_first_char)
    if(gme_export_string AND NOT gme_export_first_char STREQUAL "#")
        list(APPEND gme_exports ${gme_export_string})
    endif()
endforeach()


add_library(gme_deps INTERFACE)

## FIXME: Properly find the C++ library !!!
set(PC_LIBS -lstdc++)

if(GME_ZLIB)
    if(ZLIB_FOUND)
        message(STATUS "ZLib library located, compressed file formats will be supported")
        target_compile_definitions(gme_deps INTERFACE HAVE_ZLIB_H)
        target_link_libraries(gme_deps INTERFACE ZLIB::ZLIB)
        # Is not to be installed though
        list(APPEND PC_LIBS -lz) # for libgme.pc
    else()
        message(STATUS "** ZLib library not found, disabling support for compressed formats such as VGZ")
    endif()
else()
    message(STATUS "Zlib-Compressed formats excluded")
endif()

if(NOT MSVC)
    # Link with -no-undefined, if available
    if(NOT APPLE AND NOT CMAKE_SYSTEM_NAME MATCHES ".*OpenBSD.*")
        cmake_push_check_state()
        set(CMAKE_REQUIRED_FLAGS "-Wl,-no-undefined")
            check_cxx_source_compiles("int main(void) { return 0;}" LINKER_SUPPORTS_NO_UNDEFINED)
        cmake_pop_check_state()
    endif()

    # Link to libm, if necessary
    check_cxx_source_compiles("
        #include <math.h>
        int main(int argc, char** argv) {
            return (int) pow(argc, 2.5);
        }" LIBM_NOT_NEEDED)
    if(NOT LIBM_NOT_NEEDED)
        cmake_push_check_state()
        set(CMAKE_REQUIRED_LIBRARIES "-lm")
        check_cxx_source_compiles("
            #include <math.h>
            int main(int argc, char** argv) {
                return (int) pow(argc, 2.5);
            }" HAVE_LIBM)
        cmake_pop_check_state()
        if(HAVE_LIBM)
            list(APPEND PC_LIBS -lm) # for libgme.pc
        endif()
    endif()
endif()

# Add a version script to hide the c++ STL
if(APPLE)
    set(gme_syms "")
    foreach(gme_export ${gme_exports})
        set(gme_syms "${gme_syms}_${gme_export}\n")
    endforeach()

    # Use an intermediate 'gme_generated.sym' file to avoid a relink every time CMake runs
    set(temporary_sym_file "${CMAKE_CURRENT_BINARY_DIR}/gme_generated.sym")
    set(generated_sym_file "${CMAKE_CURRENT_BINARY_DIR}/gme.sym")
    file(WRITE "${temporary_sym_file}" "${gme_syms}")
    execute_process(COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${temporary_sym_file}" "${generated_sym_file}")
elseif(UNIX)
    set(gme_syms "{\n  global:\n")
    foreach(gme_export ${gme_exports})
        set(gme_syms "${gme_syms}    ${gme_export};\n")
    endforeach()
    set(gme_syms "${gme_syms}  local: *;\n};\n")

    # Use an intermediate 'gme_generated.sym' file to avoid a relink every time CMake runs
    set(temporary_sym_file "${CMAKE_CURRENT_BINARY_DIR}/gme_generated.sym")
    set(generated_sym_file "${CMAKE_CURRENT_BINARY_DIR}/gme.sym")
    file(WRITE "${temporary_sym_file}" "${gme_syms}")
    execute_process(COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${temporary_sym_file}" "${generated_sym_file}")

    cmake_push_check_state()
    set(CMAKE_REQUIRED_FLAGS "-Wl,-version-script='${generated_sym_file}'")
    check_cxx_source_compiles("int main() { return 0;}" LINKER_SUPPORTS_VERSION_SCRIPT)
    cmake_pop_check_state()
endif()


set(INSTALL_TARGETS)

# Shared properties for shared and static builds of libGME
macro(set_gme_props gme_target)
    list(APPEND INSTALL_TARGETS ${gme_target})

    set_property(TARGET ${gme_target} PROPERTY C_VISIBILITY_PRESET "hidden")
    set_property(TARGET ${gme_target} PROPERTY VISIBILITY_INLINES_HIDDEN TRUE)
    set_property(TARGET ${gme_target} PROPERTY CXX_VISIBILITY_PRESET "hidden")

    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_definitions(${gme_target} PRIVATE LIBGME_VISIBILITY)
    endif()

    target_compile_definitions(${gme_target} PRIVATE GEN_TYPES_H)
    if(WORDS_BIGENDIAN)
        target_compile_definitions(${gme_target} PRIVATE BLARGG_BIG_ENDIAN=1)
    else()
        target_compile_definitions(${gme_target} PRIVATE BLARGG_LITTLE_ENDIAN=1)
    endif()

    # Try to protect against undefined behavior from signed integer overflow
    # This has caused miscompilation of code already and there are other
    # potential uses; see https://bitbucket.org/mpyne/game-music-emu/issues/18/
    #                    (https://github.com/libgme/game-music-emu/issues/20 )
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(${gme_target} PRIVATE -fwrapv)
    endif()

    target_include_directories(${gme_target}
        PRIVATE "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"  # For gen_types.h
        INTERFACE "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>"
        INTERFACE "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
    )

    if(NOT MSVC)
        # Link with -no-undefined, if available
        if(APPLE)
            set_property(TARGET ${gme_target} APPEND_STRING PROPERTY LINK_FLAGS " -Wl,-undefined,error")
        elseif(NOT CMAKE_SYSTEM_NAME MATCHES ".*OpenBSD.*" AND LINKER_SUPPORTS_NO_UNDEFINED)
            set_property(TARGET ${gme_target} APPEND_STRING PROPERTY LINK_FLAGS " -Wl,-no-undefined")
        endif()
    endif()

endmacro()

# Building the Shared version of GME
if(GME_BUILD_SHARED)
    add_library(gme_shared SHARED ${libgme_SRCS})
    set_target_properties(gme_shared PROPERTIES OUTPUT_NAME gme)

    if(WIN32)
        set_property(TARGET gme_shared PROPERTY DEFINE_SYMBOL BLARGG_BUILD_DLL)
    endif()

    set_gme_props(gme_shared)

    # Add a version script to hide the c++ STL
    if(APPLE)
        set_property(TARGET gme_shared APPEND_STRING PROPERTY LINK_FLAGS " -Wl,-exported_symbols_list,'${generated_sym_file}'")
        set_property(TARGET gme_shared APPEND PROPERTY LINK_DEPENDS "${generated_sym_file}")
    elseif(UNIX AND LINKER_SUPPORTS_VERSION_SCRIPT)
        set_property(TARGET gme_shared APPEND_STRING PROPERTY LINK_FLAGS " -Wl,-version-script='${generated_sym_file}'")
        set_property(TARGET gme_shared APPEND PROPERTY LINK_DEPENDS "${generated_sym_file}")
    endif()

    target_link_libraries(gme_shared PRIVATE gme_deps)

    if(HAVE_LIBM)
        target_link_libraries(gme_shared PRIVATE -lm)
    endif()

    # The version is the release.  The "soversion" is the API version.  As long
    # as only build fixes are performed (i.e. no backwards-incompatible changes
    # to the API), the SOVERSION should be the same even when bumping up VERSION.
    # The way gme.h is designed, SOVERSION should very rarely be bumped, if ever.
    # Hopefully the API can stay compatible with old versions.
    set_target_properties(gme_shared PROPERTIES VERSION ${GME_VERSION} SOVERSION 0)

    # macOS framework build
    if(GME_BUILD_FRAMEWORK)
        set_target_properties(gme_shared
            PROPERTIES FRAMEWORK TRUE
                       FRAMEWORK_VERSION A
                       MACOSX_FRAMEWORK_IDENTIFIER net.mpyne.gme
                       VERSION ${GME_VERSION}
                       SOVERSION 0
                       PUBLIC_HEADER "${EXPORTED_HEADERS}")
    endif()
endif()

# Building the Static version of GME
if(GME_BUILD_STATIC)
    # Static build.
    add_library(gme_static STATIC ${libgme_SRCS})

    # Static builds need to find static zlib (and static forms of other needed
    # libraries.  Ensure CMake looks only for static libs if we're doing a static
    # build.  See https://stackoverflow.com/a/44738756
    if(MSVC)
        set(CMAKE_FIND_LIBRARY_SUFFIXES ".lib")
    else()
        set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
    endif()

    if(MSVC AND GME_BUILD_SHARED)
        set_target_properties(gme_static PROPERTIES OUTPUT_NAME gme-static)
    else()
        set_target_properties(gme_static PROPERTIES OUTPUT_NAME gme)
    endif()

    set_gme_props(gme_static)

    target_link_libraries(gme_static PUBLIC gme_deps)

    if(HAVE_LIBM)
        target_link_libraries(gme_static PUBLIC -lm)
    endif()
endif()

if(GME_BUILD_SHARED)
    add_library(gme::gme ALIAS gme_shared)
else()
    add_library(gme::gme ALIAS gme_static)
endif()


install(TARGETS ${INSTALL_TARGETS}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}  # DLL platforms
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}  # DLL platforms
        FRAMEWORK DESTINATION /Library/Frameworks)   # macOS framework


# Run during cmake phase, so this is available during make
configure_file(gme_types.h.in gen_types.h @ONLY)

set(TMP_PC_LIBS "")
foreach(PC_LIB ${PC_LIBS})
    if(NOT "${TMP_PC_LIBS}" STREQUAL "")
        set(TMP_PC_LIBS "${TMP_PC_LIBS} ${PC_LIB}")
    else()
        set(TMP_PC_LIBS "${PC_LIB}")
    endif()
endforeach()
set(PC_LIBS "${TMP_PC_LIBS}")
unset(TMP_PC_LIBS)

configure_file(libgme.pc.in libgme.pc @ONLY)

install(FILES ${EXPORTED_HEADERS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/gme)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/libgme.pc DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
