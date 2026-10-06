#!/usr/bin/env python3
"""Makes every target of an Xcode project build for iOS as well as macOS.

- SDKROOT macosx -> auto, plus SUPPORTED_PLATFORMS / IPHONEOS_DEPLOYMENT_TARGET
  in each configuration that sets SDKROOT.
- System frameworks referenced by absolute /System/Library path become
  SDK-relative.
- Links to AppKit-only frameworks (Cocoa, AppKit, Carbon, CoreAudioKit,
  IOKit, ...) get platformFilters = (macos, ).

Usage: make-multiplatform.py path/to/project.pbxproj [extra-macos-only-name ...]
"""
import re
import sys

MAC_ONLY = {'Cocoa.framework', 'AppKit.framework', 'Carbon.framework', 'CoreAudioKit.framework',
            'IOKit.framework', 'AudioUnit.framework', 'ApplicationServices.framework',
            'VideoDecodeAcceleration.framework', 'CoreServices.framework', 'Quartz.framework'}

path = sys.argv[1]
mac_only = MAC_ONLY | set(sys.argv[2:])
s = open(path).read()

s = re.sub(r'path = /System/Library/Frameworks/(\w+)\.framework; sourceTree = "<absolute>";',
           r'path = System/Library/Frameworks/\1.framework; sourceTree = SDKROOT;', s)


def filter_build_file(m):
    line = m.group(0)
    if 'platformFilter' in line:
        return line
    return line[:-3] + ' platformFilters = (macos, ); };'


for name in mac_only:
    s = re.sub(r'\t\t[0-9A-F]{24} /\* ' + re.escape(name) + r' in \w+ \*/ = \{isa = PBXBuildFile; fileRef = [0-9A-F]{24} /\* [^*]+ \*/; \};',
               filter_build_file, s)


def fix_config(m):
    block = m.group(0)
    if 'SDKROOT = macosx;' not in block:
        return block
    block = block.replace('SDKROOT = macosx;', 'SDKROOT = auto;')
    indent = re.search(r'\n(\t+)SDKROOT = auto;', block).group(1)
    extra = ''
    for key, value in [('IPHONEOS_DEPLOYMENT_TARGET', '18.0'),
                       ('SUPPORTED_PLATFORMS', '"macosx iphoneos iphonesimulator"'),
                       ('SUPPORTS_MACCATALYST', 'NO'),
                       ('TARGETED_DEVICE_FAMILY', '"1,2"')]:
        block = re.sub(r'\n\t+' + key + r' = [^;]+;', '', block)
        extra += f'\n{indent}{key} = {value};'
    return block.replace(f'\n{indent}SDKROOT = auto;', f'\n{indent}SDKROOT = auto;' + extra)


if 'SDKROOT' not in s:
	# No SDK named anywhere means macOS: name one in every configuration.
	s = re.sub(r'(isa = XCBuildConfiguration;.*?buildSettings = \{\n)(\t+)', lambda m: m.group(1) + m.group(2) + 'SDKROOT = macosx;\n' + m.group(2), s, flags=re.S)
s = re.sub(r'isa = XCBuildConfiguration;.*?name = \w+;', fix_config, s, flags=re.S)
open(path, 'w').write(s)
