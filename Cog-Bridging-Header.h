//
//  Use this file to import your target's public headers that you would like to expose to Swift.
//

#import <CogAudio/Status.h>
#import <MASShortcut/Shortcut.h>
// Imported textually (quoted project paths, not framework module imports):
// pulling in the CogRemoteControl clang module would also load its Swift
// module, whose macOS 13 floor the 10.15 app can't satisfy.
#import "CogRemoteControl/CogRemoteControlTarget.h"
#import "RemoteControl/CogRemoteControlServerBridgeInterface.h"
#import "Application/AppController.h"
#import "Application/PlaybackController.h"
#import "Playlist/PlaylistController.h"
#import "Playlist/PlaylistLoader.h"
#import "Playlist/PlaylistControllerEnums.h"
#import "Preferences/PathSuggester.h"
#import "Preferences/MIDIConfig.h"
#import "Spotlight/SpotlightWindowController.h"
#import "NSData+MD5.h"
