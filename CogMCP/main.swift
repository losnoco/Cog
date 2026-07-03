//
//  main.swift
//  cog-mcp
//
//  Created by Kevin López Brante on 2026-07-01.
//
//  Stdio bridge for Cog's MCP server. MCP clients launch this binary as a
//  stdio server; it pipes stdin/stdout to the newline-delimited JSON-RPC
//  listener the running Cog exposes on a Unix domain socket in the shared
//  app-group folder. No MCP logic lives here — it is a dumb byte pump, so it
//  needs no dependencies.
//

import Foundation

let appGroupIdentifier = "group.org.cogx.cog"
let socketName = "ipc.sock"

func fail(_ message: String) -> Never {
	FileHandle.standardError.write(Data((message + "\n").utf8))
	exit(1)
}

// MARK: - Arguments

var arguments = CommandLine.arguments.dropFirst().makeIterator()
while let argument = arguments.next() {
	switch argument {
	case "--help", "-h":
		print("Usage: cog-mcp")
		print("Bridges an MCP client (stdio) to the Cog audio player running on this Mac,")
		print("over the Unix domain socket Cog exposes in its shared app-group folder.")
		exit(0)
	default:
		fail("cog-mcp: unknown argument '\(argument)' (try --help)")
	}
}

// MARK: - Connect

/// The socket lives in the app-group container Cog and this helper share.
/// containerURL works on macOS even without the group entitlement in effect
/// (e.g. when inheriting an unsandboxed parent), but fall back to the
/// well-known location just in case.
let socketPath: String = {
	if let container = FileManager.default
		.containerURL(forSecurityApplicationGroupIdentifier: appGroupIdentifier) {
		return container.appendingPathComponent(socketName).path
	}
	return NSHomeDirectory() + "/Library/Group Containers/\(appGroupIdentifier)/\(socketName)"
}()

let sock = socket(AF_UNIX, SOCK_STREAM, 0)
guard sock >= 0 else {
	fail("cog-mcp: socket() failed: \(String(cString: strerror(errno)))")
}

var address = sockaddr_un()
address.sun_len = UInt8(MemoryLayout<sockaddr_un>.size)
address.sun_family = sa_family_t(AF_UNIX)
let pathBytes = Array(socketPath.utf8)
let sunPathCapacity = MemoryLayout.size(ofValue: address.sun_path)
guard pathBytes.count < sunPathCapacity else {
	fail("cog-mcp: socket path is too long: \(socketPath)")
}
withUnsafeMutableBytes(of: &address.sun_path) { sunPath in
	sunPath.copyBytes(from: pathBytes)
}

let connected = withUnsafePointer(to: &address) {
	$0.withMemoryRebound(to: sockaddr.self, capacity: 1) {
		connect(sock, $0, socklen_t(MemoryLayout<sockaddr_un>.size))
	}
}
guard connected == 0 else {
	fail("""
	cog-mcp: can't reach Cog at \(socketPath).
	Make sure Cog is running and "Allow remote control via MCP" is enabled \
	in Cog's Preferences.
	""")
}

// Writing to a peer that vanished raises SIGPIPE; handle it as EPIPE instead.
signal(SIGPIPE, SIG_IGN)

// MARK: - Pump

/// read(2) with EINTR retry.
func readSome(_ fd: Int32, _ buffer: inout [UInt8]) -> Int {
	while true {
		let n = read(fd, &buffer, buffer.count)
		if n >= 0 || errno != EINTR { return n }
	}
}

/// Writes the whole range, retrying on EINTR. Returns false on error.
func writeAll(_ fd: Int32, _ buffer: [UInt8], count: Int) -> Bool {
	var offset = 0
	while offset < count {
		let n = buffer.withUnsafeBytes {
			write(fd, $0.baseAddress! + offset, count - offset)
		}
		if n > 0 {
			offset += n
		} else if errno != EINTR {
			return false
		}
	}
	return true
}

// stdin → socket. On stdin EOF (client quit), half-close the socket so Cog
// ends the session; the socket→stdout loop then finishes and the process exits.
let stdinPump = Thread {
	var buffer = [UInt8](repeating: 0, count: 65536)
	while true {
		let n = readSome(STDIN_FILENO, &buffer)
		if n <= 0 || !writeAll(sock, buffer, count: n) {
			shutdown(sock, SHUT_WR)
			return
		}
	}
}
stdinPump.start()

// socket → stdout, on the main thread. Ends when Cog closes the connection
// (server stopped or app quit) or after the stdin-side half-close drains.
var buffer = [UInt8](repeating: 0, count: 65536)
while true {
	let n = readSome(sock, &buffer)
	if n <= 0 { break }
	if !writeAll(STDOUT_FILENO, buffer, count: n) { break }
}
exit(0)
