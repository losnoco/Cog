#!/usr/bin/env python3
"""Links a framework project to the iOS builds of Cog's third-party libraries.

Each library the project links from ThirdParty/ (a macOS .dylib or .a) is
left to macOS (platformFilters = macos), and the xcframework that
Scripts/build-ios-libraries.sh builds for it, ThirdParty/<dir>/ios/<lib>.xcframework,
is linked on iOS in its place. Run it once per project; running it again
changes nothing.

Usage: link-ios-libraries.py path/to/Project.xcodeproj ...
"""

import hashlib
import os
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent

# What each static library needs linked with it, which the macOS dylibs
# bring along by themselves.
DEPENDENCIES = {
	'ThirdParty/vorbis/ios/libvorbisfile.xcframework': ['ThirdParty/vorbis/ios/libvorbis.xcframework'],
	'ThirdParty/vorbis/ios/libvorbis.xcframework': ['ThirdParty/ogg/ios/libogg.xcframework'],
	'ThirdParty/opusfile/ios/libopusfile.xcframework': ['ThirdParty/opus/ios/libopus.xcframework', 'ThirdParty/ogg/ios/libogg.xcframework'],
	'ThirdParty/flac/ios/libFLAC.xcframework': ['ThirdParty/ogg/ios/libogg.xcframework'],
	'ThirdParty/ffmpeg/ios/libavformat.xcframework': ['ThirdParty/ffmpeg/ios/libavcodec.xcframework'],
	'ThirdParty/ffmpeg/ios/libavcodec.xcframework': ['ThirdParty/ffmpeg/ios/libavutil.xcframework', 'ThirdParty/fdk-aac/ios/libfdk-aac.xcframework'],
	'ThirdParty/ffmpeg/ios/libswresample.xcframework': ['ThirdParty/ffmpeg/ios/libavutil.xcframework'],
}


def uid(*parts):
	return hashlib.md5('\x1f'.join(parts).encode()).hexdigest()[:24].upper()


def convert(project):
	project = Path(project).resolve()
	pbxproj = project / 'project.pbxproj'
	s = pbxproj.read_text()
	references = {}
	for m in re.finditer(r'\t\t([0-9A-F]{24}) /\* [^*]+ \*/ = \{isa = PBXFileReference;[^\n]*?path = "?([^";]+\.(?:dylib|a))"?;', s):
		path = m.group(2)
		if 'ThirdParty/' not in path:
			# A path relative to its group: find the library by name.
			found = sorted(ROOT.glob(f'ThirdParty/*/lib/{Path(path).name}')) + sorted(ROOT.glob(f'ThirdParty/*/{Path(path).name}'))
			if not found:
				continue
			path = found[0].relative_to(ROOT).as_posix()
		references[m.group(1)] = path
	added_refs, added_builds = [], []
	linked = set()
	first_phase_entry = None
	for ref, path in references.items():
		third_party = path[path.index('ThirdParty/'):]
		directory = third_party.split('/')[1]
		library = Path(third_party).name.split('.')[0]
		xcframework = f'ThirdParty/{directory}/ios/{library}.xcframework'
		linked.add(xcframework)
		name = f'{library}.xcframework'
		new_ref = uid('ioslib', str(project), xcframework)
		for m in list(re.finditer(r'\t\t([0-9A-F]{24}) /\* ([^*]+) in Frameworks \*/ = \{isa = PBXBuildFile; fileRef = ' + ref + r' /\* [^*]+ \*/;( platformFilters = \(macos, \);)? \};', s)):
			build, label, filtered = m.group(1), m.group(2), m.group(3)
			if not filtered:
				s = s.replace(m.group(0), m.group(0)[:-3] + ' platformFilters = (macos, ); };')
			new_build = uid('iosbuild', str(project), build)
			first_phase_entry = first_phase_entry or new_build
			if new_build in s:
				continue
			added_builds.append(f'\t\t{new_build} /* {name} in Frameworks */ = {{isa = PBXBuildFile; fileRef = {new_ref} /* {name} */; platformFilters = (ios, ); }};')
			# Next to the macOS library in its Frameworks phase.
			s = re.sub(r'(\t+)' + build + r' /\* ' + re.escape(label) + r' in Frameworks \*/,\n',
			           lambda p: p.group(0) + f'{p.group(1)}{new_build} /* {name} in Frameworks */,\n', s)
			if new_ref not in s and all(new_ref not in r for r in added_refs):
				relative = os.path.relpath(ROOT / xcframework, project.parent)
				added_refs.append(f'\t\t{new_ref} /* {name} */ = {{isa = PBXFileReference; lastKnownFileType = wrapper.xcframework; name = {name}; path = "{relative}"; sourceTree = SOURCE_ROOT; }};')
				# In the group that holds the macOS library.
				s = re.sub(r'(\t+)' + ref + r' /\* [^*]+ \*/,\n', lambda p: p.group(0) + f'{p.group(1)}{new_ref} /* {name} */,\n', s, count=1)
	# The libraries those need, after the one that needs them.
	pending = sorted(linked)
	while pending:
		for dependency in DEPENDENCIES.get(pending.pop(0), []):
			if dependency in linked:
				continue
			linked.add(dependency)
			pending.append(dependency)
			name = Path(dependency).name
			new_ref = uid('ioslib', str(project), dependency)
			new_build = uid('iosdependency', str(project), dependency)
			if new_build in s or not first_phase_entry:
				continue
			relative = os.path.relpath(ROOT / dependency, project.parent)
			if new_ref not in s:
				added_refs.append(f'\t\t{new_ref} /* {name} */ = {{isa = PBXFileReference; lastKnownFileType = wrapper.xcframework; name = {name}; path = "{relative}"; sourceTree = SOURCE_ROOT; }};')
			added_builds.append(f'\t\t{new_build} /* {name} in Frameworks */ = {{isa = PBXBuildFile; fileRef = {new_ref} /* {name} */; platformFilters = (ios, ); }};')
			s = re.sub(r'(\t+)' + first_phase_entry + r' /\* [^*]+ in Frameworks \*/,\n',
			           lambda p: p.group(0) + f'{p.group(1)}{new_build} /* {name} in Frameworks */,\n', s, count=1)
	if added_builds:
		s = s.replace('/* End PBXBuildFile section */', '\n'.join(added_builds) + '\n/* End PBXBuildFile section */', 1)
	if added_refs:
		s = s.replace('/* End PBXFileReference section */', '\n'.join(added_refs) + '\n/* End PBXFileReference section */', 1)
	pbxproj.write_text(s)
	print(f'{project.name}: {len(added_builds)} iOS libraries linked')


for argument in sys.argv[1:]:
	convert(argument)
