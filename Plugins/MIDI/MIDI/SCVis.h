//
//  SCVis.h
//  MIDI
//
//  Created by Christopher Snowhill on 9/25/25.
//

#ifndef SCVis_h
#define SCVis_h

@interface SCVisUpdate : NSObject

@property(nonatomic) NSURL *file;
@property(nonatomic) uint64_t timestamp; // milliseconds
@property(nonatomic) uint32_t which; // which display
@property(nonatomic) NSData *state;

+ (void)post:(SCVisUpdate *)update;

- (id)initWithFile:(NSURL *)file whichScreen:(uint32_t)which state:(const void *)state stateSize:(size_t)size timestamp:(uint64_t)timestamp;

@end

#endif /* SCVis_h */
