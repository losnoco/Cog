#pragma once

#include "tabulasonora/table_manifest.hpp"

#include <cstdint>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace ts {

/// How strictly `RomImage::open` checks that the file is the pinned build.
enum class RomVerification {
    /// Verify size, PE timestamp and the full SHA-256. The default, and the only safe choice.
    full,
    /// Verify size and PE timestamp but skip the SHA-256. Faster; use only in tight loops.
    quick,
    /// Perform no checks at all. For experimenting with a different build; offsets will be wrong.
    none,
};

/// Thrown when the supplied `SCCore.dll` is not the build the table offsets are pinned to.
class RomIdentityError : public std::runtime_error {
public:
    explicit RomIdentityError(const std::string& message) : std::runtime_error(message) {}
};

/// Read-only access to `SCCore.dll` *as a data file*.
///
/// The DLL is never loaded as code — no `LoadLibrary`, no `dlopen`, no native dependency. It is
/// opened as a plain file and sliced at the offsets recorded in `TableManifest`, which is what lets
/// this engine stay portable and run on hosts the original plugin never supported.
///
/// A file image reads positionally (`pread`) rather than through a memory map, so no 27 MB copy is
/// ever resident. A host without a filesystem supplies the bytes instead; see `from_memory`.
class RomImage {
public:
    /// Opens and verifies an `SCCore.dll`.
    ///
    /// Throws `RomIdentityError` if the file is not the pinned build, or `std::runtime_error` if it
    /// cannot be opened. The manifest defaults to the embedded one.
    [[nodiscard]] static RomImage open(const std::string& path,
                                       RomVerification verification = RomVerification::full,
                                       const TableManifest* manifest = nullptr);

    /// Wraps an `SCCore.dll` already held in memory, and verifies it.
    ///
    /// Nothing is copied — the memory is read in place — so the caller must keep it alive and
    /// unchanged for as long as the image is used, exactly as a file image needs its handle open.
    [[nodiscard]] static RomImage from_memory(std::span<const std::uint8_t> image,
                                              RomVerification verification = RomVerification::full,
                                              const TableManifest* manifest = nullptr,
                                              std::string name = "<memory>");

    RomImage(RomImage&&) noexcept;
    RomImage& operator=(RomImage&&) noexcept;
    RomImage(const RomImage&) = delete;
    RomImage& operator=(const RomImage&) = delete;
    ~RomImage();

    /// Path, or descriptive name, the image was opened from.
    [[nodiscard]] const std::string& path() const noexcept { return path_; }

    /// Length of the file in bytes.
    [[nodiscard]] std::int64_t length() const noexcept { return length_; }

    /// The offset map these reads are interpreted through.
    [[nodiscard]] const TableManifest& manifest() const noexcept { return *manifest_; }

    /// Reads into a caller-supplied buffer, filling it completely.
    ///
    /// Throws `std::runtime_error` if the file ends before the buffer is filled.
    void read(std::int64_t file_offset, std::span<std::uint8_t> destination) const;

    /// Reads `length` bytes starting at `file_offset`.
    [[nodiscard]] std::vector<std::uint8_t> read(std::int64_t file_offset,
                                                 std::size_t length) const;

    /// Reads the bytes of one cached table, exactly `entry.size` long.
    [[nodiscard]] std::vector<std::uint8_t> read(const TableEntry& entry) const;

    /// Reads the COFF `TimeDateStamp` from the PE header, or `0` if the file is not a PE image.
    [[nodiscard]] std::uint32_t read_pe_timestamp() const;

    /// Computes the SHA-256 of the whole file as lower-case hex.
    [[nodiscard]] std::string compute_sha256() const;

private:
    /// Where an image's bytes come from.
    ///
    /// The only thing the rest of the class needs of a source is a length and a positional read,
    /// which is exactly the shape both a file handle and a byte buffer offer.
    class Source {
    public:
        virtual ~Source() = default;
        [[nodiscard]] virtual std::int64_t length() const noexcept = 0;
        /// Reads at an absolute offset, returning how much was actually read; zero at the end.
        [[nodiscard]] virtual std::size_t read(std::span<std::uint8_t> destination,
                                               std::int64_t offset) const = 0;
    };

    class FileSource;
    class MemorySource;

    RomImage(std::string path, std::unique_ptr<Source> source, const TableManifest& manifest);

    void verify(RomVerification verification) const;

    std::string path_;
    std::unique_ptr<Source> source_;
    const TableManifest* manifest_;
    std::int64_t length_ = 0;
};

} // namespace ts
