//
//  MIDIVisualizationController.h
//  CogAudio
//
//  Created by Christopher Snowhill on 9/27/25.
//

#ifndef MIDIVisualizationController_h
#define MIDIVisualizationController_h

NS_ASSUME_NONNULL_BEGIN

@interface MIDIEvent : NSObject
@property(nonatomic) NSURL *url;
@property(nonatomic) uint64_t timestamp; // milliseconds
@property(nonatomic) uint32_t which; // which port
@end

@interface SCVisEvent : MIDIEvent
@property(nonatomic) NSData *state;

- (id)initWithUrl:(NSURL *)url whichPort:(uint32_t)which state:(const void *)state stateSize:(size_t)size timestamp:(uint64_t)timestamp;

@end

@interface MIDIVisualizationController : NSObject {
}

- (id)initWithController:(id _Nonnull)c;

- (NSURL *)currentTrack;

- (uint64_t)currentTimestamp;

- (void)postEvent:(MIDIEvent *)event;

- (void)flushEvents;

- (NSArray<MIDIEvent *> *)eventsForTimestamp:(uint64_t)timestamp;

@end

NS_ASSUME_NONNULL_END

#endif /* MIDIVisualizationController_h */
