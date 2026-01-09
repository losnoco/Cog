//
//  fileprovider.h
//  AdPlug
//
//  Created by Christopher Snowhill on 1/27/18.
//  Copyright © 2018-2026 Christopher Snowhill. All rights reserved.
//

#ifndef fileprovider_h
#define fileprovider_h

#import <Cocoa/Cocoa.h>

#import "Plugin.h"

#include <binio.h>
#include <libAdPlug/fprovide.h>
#include <string>

class CProvider_cog : public CFileProvider {
	id<CogSource> m_file_hint;
	std::string m_file_path;

	public:
	virtual binistream *open(std::string filename) const;
	virtual void close(binistream *f) const;

	CProvider_cog() {
	}

	CProvider_cog(std::string filename, id<CogSource> file)
	: m_file_path(filename), m_file_hint(file) {
	}
};

#endif /* fileprovider_h */
