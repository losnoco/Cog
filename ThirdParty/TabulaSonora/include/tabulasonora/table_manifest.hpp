#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace ts {

/// Identity of the `SCCore.dll` build every offset in the manifest is pinned to.
///
/// `version` is provenance, not evidence, and nothing verifies it: the DLL carries no version
/// resource at all, which is why identity rests on the hash, the PE timestamp and the size. It is
/// recorded because "the build that ships with SOUND Canvas VA 1.1.6" is how a person finds the
/// right file, whereas a SHA-256 is how the code recognises it once they have.
struct DllIdentity {
    /// Expected file name, for diagnostics only.
    std::string file_name;
    /// Human-readable product description.
    std::string product;
    /// The SOUND Canvas VA release this build ships in — `1.1.6`.
    std::string version;
    /// Exact file size in bytes.
    std::int64_t size = 0;
    /// Lower-case hex SHA-256 of the whole file.
    std::string sha256;
    /// Lower-case hex SHA-1 of the whole file.
    std::string sha1;
    /// Lower-case hex MD5 of the whole file.
    std::string md5;
    /// COFF `TimeDateStamp` from the PE header.
    std::uint32_t pe_timestamp = 0;
};

/// One static table sliced byte-for-byte out of the DLL.
struct TableEntry {
    /// Cache file name, e.g. `curve_amp_hi_2ba0.bin`.
    std::string name;
    /// Project label for the symbol, e.g. `g_amp_curve_hi`. Not Roland's name.
    std::string symbol;
    /// Owning subsystem: TVA, TVF, PITCH, ENV, LFO, DIR, SAMPLER, MIX or CTRL.
    std::string subsystem;
    /// Element type as recorded by the manifest generator.
    std::string dtype;
    /// Human-readable shape, e.g. `128x4`.
    ///
    /// Not load-bearing: element counts are derived from `size`, because several shapes record a
    /// byte count where an element count would be expected.
    std::string shape;
    /// Length in bytes.
    std::int32_t size = 0;
    /// Offset of the first byte within the DLL file.
    std::int64_t file_offset = 0;
    /// Virtual address in the loaded image, when known.
    std::optional<std::int64_t> va;
    /// Section skew used to derive `file_offset` from `va`.
    std::optional<std::int64_t> section_adjust;
    /// `full` when the cached bytes match the DLL exactly, or `prefix n/m` for an over-read cache.
    std::string match = "full";
    /// What the table is for.
    std::string purpose;
};

/// A region read straight from the DLL at render time rather than cached to a `.bin` file — the two
/// wave-ROM banks and the drum-kit lookup tables.
///
/// The drum regions are recorded under a `va` key, but those values are already DLL *file offsets*
/// — they carry no image base. They are surfaced here as `file_offset` so every consumer addresses
/// the file uniformly.
struct LiveRegion {
    /// Region name, e.g. `wave_rom_bank_A`.
    std::string name;
    /// Project label for the symbol.
    std::string symbol;
    /// Owning subsystem, `ROM` or `DIR`.
    std::string subsystem;
    /// Element description.
    std::string dtype;
    /// Offset of the first byte within the DLL file.
    std::int64_t file_offset = 0;
    /// Length in bytes, when the manifest records one.
    std::optional<std::int64_t> size;
    /// What the region is for.
    std::string purpose;
};

/// The machine-readable map of everything the engine needs out of `SCCore.dll`: the pinned build
/// identity, every static table's byte-exact offset, and the live ROM/directory regions.
///
/// A copy of `manifest.json` from the Tabula Sonora spec repo is compiled into the library, so the
/// engine is self-describing with no data files on disk.
class TableManifest {
public:
    /// The manifest embedded in this library. Parsed once, on first use.
    [[nodiscard]] static const TableManifest& defaults();

    /// Parses a manifest from a JSON document.
    ///
    /// Throws `std::runtime_error` if the document is not a well-formed manifest.
    [[nodiscard]] static TableManifest parse(std::string_view json);

    /// The DLL build these offsets are valid for.
    [[nodiscard]] const DllIdentity& dll() const noexcept { return dll_; }

    /// The DLL's preferred image base, `0x180000000`.
    [[nodiscard]] std::int64_t image_base() const noexcept { return image_base_; }

    /// The static tables cached as `.bin` slices.
    [[nodiscard]] const std::vector<TableEntry>& cached_tables() const noexcept { return tables_; }

    /// The regions read directly from the DLL rather than cached.
    [[nodiscard]] const std::vector<LiveRegion>& live_regions() const noexcept { return regions_; }

    /// Looks up a cached table by its `.bin` file name, e.g. `pan_a2fa1.bin`.
    ///
    /// Throws `std::out_of_range` if no table with that name exists.
    [[nodiscard]] const TableEntry& table(std::string_view name) const;

    /// Looks up a live region by name, e.g. `wave_rom_bank_A`.
    ///
    /// Throws `std::out_of_range` if no region with that name exists.
    [[nodiscard]] const LiveRegion& region(std::string_view name) const;

private:
    TableManifest() = default;

    void index();

    /// Transparent hash so a `std::string_view` can look up an owned `std::string` key without
    /// allocating. The keys are owned rather than views into the entries: a name like
    /// `pan_a2fa1.bin` is short enough for the small-string optimisation, so a view into one would
    /// dangle the moment the manifest is moved.
    struct StringHash {
        using is_transparent = void;

        [[nodiscard]] std::size_t operator()(std::string_view text) const noexcept
        {
            return std::hash<std::string_view>{}(text);
        }
    };

    using Index = std::unordered_map<std::string, std::size_t, StringHash, std::equal_to<>>;

    DllIdentity dll_;
    std::int64_t image_base_ = 0;
    std::vector<TableEntry> tables_;
    std::vector<LiveRegion> regions_;
    Index by_name_;
    Index regions_by_name_;
};

} // namespace ts
