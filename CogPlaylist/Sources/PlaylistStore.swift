//
//  PlaylistStore.swift
//  CogPlaylist
//
//  The playlist's Core Data stack: Cog's DataModel, compiled into this
//  framework, so that its entities' classes are this framework's. The macOS
//  app builds its own container on the same model.
//

import CoreData
import Foundation

@objc public final class PlaylistStore: NSObject {
	public let container: NSPersistentContainer

	public var viewContext: NSManagedObjectContext { container.viewContext }

	/// The model, loaded once: Core Data wants one instance per model, or its
	/// entities stop matching their classes.
	@objc public static let model: NSManagedObjectModel = {
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
		super.init()
		let artwork = ArtworkStore(context: container.viewContext)
		try artwork.loadAll()
		ArtworkStore.shared = artwork
	}

	public static var defaultURL: URL {
		let directory = FileManager.default.urls(for: .applicationSupportDirectory, in: .userDomainMask)[0]
		try? FileManager.default.createDirectory(at: directory, withIntermediateDirectories: true)
		return directory.appendingPathComponent("DataModel.sqlite")
	}

	/// The metadata blob's transformer, which the model names: registered
	/// before any store opens on the model.
	@objc public static func registerTransformers() {
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
