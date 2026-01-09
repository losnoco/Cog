//
//  fileprovider.cpp
//  AdPlug
//
//  Created by Christopher Snowhill on 1/27/18.
//  Copyright © 2018-2026 Christopher Snowhill. All rights reserved.
//

#include "fileprovider.h"

class binistream_cog : public binistream {
	id<CogSource> m_file;

	Byte m_buffer[4096];
	int m_buffer_filled, m_buffer_position;

	public:
	binistream_cog(id<CogSource> p_file)
	: m_file(p_file), m_buffer_filled(0), m_buffer_position(0) {
	}

	void seek(long pos, Offset offs) {
		switch(offs) {
			case Set:
				break;

			case Add:
				if((pos < 0 && m_buffer_position + pos >= 0) ||
				   (pos >= 0 && m_buffer_position + pos < m_buffer_filled + m_buffer_position)) {
					m_buffer_filled -= pos;
					m_buffer_position += pos;
					err &= ~Eof;
					return;
				} else
					pos += [m_file tell] - m_buffer_filled;
				break;

			case End:
				[m_file seek:0 whence:SEEK_END];
				pos += [m_file tell];
				break;
		}
		[m_file seek:0 whence:SEEK_END];
		if(pos < [m_file tell]) {
			err &= ~Eof;
			[m_file seek:pos whence:SEEK_SET];
		}

		m_buffer_filled = 0;
		m_buffer_position = 0;
	}

	long pos() {
		return [m_file tell] - m_buffer_filled;
	}

	Byte getByte() {
		if(err & Eof) return -1;

		if(!m_buffer_filled) {
			m_buffer_filled = (int)[m_file read:m_buffer amount:4096];
			if(!m_buffer_filled) {
				err |= Eof;
				return -1;
			}
			m_buffer_position = 0;
		}

		Byte value = m_buffer[m_buffer_position];
		++m_buffer_position;
		--m_buffer_filled;
		return value;
	}
};

binistream *CProvider_cog::open(std::string filename) const {
	id<CogSource> p_file;

	if(filename == m_file_path) {
		p_file = m_file_hint;
		[p_file seek:0 whence:SEEK_SET];
	}

	if(p_file == nil) {
		NSString *urlString = [NSString stringWithUTF8String:filename.c_str()];
		NSString *fragmentString = @"";
		NSRange fragmentRange = [urlString rangeOfString:@"#" options:NSBackwardsSearch];
		if(fragmentRange.location != NSNotFound) {
			fragmentString = [urlString substringFromIndex:fragmentRange.location];
			urlString = [urlString substringToIndex:fragmentRange.location];
		}
		NSURL *url = [NSURL URLWithString:[[urlString stringByAddingPercentEncodingWithAllowedCharacters:NSCharacterSet.URLFragmentAllowedCharacterSet] stringByAppendingString:fragmentString]];
		id audioSourceClass = NSClassFromString(@"AudioSource");
		p_file = [audioSourceClass audioSourceForURL:url];

		if(![p_file open:url])
			return 0;
	}

	binistream_cog *f = new binistream_cog(p_file);
	if(f->error()) {
		delete f;
		return 0;
	}

	f->setFlag(binio::BigEndian, false);
	f->setFlag(binio::FloatIEEE);

	return f;
}

void CProvider_cog::close(binistream *f) const {
	binistream_cog *ff = (binistream_cog *)f;
	if(f)
		delete ff;
}
