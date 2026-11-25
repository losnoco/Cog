//
//  NSData+MD5.swift
//  Cog
//
//  Created by Christopher Snowhill on 10/9/13.
//  Copyright 2013 Christopher Snowhill. All rights reserved.
//

import CryptoKit
import Foundation

extension NSData {
	// Lowercase hex MD5. Only a key for album art in saved playlists, not for
	// security, and kept as MD5 so playlists come out as they always have.
	@objc(MD5)
	func md5() -> String {
		return Insecure.MD5.hash(data: self as Data).map { String(format: "%02x", $0) }.joined()
	}
}