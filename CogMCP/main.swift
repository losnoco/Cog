//
//  main.swift
//  cog-mcp
//
//  Created by Kevin López Brante on 2026-07-01.
//
//  Stdio bridge for Cog's MCP server. MCP clients launch this binary as a
//  stdio server; it pipes stdin/stdout to the newline-delimited JSON-RPC
//  listener the running Cog exposes on 127.0.0.1. No MCP logic lives here —
//  it is a dumb byte pump, so it needs no dependencies.
//

import Foundation

let defaultPort: UInt16 = 8089

func fail(_ message: String) -> Never {
	FileHandle.standardError.write(Data((message + "\n").utf8))
	exit(1)
}

// MARK: - Arguments

var port = defaultPort
var arguments = CommandLine.arguments.dropFirst().makeIterator()
while let argument = arguments.next() {
	switch argument {
	case "--port", "-p":
		guard let value = arguments.next(), let parsed = UInt16(value), parsed > 0 else {
			fail("cog-mcp: --port expects a number between 1 and 65535")
		}
		port = parsed
	case "--help", "-h":
		print("Usage: cog-mcp [--port N]")
		print("Bridges an MCP client (stdio) to the Cog audio player running on this Mac.")
		print("The port must match Cog's Remote Control preference (default \(defaultPort)).")
		exit(0)
	default:
		fail("cog-mcp: unknown argument '\(argument)' (try --help)")
	}
}

// MARK: - Connect

let sock = socket(AF_INET, SOCK_STREAM, 0)
guard sock >= 0 else {
	fail("cog-mcp: socket() failed: \(String(cString: strerror(errno)))")
}

var address = sockaddr_in()
address.sin_len = UInt8(MemoryLayout<sockaddr_in>.size)
address.sin_family = sa_family_t(AF_INET)
address.sin_port = port.bigEndian
address.sin_addr = in_addr(s_addr: inet_addr("127.0.0.1"))

let connected = withUnsafePointer(to: &address) {
	$0.withMemoryRebound(to: sockaddr.self, capacity: 1) {
		connect(sock, $0, socklen_t(MemoryLayout<sockaddr_in>.size))
	}
}
guard connected == 0 else {
	fail("""
	cog-mcp: can't reach Cog on 127.0.0.1:\(port).
	Make sure Cog is running and "Allow remote control via MCP" is enabled \
	in Cog's Preferences (and that the port there matches).
	""")
}

var nodelay: Int32 = 1
setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &nodelay, socklen_t(MemoryLayout<Int32>.size))

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
stdinPump.stackSize = 512 * 1024
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
