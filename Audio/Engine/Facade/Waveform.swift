//
//  Waveform.swift
//  CogAudio
//
//  A whole track's mono waveform at a fixed resolution, as the seek bar
//  draws it on both platforms.
//

import CoreGraphics
import Foundation

public struct Waveform: Equatable {
	public static let bucketCount = 2048

	/// The lowest and highest sample, and the RMS, of each bucket: an equal
	/// share of the track's frames.
	public var minimum: [Float]
	public var maximum: [Float]
	public var rms: [Float]
	/// Which buckets the scan has reached; the rest are not drawn. A scan
	/// in segments reaches several stretches at once.
	public var scanned: [Bool]

	public init() {
		minimum = [Float](repeating: 0, count: Self.bucketCount)
		maximum = minimum
		rms = minimum
		scanned = [Bool](repeating: false, count: Self.bucketCount)
	}

	public var isComplete: Bool { !scanned.contains(false) }

	// MARK: - Drawing

	/// How tall `sample` draws, from 0 to 1: linear, or in decibels above a
	/// -60 dB floor.
	public static func level(_ sample: Float, logScale: Bool) -> CGFloat {
		let magnitude = abs(sample)
		guard logScale else { return CGFloat(min(magnitude, 1)) }
		guard magnitude > 0 else { return 0 }
		return CGFloat(min(max((20 * log10(magnitude) + 60) / 60, 0), 1))
	}

	/// The peak and RMS outlines over `rect`, one point per column, where
	/// the scan has reached. Rectified, both grow up from the bottom edge;
	/// otherwise the positive half rises from the middle and the negative
	/// half falls from it. `flipped` is true where y grows downward.
	public func paths(in rect: CGRect, columns: Int, logScale: Bool, rectified: Bool, flipped: Bool) -> (peak: CGPath, rms: CGPath) {
		let peak = CGMutablePath()
		let rmsPath = CGMutablePath()
		let columns = max(columns, 1)
		guard rect.width > 0, rect.height > 0 else { return (peak, rmsPath) }

		// Distance from the baseline, toward the top of the bar.
		let up: CGFloat = flipped ? -1 : 1
		let baseline = rectified ? (flipped ? rect.maxY : rect.minY) : rect.midY
		let span = rectified ? rect.height : rect.height / 2
		let step = rect.width / CGFloat(columns)
		// The thinnest line drawn, so silence still shows where the track is.
		let hairline: CGFloat = 0.5

		// Each stretch the scan has reached is an outline of its own.
		var peakTop: [CGPoint] = []
		var peakBottom: [CGPoint] = []
		var rmsTop: [CGPoint] = []
		var rmsBottom: [CGPoint] = []
		func closeStretch() {
			guard !peakTop.isEmpty else { return }
			peak.addLines(between: peakTop + peakBottom.reversed())
			peak.closeSubpath()
			rmsPath.addLines(between: rmsTop + rmsBottom.reversed())
			rmsPath.closeSubpath()
			peakTop.removeAll(keepingCapacity: true)
			peakBottom.removeAll(keepingCapacity: true)
			rmsTop.removeAll(keepingCapacity: true)
			rmsBottom.removeAll(keepingCapacity: true)
		}

		for column in 0..<columns {
			let first = column * Self.bucketCount / columns
			let last = max(first + 1, (column + 1) * Self.bucketCount / columns)
			var low: Float = 0
			var high: Float = 0
			var power: Float = 0
			var reached = false
			for bucket in first..<min(last, Self.bucketCount) where scanned[bucket] {
				reached = true
				low = min(low, minimum[bucket])
				high = max(high, maximum[bucket])
				power = max(power, rms[bucket])
			}
			guard reached else {
				closeStretch()
				continue
			}

			let above: CGFloat
			let below: CGFloat
			let rmsAbove: CGFloat
			let rmsBelow: CGFloat
			if rectified {
				above = max(Self.level(max(-low, high), logScale: logScale) * span, hairline)
				below = 0
				rmsAbove = Self.level(power, logScale: logScale) * span
				rmsBelow = 0
			} else {
				above = max(Self.level(high, logScale: logScale) * span, hairline / 2)
				below = max(Self.level(low, logScale: logScale) * span, hairline / 2)
				rmsAbove = Self.level(power, logScale: logScale) * span
				rmsBelow = rmsAbove
			}

			// Each column's value holds across its width, so the outline
			// steps rather than slopes between buckets.
			for x in [rect.minX + CGFloat(column) * step, rect.minX + CGFloat(column + 1) * step] {
				peakTop.append(CGPoint(x: x, y: baseline + up * above))
				peakBottom.append(CGPoint(x: x, y: baseline - up * below))
				rmsTop.append(CGPoint(x: x, y: baseline + up * rmsAbove))
				rmsBottom.append(CGPoint(x: x, y: baseline - up * rmsBelow))
			}
		}
		closeStretch()
		return (peak, rmsPath)
	}

	// MARK: - Storage

	private static let magic: UInt32 = 0x4357_4652 // "CWFR"
	private static let version: UInt32 = 1

	/// The waveform as the cache stores it: a header, then the three arrays
	/// as native floats. Only a complete waveform is stored.
	public func encoded() -> Data {
		var data = Data()
		for value in [Self.magic, Self.version, UInt32(Self.bucketCount)] {
			withUnsafeBytes(of: value) { data.append(contentsOf: $0) }
		}
		for array in [minimum, maximum, rms] {
			array.withUnsafeBytes { data.append(contentsOf: $0) }
		}
		return data
	}

	public init?(encoded data: Data) {
		let header = 3 * MemoryLayout<UInt32>.size
		let arrayBytes = Self.bucketCount * MemoryLayout<Float>.size
		guard data.count == header + 3 * arrayBytes else { return nil }
		let fields = data.withUnsafeBytes { raw in
			(0..<3).map { raw.loadUnaligned(fromByteOffset: $0 * MemoryLayout<UInt32>.size, as: UInt32.self) }
		}
		guard fields == [Self.magic, Self.version, UInt32(Self.bucketCount)] else { return nil }
		func floats(_ index: Int) -> [Float] {
			let start = data.startIndex + header + index * arrayBytes
			var array = [Float](repeating: 0, count: Self.bucketCount)
			array.withUnsafeMutableBytes { _ = data.copyBytes(to: $0, from: start..<start + arrayBytes) }
			return array
		}
		minimum = floats(0)
		maximum = floats(1)
		rms = floats(2)
		scanned = [Bool](repeating: true, count: Self.bucketCount)
	}
}
