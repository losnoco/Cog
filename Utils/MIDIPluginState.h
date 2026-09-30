//
//  MIDIPluginState.h
//  Cog
//
//  Created by Christopher Snowhill on 9/30/26.
//

#import <Foundation/Foundation.h>

#ifdef __cplusplus
extern "C" {
#endif

NS_ASSUME_NONNULL_BEGIN

/*
 * Saved state (kAudioUnitProperty_ClassInfo) of the Audio Unit MIDI synths,
 * one binary property list per plugin in
 * ~/Library/Application Support/Cog/MIDI Plugin State.
 *
 * It used to live in the midiPluginSettings user default, but some synths'
 * state is large (the S-MU2000 NVRAM is some 24 MB) and every write to the
 * defaults domain re-serializes all of it, hanging the whole process. The
 * first call of either function moves any midiPluginSettings entries out
 * to files, and removes the default once every entry is safely written.
 *
 * Both the preferences bundle and the MIDI decoder compile this file, so
 * these are plain C functions (a class would be defined twice in one
 * process). Each copy caches what it loads, keyed to the file's
 * modification date, so a save from one is seen by the other.
 */

/// The saved state for `plugin` (its eight-character subtype and
/// manufacturer), or nil if there is none.
NSDictionary *_Nullable MIDIPluginStateLoad(NSString *plugin);

/// Saves `state` for `plugin`. Returns NO if it could not be written.
BOOL MIDIPluginStateSave(NSString *plugin, NSDictionary *state);

/// Moves any midiPluginSettings entries out of the defaults now, rather than
/// on first use. The app calls this at launch, off the main thread.
void MIDIPluginStateMigrate(void);

NS_ASSUME_NONNULL_END

#ifdef __cplusplus
}
#endif
