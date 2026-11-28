//
//  ContainerNode.swift
//  Cog
//
//  Created by Vincent Spader on 10/15/07.
//  Copyright 2007 Vincent Spader. All rights reserved.
//

import CogAudio
import Foundation

@objc(ContainerNode)
class ContainerNode: PathNode {
	override func isLeaf() -> Bool {
		false
	}

	override func updatePath() {
		guard let url = URL else { return }
		let urls = (AudioContainer.urls(forContainerURL: url) as? [URL]) ?? []

		var paths: [ContainedNode] = []
		for u in urls {
			paths.append(ContainedNode(dataSource: dataSource, url: u))
		}

		setSubpaths(paths)
	}
}
