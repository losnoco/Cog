//
//  MiniPlayerPlusWindowController.m
//  Cog
//

#import "MiniPlayerPlusWindowController.h"
#import "AppController.h"
#import "PlaylistEntry.h"
#import "PlaybackController.h"

// Padding constants
static const CGFloat kSidePad = 16.0;
static const CGFloat kTopPad = 12.0;
static const CGFloat kItemSpacing = 2.0;
static const CGFloat kSectionGap = 10.0;

@implementation MiniPlayerPlusWindowController {
    NSScrollView *_scrollView;
    NSView *_stackContainer;
}

static void *kMiniPlayerPlusContext = &kMiniPlayerPlusContext;

@synthesize valueToDisplay;

- (void)awakeFromNib {
    // Set up the scroll view that fills the content area
    NSView *contentView = [[self window] contentView];
    _scrollView = [[NSScrollView alloc] initWithFrame:contentView.bounds];
    _scrollView.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
    _scrollView.hasVerticalScroller = YES;
    _scrollView.hasHorizontalScroller = NO;
    _scrollView.autohidesScrollers = YES;
    _scrollView.drawsBackground = NO;
    [contentView addSubview:_scrollView];

    [[self window] setDelegate:self];

    [playlistSelectionController addObserver:self
                                  forKeyPath:@"selection"
                                     options:NSKeyValueObservingOptionNew
                                     context:kMiniPlayerPlusContext];
    [currentEntryController addObserver:self
                             forKeyPath:@"content"
                                options:NSKeyValueObservingOptionNew
                                context:kMiniPlayerPlusContext];
	[[NSNotificationCenter defaultCenter] addObserver:self
											 selector:@selector(playbackDidBegin:)
												 name:CogPlaybackDidBeginNotificiation
											   object:nil];
    [self rebuildContent];
}

- (void)dealloc {
	[playlistSelectionController removeObserver:self
									 forKeyPath:@"selection"
										context:kMiniPlayerPlusContext];
	[currentEntryController removeObserver:self
								forKeyPath:@"content"
								   context:kMiniPlayerPlusContext];
	[[NSNotificationCenter defaultCenter] removeObserver:self
													name:CogPlaybackDidBeginNotificiation
												  object:nil];
}

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary *)change context:(void *)context {
    if(context == kMiniPlayerPlusContext) {
        PlaylistEntry *currentSelection = [[playlistSelectionController selectedObjects] firstObject];
        if(currentSelection != nil) {
            [self setValueToDisplay:currentSelection];
        } else {
            [self setValueToDisplay:[currentEntryController content]];
        }
    } else {
        [super observeValueForKeyPath:keyPath ofObject:object change:change context:context];
    }
}

- (void)playbackDidBegin:(NSNotification *)notification {
	PlaylistEntry *currentSelection = [[playlistSelectionController selectedObjects] firstObject];
	if(currentSelection != nil) {
		[self setValueToDisplay:currentSelection];
	} else {
		[self setValueToDisplay:[currentEntryController content]];
	}
	[self rebuildContent];
}

- (void)setValueToDisplay:(id)newValue {
    valueToDisplay = newValue;
    [self rebuildContent];
}

- (void)windowDidEndLiveResize:(NSNotification *)notification {
    [self rebuildContent];
}

- (void)rebuildContent {
    if(!_scrollView) return;

    PlaylistEntry *entry = valueToDisplay;
    CGFloat width = NSWidth(_scrollView.bounds);
    if(width < 1) width = 360;

    // Build a flipped container view so y=0 is at the top
    NSView *flipped = [[_FlippedView alloc] initWithFrame:NSMakeRect(0, 0, width, 2000)];

    CGFloat curY = kTopPad;

    // --- Album Art ---
    NSImage *art = [entry albumArt];
    if(art) {
        CGFloat artSize = width - 2 * kSidePad;
        NSImageView *artView = [[NSImageView alloc] initWithFrame:NSMakeRect(kSidePad, curY, artSize, artSize)];
        artView.image = art;
        artView.imageScaling = NSImageScaleProportionallyUpOrDown;
        artView.imageAlignment = NSImageAlignCenter;
        [flipped addSubview:artView];
        curY += artSize + kSectionGap;
    }

    // --- Song Title ---
    NSString *title = [entry title];
    if(title.length == 0) title = [entry display];
    if(title.length > 0) {
        NSTextField *titleField = [self labelFieldWithString:title font:[NSFont boldSystemFontOfSize:16] width:width - 2*kSidePad atY:curY];
        [flipped addSubview:titleField];
        curY += NSHeight(titleField.frame) + 3;
    }

    // --- Artist + Album (medium, secondary) ---
    NSString *artist = [entry artist] ?: @"";
    NSString *albumArtist = [entry albumartist] ?: @"";
    NSString *album = [entry album] ?: @"";

    NSString *displayArtist = artist.length ? artist : (albumArtist.length ? albumArtist : nil);
    NSMutableString *subLine = [NSMutableString string];
    if(displayArtist.length) [subLine appendString:displayArtist];
    if(album.length) {
        if(subLine.length) [subLine appendString:@" — "];
        [subLine appendString:album];
    }
    if(subLine.length > 0) {
        NSTextField *subField = [self labelFieldWithString:subLine font:[NSFont systemFontOfSize:13] width:width - 2*kSidePad atY:curY];
        subField.textColor = [NSColor secondaryLabelColor];
        [flipped addSubview:subField];
        curY += NSHeight(subField.frame) + kSectionGap;
    } else {
        curY += kSectionGap;
    }

    // --- Separator line ---
    NSBox *sep = [[NSBox alloc] initWithFrame:NSMakeRect(kSidePad, curY, width - 2*kSidePad, 1)];
    sep.boxType = NSBoxSeparator;
    [flipped addSubview:sep];
    curY += 6;

    // --- Technical info rows (only if non-empty / non-zero) ---
    // Track & Length on same line
    NSString *trackText = [entry trackText];
    NSString *lengthText = [entry lengthText];
    if(trackText.length > 0 && ![trackText isEqualToString:@"0"]) {
        curY = [self addRow:NSLocalizedString(@"Track", @"Track title") value:trackText toView:flipped atY:curY width:width];
    }
    if(lengthText.length > 0) {
        curY = [self addRow:NSLocalizedString(@"Length", @"Length title") value:lengthText toView:flipped atY:curY width:width];
    }

    NSString *genre = [entry genre];
    if(genre.length) curY = [self addRow:NSLocalizedString(@"Genre", @"Genre title") value:genre toView:flipped atY:curY width:width];

    NSString *composer = [entry composer];
    if(composer.length) curY = [self addRow:NSLocalizedString(@"Composer", @"Composer title") value:composer toView:flipped atY:curY width:width];

    NSString *date = [entry date];
    if(date.length) curY = [self addRow:NSLocalizedString(@"Date", @"Date title") value:date toView:flipped atY:curY width:width];

    NSString *comment = [entry comment];
    if(comment.length) curY = [self addRow:NSLocalizedString(@"Comment", @"Comment title") value:comment toView:flipped atY:curY width:width];

    // Technical separator
    NSBox *sep2 = [[NSBox alloc] initWithFrame:NSMakeRect(kSidePad, curY, width - 2*kSidePad, 1)];
    sep2.boxType = NSBoxSeparator;
    [flipped addSubview:sep2];
    curY += 6;

    float sr = [entry sampleRate];
    if(sr > 0) {
        NSValueTransformer *t = [NSValueTransformer valueTransformerForName:@"NumberHertzToStringTransformer"];
        NSString *srStr = t ? [t transformedValue:@(sr)] : [NSString stringWithFormat:@"%.0f Hz", sr];
        curY = [self addRow:NSLocalizedString(@"Sample Rate", @"Sample Rate title") value:srStr toView:flipped atY:curY width:width];
    }
    int ch = [entry channels];
    if(ch > 0) curY = [self addRow:NSLocalizedString(@"Channels", @"Channels title") value:@(ch).stringValue toView:flipped atY:curY width:width];
    int bps = [entry bitsPerSample];
    if(bps > 0) curY = [self addRow:NSLocalizedString(@"Bits/Sample", @"Bits per sample title") value:@(bps).stringValue toView:flipped atY:curY width:width];
    int br = [entry bitrate];
    if(br > 0) curY = [self addRow:NSLocalizedString(@"Bitrate", @"Bitrate title") value:[NSString stringWithFormat:@"%d kbps", br] toView:flipped atY:curY width:width];
    NSString *codec = [entry codec];
    if(codec.length) curY = [self addRow:NSLocalizedString(@"Codec", @"Codec title") value:codec toView:flipped atY:curY width:width];
    NSString *encoding = [entry encoding];
    if(encoding.length) curY = [self addRow:NSLocalizedString(@"Encoding", @"Encoding title") value:encoding toView:flipped atY:curY width:width];

    // Only show ReplayGain if there is actual gain data applied
    if(entry.replayGainAlbumGain != 0 || entry.replayGainTrackGain != 0 ||
       (entry.soundcheck && entry.soundcheck.length) || (entry.volume != 0 && entry.volume != 1.0)) {
        NSString *replayGain = [entry gainCorrection];
        if(replayGain.length) {
            curY = [self addRow:NSLocalizedString(@"ReplayGain", @"ReplayGain title") value:replayGain toView:flipped atY:curY width:width];
        }
    }
    NSString *playCount = [entry playCount];
    if(playCount.length && ![playCount isEqualToString:@"0"]) {
        curY = [self addRow:NSLocalizedString(@"Play Count", @"Play Count title") value:playCount toView:flipped atY:curY width:width];
    }
    NSString *filename = [entry filename];
    if(filename.length) curY = [self addRow:NSLocalizedString(@"File", @"File title") value:filename toView:flipped atY:curY width:width];

    curY += kTopPad;

    // Resize the flipped container to fit content, then set as scroll view document
    flipped.frame = NSMakeRect(0, 0, width, curY);
    _scrollView.documentView = flipped;
}

/// Make a wrapping text label that auto-sizes its height, positioned left-aligned at y
- (NSTextField *)labelFieldWithString:(NSString *)str font:(NSFont *)font width:(CGFloat)w atY:(CGFloat)y {
    NSTextField *f = [[NSTextField alloc] initWithFrame:NSMakeRect(kSidePad, y, w, 10000)];
    f.stringValue = str;
    f.font = font;
    f.bezeled = NO;
    f.drawsBackground = NO;
    f.editable = NO;
    f.selectable = YES;
    f.lineBreakMode = NSLineBreakByWordWrapping;
    f.preferredMaxLayoutWidth = w;
    [f sizeToFit];
    // sizeToFit may not wrap; use cell sizing
    NSSize needed = [f.cell cellSizeForBounds:NSMakeRect(0, 0, w, 10000)];
    f.frame = NSMakeRect(kSidePad, y, w, needed.height);
    return f;
}

/// Add a label:value row, returns new Y position
- (CGFloat)addRow:(NSString *)label value:(NSString *)value toView:(NSView *)parent atY:(CGFloat)y width:(CGFloat)width {
    CGFloat labelW = 110;
    CGFloat valueX = kSidePad + labelW + 4;
    CGFloat valueW = width - valueX - kSidePad;

    NSTextField *lbl = [[NSTextField alloc] initWithFrame:NSMakeRect(kSidePad, y, labelW, 17)];
    lbl.stringValue = [label stringByAppendingString:@":"];
    lbl.font = [NSFont systemFontOfSize:NSFont.smallSystemFontSize];
    lbl.alignment = NSTextAlignmentRight;
    lbl.textColor = [NSColor secondaryLabelColor];
    lbl.bezeled = NO;
    lbl.drawsBackground = NO;
    lbl.editable = NO;

    NSTextField *val = [[NSTextField alloc] initWithFrame:NSMakeRect(valueX, y, valueW, 10000)];
    val.stringValue = value;
    val.font = [NSFont systemFontOfSize:NSFont.smallSystemFontSize];
    val.bezeled = NO;
    val.drawsBackground = NO;
    val.editable = NO;
    val.selectable = YES;
    val.lineBreakMode = NSLineBreakByWordWrapping;
    NSSize needed = [val.cell cellSizeForBounds:NSMakeRect(0, 0, valueW, 10000)];
    CGFloat rowH = MAX(needed.height, 17);
    val.frame = NSMakeRect(valueX, y, valueW, rowH);
    lbl.frame = NSMakeRect(kSidePad, y + (rowH - 17) / 2.0, labelW, 17);

    [parent addSubview:lbl];
    [parent addSubview:val];
    return y + rowH + kItemSpacing;
}

@end

// Private flipped view so y=0 is at the top
@implementation _FlippedView
- (BOOL)isFlipped { return YES; }
@end
