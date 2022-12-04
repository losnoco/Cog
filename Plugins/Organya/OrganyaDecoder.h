//
//  OrganyaDecoder.h
//  Organya
//
//  Created by Christopher Snowhill on 12/4/22.
//

#ifndef OrganyaDecoder_h
#define OrganyaDecoder_h

#import "Plugin.h"

namespace Organya {
struct Song;
}

@interface OrganyaDecoder : NSObject <CogDecoder> {
	Organya::Song *m_song;
	id<CogSource> source;
	long length, lengthWithFade;
	long samplesDiscard;
	
	double sampleRate;
	
	long renderedTotal;
	long loopedTotal;
	long loopsRemain;
	long fadeTotal;
	long fadeRemain;
}

- (void)setSource:(id<CogSource>)s;
- (id<CogSource>)source;
- (void)cleanUp;
@end

#endif /* OrganyaDecoder_h */
