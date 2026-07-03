//
//  RemoteControlSocketApp.swift
//  CogRemoteControl
//
//  Created by Kevin López Brante on 2026-07-01.
//
//  NIO Unix-domain-socket host for the MCP stdio-style transport: each
//  accepted connection carries newline-delimited JSON-RPC (the same framing
//  as stdio), so the bundled cog-mcp helper can pipe an MCP client straight
//  through. The socket file lives in the app-group container, so only this
//  user's processes can reach it.
//

import Foundation
import Logging
import MCP
@preconcurrency import NIOCore
@preconcurrency import NIOPosix

/// Hosts MCP `Server` sessions over a Unix domain socket. Each connection
/// gets its own `Server` + `LineFramedTransport` pair; the connection's
/// lifetime is the session's lifetime.
@available(macOS 13.0, *)
actor RemoteControlSocketApp {
	typealias ServerFactory = @Sendable () async -> Server

	private let socketPath: String
	private let serverFactory: ServerFactory

	private var group: MultiThreadedEventLoopGroup?
	private var channel: Channel?

	private var sessions: [ObjectIdentifier: (server: Server, transport: LineFramedTransport)] = [:]

	init(socketPath: String, serverFactory: @escaping ServerFactory) {
		self.socketPath = socketPath
		self.serverFactory = serverFactory
	}

	// MARK: - Lifecycle

	/// Binds the listening socket and starts accepting connections.
	/// Throws (e.g. unwritable directory) without leaving anything running.
	func start() async throws {
		guard channel == nil else { return }

		try FileManager.default.createDirectory(
			atPath: (socketPath as NSString).deletingLastPathComponent,
			withIntermediateDirectories: true
		)

		let group = MultiThreadedEventLoopGroup(numberOfThreads: 1)

		let bootstrap = ServerBootstrap(group: group)
			.serverChannelOption(ChannelOptions.backlog, value: 16)
			.childChannelInitializer { channel in
				channel.pipeline.addHandler(LineFramedHandler(app: self))
			}

		do {
			channel = try await bootstrap.bind(
				unixDomainSocketPath: socketPath,
				cleanupExistingSocketFile: true
			).get()
		} catch {
			try? await group.shutdownGracefully()
			throw error
		}

		self.group = group
	}

	/// Stops accepting connections and closes all sessions.
	func stop() async {
		for key in Array(sessions.keys) {
			await endSession(key)
		}
		try? await channel?.close()
		channel = nil
		try? await group?.shutdownGracefully()
		group = nil
		// NIO unlinks stale sockets on bind, not on close; tidy up ourselves.
		try? FileManager.default.removeItem(atPath: socketPath)
	}

	// MARK: - Session Management

	/// Called once per accepted connection. The transport is already wired to
	/// the channel; reads may be buffered in its stream before the server starts.
	func startSession(transport: LineFramedTransport) async {
		let server = await serverFactory()
		do {
			try await server.start(transport: transport)
		} catch {
			await transport.disconnect()
			return
		}
		sessions[ObjectIdentifier(transport)] = (server, transport)

		// The connection may have died before the session was registered;
		// if so the close notification already came and went, so clean up here.
		if !transport.isChannelActive {
			await endSession(ObjectIdentifier(transport))
		}
	}

	func connectionClosed(transport: LineFramedTransport) async {
		await endSession(ObjectIdentifier(transport))
	}

	private func endSession(_ key: ObjectIdentifier) async {
		guard let session = sessions.removeValue(forKey: key) else { return }
		await session.server.stop()
		await session.transport.disconnect()
	}
}

// MARK: - Per-connection MCP Transport

/// MCP `Transport` bridging a NIO channel that carries newline-delimited
/// JSON-RPC messages — the same framing the stdio transport uses.
@available(macOS 13.0, *)
actor LineFramedTransport: Transport {
	nonisolated let logger: Logger

	private let channel: Channel
	private let stream: AsyncThrowingStream<Data, Swift.Error>
	private nonisolated let continuation: AsyncThrowingStream<Data, Swift.Error>.Continuation

	init(channel: Channel) {
		self.channel = channel
		self.logger = Logger(label: "org.cogx.cog.remote-control.transport")

		var continuation: AsyncThrowingStream<Data, Swift.Error>.Continuation!
		self.stream = AsyncThrowingStream { continuation = $0 }
		self.continuation = continuation
	}

	nonisolated var isChannelActive: Bool { channel.isActive }

	/// The channel is already connected when the transport is created.
	func connect() async throws {}

	func disconnect() async {
		continuation.finish()
		channel.close(mode: .all, promise: nil)
	}

	func send(_ data: Data) async throws {
		var framed = data
		framed.append(0x0A)
		var buffer = channel.allocator.buffer(capacity: framed.count)
		buffer.writeBytes(framed)
		try await channel.writeAndFlush(buffer).get()
	}

	func receive() -> AsyncThrowingStream<Data, Swift.Error> { stream }

	// Called from the channel handler (event loop thread).
	nonisolated func deliver(line: Data) { continuation.yield(line) }
	nonisolated func finishStream() { continuation.finish() }
}

// MARK: - NIO Handler

/// Splits the inbound byte stream on newlines and feeds complete messages to
/// the connection's `LineFramedTransport`; tells the app when the connection
/// opens and closes.
@available(macOS 13.0, *)
private final class LineFramedHandler: ChannelInboundHandler, @unchecked Sendable {
	typealias InboundIn = ByteBuffer
	typealias OutboundOut = ByteBuffer

	/// A JSON-RPC message larger than this is not remote-control traffic;
	/// drop the connection rather than buffer without bound.
	private static let maxLineBytes = 4 << 20

	private let app: RemoteControlSocketApp
	private var transport: LineFramedTransport?
	private var pending = Data()

	init(app: RemoteControlSocketApp) {
		self.app = app
	}

	func channelActive(context: ChannelHandlerContext) {
		let transport = LineFramedTransport(channel: context.channel)
		self.transport = transport
		Task { await app.startSession(transport: transport) }
	}

	func channelRead(context: ChannelHandlerContext, data: NIOAny) {
		var buffer = unwrapInboundIn(data)
		if let bytes = buffer.readBytes(length: buffer.readableBytes) {
			pending.append(contentsOf: bytes)
		}

		while let newlineIndex = pending.firstIndex(of: 0x0A) {
			var line = pending.prefix(upTo: newlineIndex)
			if line.last == 0x0D {
				line = line.dropLast()
			}
			if !line.isEmpty {
				transport?.deliver(line: Data(line))
			}
			pending = Data(pending.suffix(from: pending.index(after: newlineIndex)))
		}

		if pending.count > Self.maxLineBytes {
			context.close(promise: nil)
		}
	}

	func channelInactive(context: ChannelHandlerContext) {
		notifyClosed()
		context.fireChannelInactive()
	}

	func errorCaught(context: ChannelHandlerContext, error: Error) {
		notifyClosed()
		context.close(promise: nil)
	}

	private func notifyClosed() {
		guard let transport else { return }
		self.transport = nil
		transport.finishStream()
		Task { await app.connectionClosed(transport: transport) }
	}
}
