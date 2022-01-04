# List of source files required by libgme and any emulators
# This is not 100% accurate (Fir_Resampler for instance) but
# you'll be OK.
set(libgme_SRCS Blip_Buffer.cpp
                Classic_Emu.cpp
                Data_Reader.cpp
                Dual_Resampler.cpp
                Effects_Buffer.cpp
                Fir_Resampler.cpp
                gme.cpp
                Gme_File.cpp
                M3u_Playlist.cpp
                Multi_Buffer.cpp
                Music_Emu.cpp
                )

find_package(ZLIB QUIET)

# Ay_Apu is very popular around here
if (USE_GME_AY OR USE_GME_KSS)
    set(libgme_SRCS ${libgme_SRCS}
                Ay_Apu.cpp
        )
endif()

# so is Ym2612_Emu
if (USE_GME_VGM OR USE_GME_GYM)
    if(GME_YM2612_EMU STREQUAL "Nuked")
        add_definitions(-DVGM_YM2612_NUKED)
        set(libgme_SRCS ${libgme_SRCS}
                    Ym2612_Nuked.cpp
            )
        message("VGM/GYM: Nuked OPN2 emulator will be used")
    elseif(GME_YM2612_EMU STREQUAL "MAME")
        add_definitions(-DVGM_YM2612_MAME)
        set(libgme_SRCS ${libgme_SRCS}
                    Ym2612_MAME.cpp
            )
        message("VGM/GYM: MAME YM2612 emulator will be used")
    else()
        add_definitions(-DVGM_YM2612_GENS)
        set(libgme_SRCS ${libgme_SRCS}
                    Ym2612_GENS.cpp
            )
        message("VGM/GYM: GENS 2.10 emulator will be used")
    endif()
endif()

# But none are as popular as Sms_Apu
if (USE_GME_VGM OR USE_GME_GYM OR USE_GME_KSS)
    set(libgme_SRCS ${libgme_SRCS}
                Sms_Apu.cpp
        )
endif()

if (USE_GME_AY)
    set(libgme_SRCS ${libgme_SRCS}
              # Ay_Apu.cpp included earlier
                Ay_Cpu.cpp
                Ay_Emu.cpp
        )
endif()

if (USE_GME_GBS)
    set(libgme_SRCS ${libgme_SRCS}
                Gb_Apu.cpp
                Gb_Cpu.cpp
                Gb_Oscs.cpp
                Gbs_Emu.cpp
        )
endif()

if (USE_GME_GYM)
    set(libgme_SRCS ${libgme_SRCS}
              # Sms_Apu.cpp included earlier
              # Ym2612_Emu.cpp included earlier
                Gym_Emu.cpp
        )
endif()

if (USE_GME_HES)
    set(libgme_SRCS ${libgme_SRCS}
                Hes_Apu.cpp
                Hes_Cpu.cpp
                Hes_Emu.cpp
        )
endif()

if (USE_GME_KSS)
    set(libgme_SRCS ${libgme_SRCS}
              # Ay_Apu.cpp included earlier
              # Sms_Apu.cpp included earlier
                Kss_Cpu.cpp
                Kss_Emu.cpp
                Kss_Scc_Apu.cpp
        )
endif()

if (USE_GME_NSF OR USE_GME_NSFE)
    set(libgme_SRCS ${libgme_SRCS}
                Nes_Apu.cpp
                Nes_Cpu.cpp
                Nes_Fme7_Apu.cpp
                Nes_Namco_Apu.cpp
                Nes_Oscs.cpp
                Nes_Vrc6_Apu.cpp
                Nsf_Emu.cpp
        )
endif()

if (USE_GME_NSFE)
    set(libgme_SRCS ${libgme_SRCS}
                Nsfe_Emu.cpp
        )
endif()

if (USE_GME_SAP)
    set(libgme_SRCS ${libgme_SRCS}
                Sap_Apu.cpp
                Sap_Cpu.cpp
                Sap_Emu.cpp
        )
endif()

if (USE_GME_SPC)
    set(libgme_SRCS ${libgme_SRCS}
                Snes_Spc.cpp
                Spc_Cpu.cpp
                Spc_Dsp.cpp
                Spc_Emu.cpp
                Spc_Filter.cpp
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
                Vgm_Emu_Impl.cpp
                Ym2413_Emu.cpp
        )
endif()

# These headers are part of the generic gme interface.
set (EXPORTED_HEADERS gme.h blargg_source.h)

# while building a macOS framework, exported headers must be in the source
# list, or the header files aren't copied to the bundle.
if (BUILD_FRAMEWORK)
    set(libgme_SRCS ${libgme_SRCS} ${EXPORTED_HEADERS})
endif()

# On some platforms we may need to change headers or whatnot based on whether
# we're building the library or merely using the library. The following is
# only defined when building the library to allow us to tell which is which.
add_definitions(-DBLARGG_BUILD_DLL)

# For the gme_types.h
include_directories(${CMAKE_CURRENT_BINARY_DIR})

# Add library to be compiled.
add_library(gme ${libgme_SRCS})

if(ZLIB_FOUND)
    message(" ** ZLib library located, compressed file formats will be supported")
    target_compile_definitions(gme PRIVATE -DHAVE_ZLIB_H)
    target_include_directories(gme PRIVATE ${ZLIB_INCLUDE_DIRS})
    target_link_libraries(gme ${ZLIB_LIBRARIES})
    # Is not to be installed though

    set(PKG_CONFIG_ZLIB -lz) # evaluated in libgme.pc.in
else()
    message("ZLib library not found, disabling support for compressed formats such as VGZ")
endif()

if(USE_GME_SPC)
    if(UNRAR_FOUND)
        message(" ** unRAR library located, the RSN file format will be supported")
        target_compile_definitions(gme PRIVATE -DRARDLL)
        target_include_directories(gme PRIVATE ${UNRAR_INCLUDE_DIRS})
        target_link_libraries(gme ${UNRAR_LIBRARIES})
        # Is not to be installed though

        set(PKG_CONFIG_UNRAR -lunrar) # evaluated in libgme.pc.in
    else()
        message("unRAR library not found, disabling support for the RSN file format")
    endif()
endif()
# The version is the release.  The "soversion" is the API version.  As long
# as only build fixes are performed (i.e. no backwards-incompatible changes
# to the API), the SOVERSION should be the same even when bumping up VERSION.
# The way gme.h is designed, SOVERSION should very rarely be bumped, if ever.
# Hopefully the API can stay compatible with old versions.
set_target_properties(gme
    PROPERTIES VERSION ${GME_VERSION}
               SOVERSION 1)

# macOS framework build
if(BUILD_FRAMEWORK)
    set_target_properties(gme
        PROPERTIES FRAMEWORK TRUE
                   FRAMEWORK_VERSION A
                   MACOSX_FRAMEWORK_IDENTIFIER net.mpyne.gme
                   VERSION ${GME_VERSION}
                   SOVERSION 0
                   PUBLIC_HEADER "${EXPORTED_HEADERS}")
endif()

install(TARGETS gme LIBRARY DESTINATION lib${LIB_SUFFIX}
                    RUNTIME DESTINATION bin # DLL platforms
                    ARCHIVE DESTINATION lib # DLL platforms
                    FRAMEWORK DESTINATION /Library/Frameworks) # macOS framework


# Run during cmake phase, so this is available during make
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/gme_types.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/gme_types.h)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/libgme.pc.in
    ${CMAKE_CURRENT_BINARY_DIR}/libgme.pc @ONLY)

install(FILES ${EXPORTED_HEADERS} DESTINATION include/gme)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/libgme.pc DESTINATION lib${LIB_SUFFIX}/pkgconfig)
