#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace ts {

/// The environment variable that pins the ROM path.
///
/// Every front end honours it, so the path is typed once per shell rather than once per command.
inline constexpr const char* rom_path_variable = "TS_SCCORE_DLL";

/// Where a front end looked for the ROM, and what it found.
struct RomLocation {
    /// The resolved path, empty if nothing was found.
    std::filesystem::path path;

    /// Human-readable account of where it came from, for the error message when it is empty.
    std::vector<std::string> searched;

    [[nodiscard]] bool found() const noexcept { return !path.empty(); }
};

/// Finds the ROM, preferring what the caller asked for over what the environment pins.
///
/// The order is: an explicit path, then `TS_SCCORE_DLL`, then `SCCore.dll` beside the working
/// directory. An explicit path always wins and is never second-guessed -- a pinned environment must
/// not silently redirect a command that names a file, or comparing two DLL builds becomes a matter
/// of remembering what the shell was set to.
///
/// A path that was given explicitly is returned whether or not it exists, so the caller reports
/// "cannot open that file" rather than "no ROM found anywhere", which is the more useful error.
[[nodiscard]] RomLocation locate_rom(const std::string& explicit_path);

/// The message to print when nothing was found.
[[nodiscard]] std::string rom_not_found_message(const RomLocation& location);

} // namespace ts
