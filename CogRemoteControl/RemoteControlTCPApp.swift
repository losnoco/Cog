//
//  RemoteControlTCPApp.swift
//  CogRemoteControl
//
//  Created by Kevin López Brante on 2026-07-01.
//
//  NIO TCP host for the MCP stdio-style transport: each accepted loopback
//  connection carries newline-delimited JSON-RPC (the same framing as stdio),
//  so the bundled cog-mcp helper can pipe an MCP client straight through.
//
//  TCP rather than a Unix socket because the app is sandboxed: the socket
//  file would have to live in the app container, and outside processes
//  touching another app's container trip macOS 14+'s "app container
//  protection" TCC prompt (attributed to whichever app spawned cog-mcp).
//

import Foundation
import Logging
import MCP
@preconcurrency import NIOCore
@preconcurrency import NIOPosix

/// Hosts MCP `Server` sessions over raw TCP on a local port. Each connection
/// gets its own `Server` + `LineFramedTransport` pair; the connection's
/// lifetime is the session's lifetime.
@available(macOS 13.0, *)
actor RemoteControlTCPApp {
	typealias ServerFactory = @Sendable () async -> Server

	private let host: String
	private let port: Int
	private let serverFactory: ServerFactory

	private var group: MultiThreadedEventLoopGroup?
	private var channel: Channel?

	private var sessions: [ObjectIdentifier: (server: Server, transport: LineFramedTransport)] = [:]

	init(host: String, port: Int, serverFactory: @escaping ServerFactory) {
		self.host = host
		self.port = port
		self.serverFactory = serverFactory
	}

	// MARK: - Lifecycle

	/// Binds the listening socket and starts accepting connections.
	/// Throws (e.g. address in use) without leaving anything running.
	func start() async throws {
		guard channel == nil else { return }

		let group = MultiThreadedEventLoopGroup(numberOfThreads: 1)

		let bootstrap = ServerBootstrap(group: group)
			.serverChannelOption(ChannelOptions.backlog, value: 16)
			.serverChannelOption(ChannelOptions.socketOption(.so_reuseaddr), value: 1)
			.childChannelInitializer { channel in
				channel.pipeline.addHandler(LineFramedHandler(app: self))
			}
			.childChannelOption(ChannelOptions.Types.SocketOption(level: .tcp, name: .tcp_nodelay), value: 1)

		do {
			channel = try await bootstrap.bind(host: host, port: port).get()
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

	private let app: RemoteControlTCPApp
	private var transport: LineFramedTransport?
	private var pending = Data()

	init(app: RemoteControlTCPApp) {
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
