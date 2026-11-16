//
//  ProjectMView.h
//  Cog
//
//  Created by Kevin López Brante on 2026-11-15.
//

#import <Cocoa/Cocoa.h>

NS_ASSUME_NONNULL_BEGIN

/// projectM's presets, drawn with OpenGL ES through ANGLE on Metal, on a
/// display link thread, and fed the stereo audio that is being heard.
@interface ProjectMView : NSView

/// A view, or nil when ANGLE can't make a surface to draw on.
+ (ProjectMView *_Nullable)createWithFrame:(NSRect)frame;

/// Draws while YES; NO freezes the last frame and stops the work.
@property(nonatomic) BOOL isRunning;

/// Folders searched, recursively, for `.milk` presets; set before the first
/// draw or later to reload the playlist.
@property(nonatomic, copy) NSArray<NSURL *> *presetFolders;
/// Folders searched for the textures presets name.
@property(nonatomic, copy) NSArray<NSURL *> *textureFolders;

@property(nonatomic) BOOL shuffle;
/// Seconds each preset shows before the next.
@property(nonatomic) double presetDuration;
/// Keeps the current preset until unlocked.
@property(nonatomic) BOOL presetLocked;

/// Seconds one preset blends into the next.
@property(nonatomic) double crossfadeDuration;
/// Cut straight to the next preset on a strong beat.
@property(nonatomic) BOOL hardCutsEnabled;
/// How much louder than the average a beat must be to cut; lower cuts more.
@property(nonatomic) double hardCutThreshold;
/// The per-pixel mesh, in cells: more is finer and costs more.
@property(nonatomic) NSSize meshSize;
/// Draws at 1/renderScale of the view's resolution in each direction, and
/// scales that up: 1, 2 or 4.
@property(nonatomic) NSInteger renderScale;
/// Frames per second at most; 0 draws every display refresh.
@property(nonatomic) double frameRateLimit;
/// Top-level folders of the presets left out, by name.
@property(nonatomic, copy) NSArray<NSString *> *excludedThemes;
/// The preset to start with, by path, when the playlist has it.
@property(nonatomic, copy, nullable) NSString *startingPresetPath;

/// The preset showing, by path; on the main thread when it changes.
@property(nonatomic, copy, nullable) void (^presetDidChange)(NSString *path);

- (void)nextPreset;
- (void)previousPreset;

/// The playlist's presets in order, and the index of the one showing; on
/// the main thread.
- (void)fetchPresets:(void (^)(NSArray<NSString *> *paths, NSInteger current))completion;
- (void)selectPresetAtIndex:(NSUInteger)index;

/// Stops drawing and frees projectM and its GL resources.
- (void)shutdown;

@end

NS_ASSUME_NONNULL_END
