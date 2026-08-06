#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

namespace ts::formats {

/// Converts a music file in a recognised non-SMF format into an in-memory Standard MIDI File.
///
/// The formats are the ones game music actually shipped in: RIFF-MIDI wrappers (`RMID`),
/// Microsoft DirectMusic segments (`MIDS`), DOOM's MUS, Miles Sound System XMI, the General MIDI
/// Format found in AdLib-era titles (`GMF`), both HMI Sound Operating System containers
/// (`HMIMIDIP`/`HMIMIDIR` and `HMI-MIDISONG`), Mobile XMF, and the Loudness Sound System tracker
/// (`.lds`, which has no magic — it is only recognised through the file name). Each is either
/// unwrapped (RMID, XMF) or converted event-for-event into an equivalent SMF byte stream, so one
/// reader — `smf::parse` — stays the only thing that understands events.
///
/// Ported from spessasynth_core_c's `src/midi/parsers/`, which itself ports midi_processing.
///
/// Returns nothing when the data is not a recognised foreign format — including when it is
/// already a Standard MIDI File, which needs no conversion. Throws `std::runtime_error` when the
/// data *is* recognised but malformed: a truncated MUS is a broken file, not an unknown one.
///
/// `name` is the file name the data came from, when there is one. Only LDS needs it.
[[nodiscard]] std::optional<std::vector<std::uint8_t>> to_smf(std::span<const std::uint8_t> data,
                                                              std::string_view name = {});

} // namespace ts::formats
