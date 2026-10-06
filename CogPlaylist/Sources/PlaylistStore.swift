//
//  PlaylistStore.swift
//  CogPlaylist
//
//  The playlist's Core Data stack: Cog's own DataModel (shared with the macOS
//  app), loaded from this framework.
//

import CoreData
import Foundation

public final class PlaylistStore {
	public let container: NSPersistentContainer

	public var viewContext: NSManagedObjectContext { container.viewContext }

	/// The model, loaded once: Core Data wants one instance per model, or its
	/// entities stop matching their classes.
	private static let model: NSManagedObjectModel = {
		let bundle = Bundle(for: PlaylistStore.self)
		guard let url = bundle.url(forResource: "DataModel", withExtension: "momd"),
		      let model = NSManagedObjectModel(contentsOf: url) else {
			fatalError("CogPlaylist's DataModel is missing")
		}
		return model
	}()

	/// A store at `url`, by default the app's Application Support
	/// directory, as macOS keeps it; or in memory, for tests.
	public init(url: URL? = nil, inMemory: Bool = false) throws {
		Self.registerTransformers()
		container = NSPersistentContainer(name: "DataModel", managedObjectModel: Self.model)
		let description = NSPersistentStoreDescription(url: inMemory ? URL(fileURLWithPath: "/dev/null") : (url ?? Self.defaultURL))
		description.shouldMigrateStoreAutomatically = true
		description.shouldInferMappingModelAutomatically = true
		container.persistentStoreDescriptions = [description]
		var loadError: Error?
		container.loadPersistentStores { _, error in loadError = error }
		if let loadError { throw loadError }
		container.viewContext.mergePolicy = NSMergeByPropertyObjectTrumpMergePolicy
	}

	public static var defaultURL: URL {
		let directory = FileManager.default.urls(for: .applicationSupportDirectory, in: .userDomainMask)[0]
		try? FileManager.default.createDirectory(at: directory, withIntermediateDirectories: true)
		return directory.appendingPathComponent("DataModel.sqlite")
	}

	/// The metadata blob's transformer, which the model names.
	private static func registerTransformers() {
		let name = NSValueTransformerName("MaybeSecureValueDataTransformer")
		if ValueTransformer(forName: name) == nil {
			ValueTransformer.setValueTransformer(MaybeSecureValueDataTransformer(), forName: name)
		}
	}

	/// Saves the view context if it has changes.
	public func save() {
		let context = container.viewContext
		guard context.hasChanges else { return }
		do {
			try context.save()
		} catch {
			NSLog("Could not save the playlist: \(error)")
		}
	}
}
