#!/usr/bin/env python3
"""Generates iOS/MobileCog.xcodeproj: the Cog app for iOS, and CogPlugins.

On macOS each plugin is a loadable bundle built by its own project under
Plugins/. iOS loads no code bundles, so there the plugins are compiled from
the same sources into one framework, CogPlugins, which the app links;
PluginController finds their classes at launch.

To add a plugin, add it to PLUGINS (and its libraries, if any) and run this
script again. Do not edit the generated project by hand.
"""

import hashlib
import os
import re
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parent
PROJECT = HERE / 'MobileCog.xcodeproj'

IOS_DEPLOYMENT_TARGET = '18.0'

# Each plugin: its source directory (relative to the repository), and
# optionally files there to leave out ('exclude') or the only ones to build
# ('sources', relative to the directory, when it holds unused ones), files
# from elsewhere it compiles too ('extra_sources', relative to the
# repository), header search paths
# ('headers'), the iOS libraries it links ('libraries': xcframeworks that
# Scripts/build-ios-libraries.sh builds, relative to the repository), the
# framework projects it links ('projects', from Frameworks/, built for iOS
# as well), compiler flags for its sources ('cflags'), other linker flags
# ('ldflags', for system libraries) and files it loads from its bundle
# ('resources', relative to its directory), which iOS finds in CogPlugins.
PLUGINS = [
	{'name': 'CoreAudio', 'dir': 'Plugins/CoreAudio'},
	{'name': 'CueSheet', 'dir': 'Plugins/CueSheet'},
	{'name': 'FFMPEG', 'dir': 'Plugins/FFMPEG',
	 'headers': ['ThirdParty/ffmpeg/include'],
	 'libraries': ['ThirdParty/ffmpeg/ios/libavcodec.xcframework', 'ThirdParty/ffmpeg/ios/libavformat.xcframework',
	               'ThirdParty/ffmpeg/ios/libavutil.xcframework', 'ThirdParty/ffmpeg/ios/libswresample.xcframework',
	               'ThirdParty/fdk-aac/ios/libfdk-aac.xcframework'],
	 'ldflags': ['-lz', '-lbz2', '-liconv']},
	{'name': 'FileSource', 'dir': 'Plugins/FileSource'},
	{'name': 'Flac', 'dir': 'Plugins/Flac',
	 'headers': ['ThirdParty/flac/include', 'ThirdParty/ogg/include'],
	 'libraries': ['ThirdParty/flac/ios/libFLAC.xcframework', 'ThirdParty/ogg/ios/libogg.xcframework']},
	{'name': 'HTTPSource', 'dir': 'Plugins/HTTPSource'},
	{'name': 'M3u', 'dir': 'Plugins/M3u'},
	{'name': 'Pls', 'dir': 'Plugins/Pls'},
	{'name': 'SilenceDecoder', 'dir': 'Plugins/SilenceDecoder/SilenceDecoder'},
	{'name': 'AdPlug', 'dir': 'Plugins/AdPlug/AdPlug',
	 'headers': ['Frameworks/libbinio/libbinio/libbinio/src', 'Frameworks/libbinio/libbinio'],
	 'resources': ['../../../Frameworks/AdPlug/AdPlug/database/adplug.db'],
	 'projects': ['Frameworks/AdPlug/libAdPlug.xcodeproj', 'Frameworks/libbinio/libbinio.xcodeproj']},
	{'name': 'APL', 'dir': 'Plugins/APL'},
	{'name': 'ArchiveSource', 'dir': 'Plugins/ArchiveSource/ArchiveSource'},
	{'name': 'GME', 'dir': 'Plugins/GME', 'projects': ['Frameworks/GME/GME.xcodeproj']},
	{'name': 'HighlyComplete', 'dir': 'Plugins/HighlyComplete/HighlyComplete',
	 'headers': ['Frameworks/mGBA/mGBA/mgba/include'],
	 'cflags': '-DEMU_LITTLE_ENDIAN -DHAVE_STDINT_H -DMINIMAL_CORE=2 -DMGBA_STANDALONE',
	 'projects': ['Frameworks/HighlyExperimental/HighlyExperimental.xcodeproj', 'Frameworks/HighlyQuixotic/HighlyQuixotic.xcodeproj',
	              'Frameworks/HighlyTheoretical/HighlyTheoretical.xcodeproj', 'Frameworks/lazyusf2/lazyusf2.xcodeproj',
	              'Frameworks/mGBA/mGBA.xcodeproj', 'Frameworks/psflib/psflib.xcodeproj', 'Frameworks/snes9x/snes9x.xcodeproj',
	              'Frameworks/SSEQPlayer/SSEQPlayer.xcodeproj', 'Frameworks/vio2sf/vio2sf.xcodeproj']},
	{'name': 'Hively', 'dir': 'Plugins/Hively/Hively', 'projects': ['Frameworks/HivelyPlayer/HivelyPlayer.xcodeproj']},
	{'name': 'Musepack', 'dir': 'Plugins/Musepack', 'projects': ['Frameworks/MPCDec/MPCDec.xcodeproj']},
	{'name': 'OpenMPT', 'dir': 'Plugins/OpenMPT/OpenMPT', 'projects': ['Frameworks/OpenMPT/libOpenMPT.xcodeproj']},
	{'name': 'Shorten', 'dir': 'Plugins/Shorten', 'projects': ['Frameworks/Shorten/Shorten.xcodeproj']},
	{'name': 'sidplay', 'dir': 'Plugins/sidplay', 'projects': ['Frameworks/libsidplayfp/sidplayfp.xcodeproj']},
	{'name': 'Syntrax', 'dir': 'Plugins/Syntrax/Syntrax', 'projects': ['Frameworks/Syntrax-c/Syntrax_c.xcodeproj']},
	{'name': 'vgmstream', 'dir': 'Plugins/vgmstream/vgmstream',
	 'headers': ['ThirdParty/ffmpeg/include'],
	 'cflags': '-DVGM_USE_ATRAC9 -DVGM_USE_FFMPEG -DVGM_USE_G719 -DVGM_USE_G7221 -DVGM_USE_MPEG -DVGM_USE_VORBIS -D__MACOSX__',
	 'projects': ['Frameworks/vgmstream/libvgmstream.xcodeproj']},
	{'name': 'HLS', 'dir': 'Plugins/HLS'},
	{'name': 'libvgmPlayer', 'dir': 'Plugins/libvgmPlayer',
	 'headers': ['ThirdParty/libvgm/include'],
	 'libraries': ['ThirdParty/libvgm/ios/libvgm-player.xcframework', 'ThirdParty/libvgm/ios/libvgm-emu.xcframework',
	               'ThirdParty/libvgm/ios/libvgm-utils.xcframework'],
	 'ldflags': ['-lz', '-liconv']},
	{'name': 'MIDI', 'dir': 'Plugins/MIDI/MIDI',
	 'sources': ['AUPlayer.mm', 'MIDIContainer.mm', 'MIDIDecoder.mm', 'MIDIMetadataReader.mm', 'MIDIPlayer.cpp', 'MSPlayer.cpp',
	             'resampler.c', 'SCPlayer.mm', 'SpessaPlayer.mm', 'synthlib_doom/i_oplmusic.cpp', 'synthlib_opl3w/opl3midi.cpp',
	             'fmopl3lib/opl3.cpp', 'fmopl3lib/opl3class.cpp'],
	 'extra_sources': ['Utils/MIDIPluginState.m'],
	 'headers': ['ThirdParty/json'],
	 'projects': ['Frameworks/nuked-sc55/nuked-sc55.xcodeproj', 'Plugins/MIDI/MIDI/spessasynth_core/spessasynth_core.xcodeproj']},
	{'name': 'minimp3', 'dir': 'Plugins/minimp3',
	 'headers': ['ThirdParty/libid3tag/include'],
	 'libraries': ['ThirdParty/libid3tag/ios/libid3tag.xcframework'], 'ldflags': ['-lz']},
	{'name': 'Opus', 'dir': 'Plugins/Opus/Opus',
	 'headers': ['ThirdParty/opusfile/include', 'ThirdParty/opus/include', 'ThirdParty/ogg/include', 'ThirdParty/flac/include'],
	 'libraries': ['ThirdParty/opusfile/ios/libopusfile.xcframework', 'ThirdParty/opus/ios/libopus.xcframework',
	               'ThirdParty/ogg/ios/libogg.xcframework', 'ThirdParty/flac/ios/libFLAC.xcframework']},
	{'name': 'Organya', 'dir': 'Plugins/Organya',
	 'resources': ['wavetable.dat', 'fx96.pxt', 'fx97.pxt', 'fx98.pxt', 'fx99.pxt', 'fx9a.pxt', 'fx9b.pxt']},
	{'name': 'Vorbis', 'dir': 'Plugins/Vorbis',
	 'headers': ['Plugins/Vorbis/vorbis-tools/include', 'ThirdParty/vorbis/include', 'ThirdParty/ogg/include', 'ThirdParty/flac/include'],
	 'libraries': ['ThirdParty/vorbis/ios/libvorbisfile.xcframework', 'ThirdParty/vorbis/ios/libvorbis.xcframework',
	               'ThirdParty/ogg/ios/libogg.xcframework', 'ThirdParty/flac/ios/libFLAC.xcframework']},
	{'name': 'WavPack', 'dir': 'Plugins/WavPack',
	 'headers': ['ThirdParty/WavPack/include'],
	 'libraries': ['ThirdParty/WavPack/ios/libwavpack.xcframework']},
	{'name': 'TagLib', 'dir': 'Plugins/TagLib',
	 'libraries': ['ThirdParty/taglib/ios/libtag.xcframework'], 'ldflags': ['-lz']},
]

# Header search paths shared by all plugins, relative to the repository.
HEADER_SEARCH_PATHS = [
	'Audio',
	'Audio/Shared',
	'Audio/Utils',
	'Utils',
	'Playlist',
]

# Projects whose framework every plugin links.
COMMON_PROJECTS = ['Audio/CogAudio.xcodeproj', 'Frameworks/File_Extractor/File_Extractor.xcodeproj']


def framework_target(project):
	"""(product name, target ID, product ID) of a project's framework target."""
	text = (ROOT / project / 'project.pbxproj').read_text(errors='replace')
	for m in re.finditer(r'\t\t([0-9A-F]{24}) /\* [^*]+ \*/ = \{\n\t\t\tisa = PBXNativeTarget;(.*?)\n\t\t\};', text, re.S):
		body = m.group(2)
		if 'com.apple.product-type.framework' not in body:
			continue
		product = re.search(r'productReference = ([0-9A-F]{24}) /\* ([^*]+)\.framework \*/', body)
		return product.group(2), m.group(1), product.group(1)
	raise SystemExit(f'No framework target in {project}')


# Projects only the tests link: the shared playlist, whose loader they try
# with the plugins. (The app links it too.)
TEST_PROJECTS = ['CogPlaylist/CogPlaylist.xcodeproj']

# Frameworks the plugins' frameworks link in turn (vgmstream's), which the
# app must embed as well.
EMBED_PROJECTS = ['Frameworks/g719/g719.xcodeproj', 'Frameworks/libatrac9/libatrac9.xcodeproj',
                  'Frameworks/libcelt_0061/libcelt_0061/libcelt_0061.xcodeproj',
                  'Frameworks/libcelt_0110/libcelt_0110/libcelt_0110.xcodeproj']

# What the app bundles: the MIDI plugin's SoundFonts (it looks for them in
# the main bundle, as on macOS), the equalizer's presets and Cog's icon.
APP_RESOURCES = ['GeneralUserGS.sf3', 'GeneralUserGS-Drums.sf3', 'GeneralUserXG-SFeTest.sf3', 'tg300b.sflist.json',
                 'Cog.q1.json']
APP_ICON = 'Play.icon'

# The macOS app's sources the iOS app compiles as they are: lyrics from
# LRCLIB, and scrobbling to Last.fm and ListenBrainz. Generated/Secrets.swift
# (the Last.fm key) is written by Scripts/generate-swift-secrets.sh as the
# app builds, as on macOS.
APP_SHARED_SOURCES = ['LyricsWindow/LrclibClient.swift', 'LyricsWindow/LyricsLookup.swift',
                      'Scrobbler/AudioScrobbler.swift', 'Scrobbler/LastFMAPI.swift', 'Scrobbler/ListenBrainzAPI.swift',
                      'Scrobbler/ListenBrainzScrobbler.swift', 'Scrobbler/KeychainHelper.swift', 'Generated/Secrets.swift']

SUBPROJECTS = []
for project in COMMON_PROJECTS + [p for plugin in PLUGINS for p in plugin.get('projects', [])] + TEST_PROJECTS + EMBED_PROJECTS:
	if project not in [entry[1] for entry in SUBPROJECTS]:
		name, target_id, product_id = framework_target(project)
		SUBPROJECTS.append((name, project, target_id, product_id))

SYSTEM_FRAMEWORKS = ['AudioToolbox', 'AVFoundation', 'CoreMedia', 'CoreMIDI', 'Foundation', 'Security']

SOURCE_TYPES = {
	'.m': 'sourcecode.c.objc',
	'.mm': 'sourcecode.cpp.objcpp',
	'.c': 'sourcecode.c.c',
	'.cpp': 'sourcecode.cpp.cpp',
	'.cc': 'sourcecode.cpp.cpp',
	'.swift': 'sourcecode.swift',
}
HEADER_TYPES = {'.h': 'sourcecode.c.h', '.hpp': 'sourcecode.cpp.h'}


def explicit_file_types(directory):
	"""File types the plugin's macOS project sets by hand (an .m compiled as
	Objective-C++, say), by file name."""
	types = {}
	for project in list(directory.glob('*.xcodeproj')) + list(directory.parent.glob('*.xcodeproj')):
		text = (project / 'project.pbxproj').read_text(errors='replace')
		for match in re.finditer(r'/\* ([^*]+) \*/ = \{isa = PBXFileReference; explicitFileType = ([\w.+-]+);', text):
			types[match.group(1)] = match.group(2)
	return types


def uid(*parts):
	return hashlib.md5('\x1f'.join(parts).encode()).hexdigest()[:24].upper()


def quote(value):
	value = str(value)
	if value and all(c.isalnum() or c in '._/' for c in value) and not value[0].isdigit():
		return value
	return '"' + value.replace('\\', '\\\\').replace('"', '\\"') + '"'


def rel(path):
	"""A repository path as seen from the project directory."""
	return os.path.relpath(ROOT / path, HERE)


objects = {}  # section -> list of (id, comment, body)


def add(section, ident, comment, body):
	objects.setdefault(section, []).append((ident, comment, body))


class Ref(str):
	"""An object reference, rendered with its comment."""
	def __new__(cls, ident, comment):
		return super().__new__(cls, f'{ident} /* {comment} */')


# MARK: - Files

plugin_groups = []
plugin_sources = []  # build file refs for the Sources phase
plugin_resources = []  # and for the Resources phase
for plugin in PLUGINS:
	directory = ROOT / plugin['dir']
	exclude = set(plugin.get('exclude', []))
	explicit = explicit_file_types(directory)
	children = []
	extra = [ROOT / path for path in plugin.get('extra_sources', [])]
	for path in sorted(directory.rglob('*')) + extra:
		if not path.is_file() or any(part.endswith(('.xcodeproj', '.lproj')) for part in path.parts):
			continue
		relative = path.relative_to(ROOT).as_posix()
		if path.name in exclude or relative in exclude:
			continue
		if 'sources' in plugin and path not in extra and path.relative_to(directory).as_posix() not in plugin['sources']:
			continue
		ext = path.suffix
		if ext not in SOURCE_TYPES and ext not in HEADER_TYPES:
			continue
		file_id = uid('file', relative)
		kind = ('explicitFileType', explicit[path.name]) if path.name in explicit else \
			('lastKnownFileType', SOURCE_TYPES.get(ext) or HEADER_TYPES[ext])
		add('PBXFileReference', file_id, path.name, {
			'isa': 'PBXFileReference', kind[0]: kind[1], 'fileEncoding': '4',
			'name': path.name, 'path': rel(relative), 'sourceTree': 'SOURCE_ROOT'})
		children.append(Ref(file_id, path.name))
		if ext in SOURCE_TYPES:
			build_id = uid('build', relative)
			build_file = {'isa': 'PBXBuildFile', 'fileRef': Ref(file_id, path.name)}
			if plugin.get('cflags'):
				build_file['settings'] = {'COMPILER_FLAGS': plugin['cflags']}
			add('PBXBuildFile', build_id, f'{path.name} in Sources', build_file)
			plugin_sources.append(Ref(build_id, f'{path.name} in Sources'))
	for resource in plugin.get('resources', []):
		relative = os.path.normpath(f"{plugin['dir']}/{resource}")
		name = Path(resource).name
		file_id = uid('file', relative)
		add('PBXFileReference', file_id, name, {
			'isa': 'PBXFileReference', 'lastKnownFileType': 'file', 'name': name,
			'path': rel(relative), 'sourceTree': 'SOURCE_ROOT'})
		children.append(Ref(file_id, name))
		build_id = uid('resource', relative)
		add('PBXBuildFile', build_id, f'{name} in Resources', {'isa': 'PBXBuildFile', 'fileRef': Ref(file_id, name)})
		plugin_resources.append(Ref(build_id, f'{name} in Resources'))
	group_id = uid('group', plugin['name'])
	add('PBXGroup', group_id, plugin['name'], {
		'isa': 'PBXGroup', 'children': children, 'name': plugin['name'], 'sourceTree': '<group>'})
	plugin_groups.append(Ref(group_id, plugin['name']))

# Support files, in iOS/CogPlugins
support_children = []
for name, kind in [('CogPlugins_Prefix.pch', 'sourcecode.c.h'), ('CogPlugins.xcconfig', 'text.xcconfig')]:
	file_id = uid('support', name)
	add('PBXFileReference', file_id, name, {
		'isa': 'PBXFileReference', 'fileEncoding': '4', 'lastKnownFileType': kind,
		'path': f'CogPlugins/{name}', 'sourceTree': '<group>'})
	support_children.append(Ref(file_id, name))
XCCONFIG = Ref(uid('support', 'CogPlugins.xcconfig'), 'CogPlugins.xcconfig')

# Tests, in iOS/CogPluginsTests
test_children = []
test_sources = []
for path in sorted((HERE / 'CogPluginsTests').glob('*.swift')):
	file_id = uid('test', path.name)
	add('PBXFileReference', file_id, path.name, {
		'isa': 'PBXFileReference', 'fileEncoding': '4', 'lastKnownFileType': 'sourcecode.swift',
		'path': f'CogPluginsTests/{path.name}', 'sourceTree': '<group>'})
	test_children.append(Ref(file_id, path.name))
	build_id = uid('testbuild', path.name)
	add('PBXBuildFile', build_id, f'{path.name} in Sources', {'isa': 'PBXBuildFile', 'fileRef': Ref(file_id, path.name)})
	test_sources.append(Ref(build_id, f'{path.name} in Sources'))

# MARK: - Frameworks

framework_children = []
plugin_links = []
test_links = []
for library in sorted({library for plugin in PLUGINS for library in plugin.get('libraries', [])}):
	name = Path(library).name
	file_id = uid('library', library)
	add('PBXFileReference', file_id, name, {
		'isa': 'PBXFileReference', 'lastKnownFileType': 'wrapper.xcframework', 'name': name,
		'path': rel(library), 'sourceTree': 'SOURCE_ROOT'})
	framework_children.append(Ref(file_id, name))
	build_id = uid('librarybuild', library)
	add('PBXBuildFile', build_id, f'{name} in Frameworks', {'isa': 'PBXBuildFile', 'fileRef': Ref(file_id, name)})
	plugin_links.append(Ref(build_id, f'{name} in Frameworks'))
for name in SYSTEM_FRAMEWORKS:
	file_id = uid('sysfw', name)
	add('PBXFileReference', file_id, f'{name}.framework', {
		'isa': 'PBXFileReference', 'lastKnownFileType': 'wrapper.framework', 'name': f'{name}.framework',
		'path': f'System/Library/Frameworks/{name}.framework', 'sourceTree': 'SDKROOT'})
	framework_children.append(Ref(file_id, f'{name}.framework'))
	build_id = uid('sysfwbuild', name)
	add('PBXBuildFile', build_id, f'{name}.framework in Frameworks', {
		'isa': 'PBXBuildFile', 'fileRef': Ref(file_id, f'{name}.framework')})
	plugin_links.append(Ref(build_id, f'{name}.framework in Frameworks'))

TARGET_ID = uid('target', 'CogPlugins')
TEST_TARGET_ID = uid('target', 'CogPluginsTests')
PRODUCT_ID = uid('product', 'CogPlugins')
TEST_PRODUCT_ID = uid('product', 'CogPluginsTests')
PROJECT_ID = uid('project', 'CogPlugins')

project_references = []
plugin_dependencies = []
test_dependencies = []
app_dependencies = []
framework_refs = {}
subproject_children = []
for name, path, target_id, product_id in SUBPROJECTS:
	project_file = uid('subproject', name)
	add('PBXFileReference', project_file, f'{name}.xcodeproj', {
		'isa': 'PBXFileReference', 'lastKnownFileType': 'wrapper.pb-project', 'name': f'{name}.xcodeproj',
		'path': rel(path), 'sourceTree': 'SOURCE_ROOT'})
	subproject_children.append(Ref(project_file, f'{name}.xcodeproj'))
	product_proxy = uid('productproxy', name)
	add('PBXContainerItemProxy', product_proxy, 'PBXContainerItemProxy', {
		'isa': 'PBXContainerItemProxy', 'containerPortal': Ref(project_file, f'{name}.xcodeproj'),
		'proxyType': '2', 'remoteGlobalIDString': product_id, 'remoteInfo': name})
	reference_proxy = uid('referenceproxy', name)
	add('PBXReferenceProxy', reference_proxy, f'{name}.framework', {
		'isa': 'PBXReferenceProxy', 'fileType': 'wrapper.framework', 'path': f'{name}.framework',
		'remoteRef': Ref(product_proxy, 'PBXContainerItemProxy'), 'sourceTree': 'BUILT_PRODUCTS_DIR'})
	products_group = uid('productsgroup', name)
	add('PBXGroup', products_group, 'Products', {
		'isa': 'PBXGroup', 'children': [Ref(reference_proxy, f'{name}.framework')], 'name': 'Products',
		'sourceTree': '<group>'})
	project_references.append({'ProductGroup': Ref(products_group, 'Products'), 'ProjectRef': Ref(project_file, f'{name}.xcodeproj')})
	target_proxy = uid('targetproxy', name)
	add('PBXContainerItemProxy', target_proxy, 'PBXContainerItemProxy', {
		'isa': 'PBXContainerItemProxy', 'containerPortal': Ref(project_file, f'{name}.xcodeproj'),
		'proxyType': '1', 'remoteGlobalIDString': target_id, 'remoteInfo': name})
	dependency = uid('dependency', name)
	add('PBXTargetDependency', dependency, 'PBXTargetDependency', {
		'isa': 'PBXTargetDependency', 'name': name, 'targetProxy': Ref(target_proxy, 'PBXContainerItemProxy')})
	framework_refs[name] = reference_proxy
	if path in TEST_PROJECTS:
		test_dependencies.append(Ref(dependency, 'PBXTargetDependency'))
		app_dependencies.append(Ref(dependency, 'PBXTargetDependency'))
	elif path in EMBED_PROJECTS:
		pass
	else:
		plugin_dependencies.append(Ref(dependency, 'PBXTargetDependency'))
		build_id = uid('subprojectbuild', name)
		add('PBXBuildFile', build_id, f'{name}.framework in Frameworks', {
			'isa': 'PBXBuildFile', 'fileRef': Ref(reference_proxy, f'{name}.framework')})
		plugin_links.append(Ref(build_id, f'{name}.framework in Frameworks'))
	if name == 'CogAudio' or path in TEST_PROJECTS:
		test_build = uid('testlink', name)
		add('PBXBuildFile', test_build, f'{name}.framework in Frameworks', {
			'isa': 'PBXBuildFile', 'fileRef': Ref(reference_proxy, f'{name}.framework')})
		test_links.append(Ref(test_build, f'{name}.framework in Frameworks'))

add('PBXFileReference', PRODUCT_ID, 'CogPlugins.framework', {
	'isa': 'PBXFileReference', 'explicitFileType': 'wrapper.framework', 'includeInIndex': '0',
	'path': 'CogPlugins.framework', 'sourceTree': 'BUILT_PRODUCTS_DIR'})
add('PBXFileReference', TEST_PRODUCT_ID, 'CogPluginsTests.xctest', {
	'isa': 'PBXFileReference', 'explicitFileType': 'wrapper.cfbundle', 'includeInIndex': '0',
	'path': 'CogPluginsTests.xctest', 'sourceTree': 'BUILT_PRODUCTS_DIR'})
test_link_plugins = uid('testlink', 'CogPlugins')
add('PBXBuildFile', test_link_plugins, 'CogPlugins.framework in Frameworks', {
	'isa': 'PBXBuildFile', 'fileRef': Ref(PRODUCT_ID, 'CogPlugins.framework')})
test_links.append(Ref(test_link_plugins, 'CogPlugins.framework in Frameworks'))

# MARK: - Groups

# MARK: - App files

APP_TARGET_ID = uid('target', 'Cog')
APP_PRODUCT_ID = uid('product', 'Cog')
APP_FOLDER = uid('sync', 'App')
# The app's sources: the folder iOS/App, which Xcode keeps in step itself.
add('PBXFileSystemSynchronizedRootGroup', APP_FOLDER, 'App', {
	'isa': 'PBXFileSystemSynchronizedRootGroup', 'path': 'App', 'sourceTree': '<group>'})
add('PBXFileReference', APP_PRODUCT_ID, 'Cog.app', {
	'isa': 'PBXFileReference', 'explicitFileType': 'wrapper.application', 'includeInIndex': '0',
	'path': 'Cog.app', 'sourceTree': 'BUILT_PRODUCTS_DIR'})
APP_INFO = uid('appfile', 'CogApp-Info.plist')
add('PBXFileReference', APP_INFO, 'CogApp-Info.plist', {
	'isa': 'PBXFileReference', 'lastKnownFileType': 'text.plist.xml', 'path': 'CogApp-Info.plist', 'sourceTree': '<group>'})
app_children = [Ref(APP_FOLDER, 'App'), Ref(APP_INFO, 'CogApp-Info.plist')]
app_resources = []
app_sources = []
for source in APP_SHARED_SOURCES:
	name = Path(source).name
	file_id = uid('appsource', source)
	add('PBXFileReference', file_id, name, {
		'isa': 'PBXFileReference', 'lastKnownFileType': 'sourcecode.swift', 'name': name, 'path': rel(source), 'sourceTree': 'SOURCE_ROOT'})
	app_children.append(Ref(file_id, name))
	build_id = uid('appsourcebuild', source)
	add('PBXBuildFile', build_id, f'{name} in Sources', {'isa': 'PBXBuildFile', 'fileRef': Ref(file_id, name)})
	app_sources.append(Ref(build_id, f'{name} in Sources'))
for resource in APP_RESOURCES + [APP_ICON]:
	file_id = uid('appresource', resource)
	kind = 'folder.iconcomposer.icon' if resource.endswith('.icon') else 'file'
	add('PBXFileReference', file_id, resource, {
		'isa': 'PBXFileReference', 'lastKnownFileType': kind, 'name': resource, 'path': rel(resource), 'sourceTree': 'SOURCE_ROOT'})
	app_children.append(Ref(file_id, resource))
	build_id = uid('appresourcebuild', resource)
	add('PBXBuildFile', build_id, f'{resource} in Resources', {'isa': 'PBXBuildFile', 'fileRef': Ref(file_id, resource)})
	app_resources.append(Ref(build_id, f'{resource} in Resources'))

# Linked: what the app's code imports. Embedded: every framework they need.
app_links = []
for name, ref in [('CogAudio', framework_refs['CogAudio']), ('CogPlaylist', framework_refs['CogPlaylist'])]:
	build_id = uid('applink', name)
	add('PBXBuildFile', build_id, f'{name}.framework in Frameworks', {'isa': 'PBXBuildFile', 'fileRef': Ref(ref, f'{name}.framework')})
	app_links.append(Ref(build_id, f'{name}.framework in Frameworks'))
build_id = uid('applink', 'CogPlugins')
add('PBXBuildFile', build_id, 'CogPlugins.framework in Frameworks', {'isa': 'PBXBuildFile', 'fileRef': Ref(PRODUCT_ID, 'CogPlugins.framework')})
app_links.append(Ref(build_id, 'CogPlugins.framework in Frameworks'))
app_embeds = []
for name, ref in sorted(list(framework_refs.items()) + [('CogPlugins', PRODUCT_ID)]):
	build_id = uid('appembed', name)
	add('PBXBuildFile', build_id, f'{name}.framework in Embed Frameworks', {
		'isa': 'PBXBuildFile', 'fileRef': Ref(ref, f'{name}.framework'),
		'settings': {'ATTRIBUTES': ['CodeSignOnCopy', 'RemoveHeadersOnCopy']}})
	app_embeds.append(Ref(build_id, f'{name}.framework in Embed Frameworks'))

groups = [
	('Plugins', plugin_groups, None),
	('CogPlugins', support_children, None),
	('CogPluginsTests', test_children, None),
	('Projects', subproject_children, None),
	('Frameworks', framework_children, None),
	('Cog', app_children, None),
	('Products', [Ref(APP_PRODUCT_ID, 'Cog.app'), Ref(PRODUCT_ID, 'CogPlugins.framework'), Ref(TEST_PRODUCT_ID, 'CogPluginsTests.xctest')], None),
]
main_children = []
for name, children, _ in groups:
	group_id = uid('maingroup', name)
	add('PBXGroup', group_id, name, {'isa': 'PBXGroup', 'children': children, 'name': name, 'sourceTree': '<group>'})
	main_children.append(Ref(group_id, name))
MAIN_GROUP = uid('maingroup', '')
add('PBXGroup', MAIN_GROUP, '', {'isa': 'PBXGroup', 'children': main_children, 'sourceTree': '<group>'})
PRODUCTS_GROUP = Ref(uid('maingroup', 'Products'), 'Products')

# MARK: - Phases

def phase(isa, name, target, files):
	ident = uid('phase', target, name)
	add(isa, ident, name, {'isa': isa, 'buildActionMask': '2147483647', 'files': files,
	                       'runOnlyForDeploymentPostprocessing': '0'})
	return Ref(ident, name)


plugin_phases = [
	phase('PBXSourcesBuildPhase', 'Sources', 'CogPlugins', plugin_sources),
	phase('PBXFrameworksBuildPhase', 'Frameworks', 'CogPlugins', plugin_links),
	phase('PBXResourcesBuildPhase', 'Resources', 'CogPlugins', plugin_resources),
]
test_phases = [
	phase('PBXSourcesBuildPhase', 'Sources', 'CogPluginsTests', test_sources),
	phase('PBXFrameworksBuildPhase', 'Frameworks', 'CogPluginsTests', test_links),
]

# MARK: - Configurations

def configurations(owner, common, debug, release, base=None):
	ids = []
	for name, extra in [('Debug', debug), ('Release', release)]:
		ident = uid('config', owner, name)
		body = {'isa': 'XCBuildConfiguration'}
		if base:
			body['baseConfigurationReference'] = base
		body['buildSettings'] = dict(sorted({**common, **extra}.items()))
		body['name'] = name
		add('XCBuildConfiguration', ident, name, body)
		ids.append(Ref(ident, name))
	list_id = uid('configlist', owner)
	add('XCConfigurationList', list_id, f'Build configuration list for {owner}', {
		'isa': 'XCConfigurationList', 'buildConfigurations': ids,
		'defaultConfigurationIsVisible': '0', 'defaultConfigurationName': 'Release'})
	return Ref(list_id, f'Build configuration list for {owner}')


project_settings = {
	'ALWAYS_SEARCH_USER_PATHS': 'NO',
	'CLANG_CXX_LANGUAGE_STANDARD': 'gnu++20',
	'CLANG_ENABLE_MODULES': 'YES',
	'CLANG_ENABLE_OBJC_ARC': 'YES',
	'GCC_C_LANGUAGE_STANDARD': 'gnu17',
	'IPHONEOS_DEPLOYMENT_TARGET': IOS_DEPLOYMENT_TARGET,
	'OTHER_CFLAGS': '-Wframe-larger-than=4000',
	'OTHER_CPLUSPLUSFLAGS': '-Wframe-larger-than=16000',
	'SDKROOT': 'iphoneos',
	'SUPPORTED_PLATFORMS': 'iphoneos iphonesimulator',
	'SUPPORTS_MACCATALYST': 'NO',
	'SWIFT_VERSION': '5.0',
	'TARGETED_DEVICE_FAMILY': '1,2',
}
project_config = configurations('PBXProject "CogPlugins"', project_settings,
                                {'DEBUG_INFORMATION_FORMAT': 'dwarf', 'ENABLE_TESTABILITY': 'YES', 'GCC_OPTIMIZATION_LEVEL': '0',
                                 'GCC_PREPROCESSOR_DEFINITIONS': ['DEBUG=1', '$(inherited)'], 'ONLY_ACTIVE_ARCH': 'YES',
                                 'SWIFT_ACTIVE_COMPILATION_CONDITIONS': 'DEBUG', 'SWIFT_OPTIMIZATION_LEVEL': '-Onone'},
                                {'DEBUG_INFORMATION_FORMAT': 'dwarf-with-dsym', 'SWIFT_COMPILATION_MODE': 'wholemodule'},
                                base=XCCONFIG)
plugin_config = configurations('PBXNativeTarget "CogPlugins"', {
	'CODE_SIGN_STYLE': 'Automatic',
	'DEFINES_MODULE': 'NO',
	'DYLIB_INSTALL_NAME_BASE': '@rpath',
	'GCC_PRECOMPILE_PREFIX_HEADER': 'YES',
	'GCC_PREFIX_HEADER': 'CogPlugins/CogPlugins_Prefix.pch',
	'GENERATE_INFOPLIST_FILE': 'YES',
	'HEADER_SEARCH_PATHS': ['$(inherited)'] + [rel(p) for p in HEADER_SEARCH_PATHS +
	                                            sorted({h for plugin in PLUGINS for h in plugin.get('headers', [])})],
	'INSTALL_PATH': '$(LOCAL_LIBRARY_DIR)/Frameworks',
	'OTHER_LDFLAGS': ['$(inherited)', '-lc++'] + sorted({f for plugin in PLUGINS for f in plugin.get('ldflags', [])}),
	'LD_RUNPATH_SEARCH_PATHS': ['$(inherited)', '@executable_path/Frameworks', '@loader_path/Frameworks'],
	'PRODUCT_BUNDLE_IDENTIFIER': 'org.cogx.CogPlugins',
	'PRODUCT_NAME': '$(TARGET_NAME)',
	'SKIP_INSTALL': 'YES',
}, {}, {})
test_config = configurations('PBXNativeTarget "CogPluginsTests"', {
	'CODE_SIGN_STYLE': 'Automatic',
	'GENERATE_INFOPLIST_FILE': 'YES',
	'LD_RUNPATH_SEARCH_PATHS': ['$(inherited)', '@executable_path/Frameworks', '@loader_path/Frameworks'],
	'PRODUCT_BUNDLE_IDENTIFIER': 'org.cogx.CogPluginsTests',
	'PRODUCT_NAME': '$(TARGET_NAME)',
}, {}, {})

# MARK: - Targets and project

test_dependency_proxy = uid('testproxy', 'CogPlugins')
add('PBXContainerItemProxy', test_dependency_proxy, 'PBXContainerItemProxy', {
	'isa': 'PBXContainerItemProxy', 'containerPortal': Ref(PROJECT_ID, 'Project object'),
	'proxyType': '1', 'remoteGlobalIDString': TARGET_ID, 'remoteInfo': 'CogPlugins'})
test_dependency = uid('testdependency', 'CogPlugins')
add('PBXTargetDependency', test_dependency, 'PBXTargetDependency', {
	'isa': 'PBXTargetDependency', 'target': Ref(TARGET_ID, 'CogPlugins'),
	'targetProxy': Ref(test_dependency_proxy, 'PBXContainerItemProxy')})

add('PBXNativeTarget', TARGET_ID, 'CogPlugins', {
	'isa': 'PBXNativeTarget', 'buildConfigurationList': plugin_config, 'buildPhases': plugin_phases,
	'buildRules': [], 'dependencies': plugin_dependencies, 'name': 'CogPlugins', 'productName': 'CogPlugins',
	'productReference': Ref(PRODUCT_ID, 'CogPlugins.framework'),
	'productType': 'com.apple.product-type.framework'})
add('PBXNativeTarget', TEST_TARGET_ID, 'CogPluginsTests', {
	'isa': 'PBXNativeTarget', 'buildConfigurationList': test_config, 'buildPhases': test_phases,
	'buildRules': [], 'dependencies': [Ref(test_dependency, 'PBXTargetDependency')] + test_dependencies, 'name': 'CogPluginsTests',
	'productName': 'CogPluginsTests', 'productReference': Ref(TEST_PRODUCT_ID, 'CogPluginsTests.xctest'),
	'productType': 'com.apple.product-type.bundle.unit-test'})
add('PBXProject', PROJECT_ID, 'Project object', {
	'isa': 'PBXProject',
	'attributes': {'BuildIndependentTargetsInParallel': 'YES', 'LastUpgradeCheck': '2700'},
	'buildConfigurationList': project_config, 'compatibilityVersion': 'Xcode 15.0', 'developmentRegion': 'en',
	'hasScannedForEncodings': '0', 'knownRegions': ['en', 'Base'], 'mainGroup': Ref(MAIN_GROUP, ''),
	'productRefGroup': PRODUCTS_GROUP, 'projectDirPath': '', 'projectReferences': project_references,
	'projectRoot': '', 'targets': [Ref(APP_TARGET_ID, 'Cog'), Ref(TARGET_ID, 'CogPlugins'), Ref(TEST_TARGET_ID, 'CogPluginsTests')]})

# MARK: - App target

embed_phase = uid('phase', 'Cog', 'Embed Frameworks')
add('PBXCopyFilesBuildPhase', embed_phase, 'Embed Frameworks', {
	'isa': 'PBXCopyFilesBuildPhase', 'buildActionMask': '2147483647', 'dstPath': '', 'dstSubfolderSpec': '10',
	'files': app_embeds, 'name': 'Embed Frameworks', 'runOnlyForDeploymentPostprocessing': '0'})
secrets_phase = uid('phase', 'Cog', 'Generate Swift secrets file')
add('PBXShellScriptBuildPhase', secrets_phase, 'Generate Swift secrets file', {
	'isa': 'PBXShellScriptBuildPhase', 'buildActionMask': '2147483647', 'files': [], 'inputFileListPaths': [],
	'inputPaths': ['$(SRCROOT)/../Scripts/generate-swift-secrets.sh'], 'name': 'Generate Swift secrets file',
	'outputFileListPaths': [], 'outputPaths': ['$(SRCROOT)/../Generated/Secrets.swift'],
	'runOnlyForDeploymentPostprocessing': '0', 'shellPath': '/bin/sh',
	# The script writes into $SRCROOT/Generated, the repository's.
	'shellScript': 'SRCROOT="${SRCROOT}/.." "${SCRIPT_INPUT_FILE_0}"\n', 'showEnvVarsInLog': '0'})
app_phases = [
	Ref(secrets_phase, 'Generate Swift secrets file'),
	phase('PBXSourcesBuildPhase', 'Sources', 'Cog', app_sources),
	phase('PBXFrameworksBuildPhase', 'Frameworks', 'Cog', app_links),
	phase('PBXResourcesBuildPhase', 'Resources', 'Cog', app_resources),
	Ref(embed_phase, 'Embed Frameworks'),
]
app_plugins_proxy = uid('appproxy', 'CogPlugins')
add('PBXContainerItemProxy', app_plugins_proxy, 'PBXContainerItemProxy', {
	'isa': 'PBXContainerItemProxy', 'containerPortal': Ref(PROJECT_ID, 'Project object'),
	'proxyType': '1', 'remoteGlobalIDString': TARGET_ID, 'remoteInfo': 'CogPlugins'})
app_plugins_dependency = uid('appdependency', 'CogPlugins')
add('PBXTargetDependency', app_plugins_dependency, 'PBXTargetDependency', {
	'isa': 'PBXTargetDependency', 'target': Ref(TARGET_ID, 'CogPlugins'), 'targetProxy': Ref(app_plugins_proxy, 'PBXContainerItemProxy')})
app_config = configurations('PBXNativeTarget "Cog"', {
	'ASSETCATALOG_COMPILER_APPICON_NAME': 'Play',
	'CODE_SIGN_STYLE': 'Automatic',
	'CURRENT_PROJECT_VERSION': '1',
	'GENERATE_INFOPLIST_FILE': 'YES',
	'INFOPLIST_FILE': 'CogApp-Info.plist',
	'INFOPLIST_KEY_CFBundleDisplayName': 'Cog',
	'INFOPLIST_KEY_LSApplicationCategoryType': 'public.app-category.music',
	'INFOPLIST_KEY_UIApplicationSceneManifest_Generation': 'YES',
	'INFOPLIST_KEY_UILaunchScreen_Generation': 'YES',
	'INFOPLIST_KEY_UISupportedInterfaceOrientations_iPad': 'UIInterfaceOrientationPortrait UIInterfaceOrientationPortraitUpsideDown UIInterfaceOrientationLandscapeLeft UIInterfaceOrientationLandscapeRight',
	'INFOPLIST_KEY_UISupportedInterfaceOrientations_iPhone': 'UIInterfaceOrientationPortrait UIInterfaceOrientationLandscapeLeft UIInterfaceOrientationLandscapeRight',
	'LD_RUNPATH_SEARCH_PATHS': ['$(inherited)', '@executable_path/Frameworks'],
	'MARKETING_VERSION': '0.1',
	'PRODUCT_BUNDLE_IDENTIFIER': 'co.losno.MobileCog',
	'PRODUCT_NAME': 'Cog',
	'SWIFT_EMIT_LOC_STRINGS': 'YES',
}, {}, {})
add('PBXNativeTarget', APP_TARGET_ID, 'Cog', {
	'isa': 'PBXNativeTarget', 'buildConfigurationList': app_config, 'buildPhases': app_phases,
	'buildRules': [], 'dependencies': [Ref(app_plugins_dependency, 'PBXTargetDependency')] + app_dependencies,
	'fileSystemSynchronizedGroups': [Ref(APP_FOLDER, 'App')],
	'name': 'Cog', 'productName': 'Cog', 'productReference': Ref(APP_PRODUCT_ID, 'Cog.app'),
	'productType': 'com.apple.product-type.application'})

# MARK: - Writing


def render(value, indent):
	if isinstance(value, list):
		inner = ''.join(f'{indent}\t{render(v, indent + chr(9))},\n' for v in value)
		return f'(\n{inner}{indent})'
	if isinstance(value, dict):
		inner = ''.join(f'{indent}\t{quote(k)} = {render(v, indent + chr(9))};\n' for k, v in value.items())
		return f'{{\n{inner}{indent}}}'
	return value if isinstance(value, Ref) else quote(value)


def render_inline(body):
	def value(v):
		if isinstance(v, dict):
			return render_inline(v)
		if isinstance(v, list):
			return '(' + ''.join(f'{value(item)}, ' for item in v) + ')'
		return v if isinstance(v, Ref) else quote(v)
	return '{' + ''.join(f'{quote(k)} = {value(v)}; ' for k, v in body.items()) + '}'


INLINE = {'PBXBuildFile', 'PBXFileReference'}
out = ['// !$*UTF8*$!', '{', '\tarchiveVersion = 1;', '\tclasses = {', '\t};', '\tobjectVersion = 77;', '\tobjects = {']
for section in sorted(objects):
	out.append('')
	out.append(f'/* Begin {section} section */')
	for ident, comment, body in sorted(objects[section]):
		label = f'{ident} /* {comment} */' if comment else ident
		if section in INLINE:
			out.append(f'\t\t{label} = {render_inline(body)};')
		else:
			out.append(f'\t\t{label} = {render(body, chr(9) * 2)};')
	out.append(f'/* End {section} section */')
out += ['\t};', f'\trootObject = {PROJECT_ID} /* Project object */;', '}', '']

PROJECT.mkdir(exist_ok=True)
(PROJECT / 'project.pbxproj').write_text('\n'.join(out))

# A shared scheme, so the tests run with the framework.
scheme_dir = PROJECT / 'xcshareddata' / 'xcschemes'
scheme_dir.mkdir(parents=True, exist_ok=True)
(scheme_dir / 'Cog.xcscheme').write_text(f'''<?xml version="1.0" encoding="UTF-8"?>
<Scheme LastUpgradeVersion = "2700" version = "1.7">
   <BuildAction parallelizeBuildables = "YES" buildImplicitDependencies = "YES">
      <BuildActionEntries>
         <BuildActionEntry buildForTesting = "YES" buildForRunning = "YES" buildForProfiling = "YES" buildForArchiving = "YES" buildForAnalyzing = "YES">
            <BuildableReference BuildableIdentifier = "primary" BlueprintIdentifier = "{APP_TARGET_ID}" BuildableName = "Cog.app" BlueprintName = "Cog" ReferencedContainer = "container:MobileCog.xcodeproj">
            </BuildableReference>
         </BuildActionEntry>
      </BuildActionEntries>
   </BuildAction>
   <TestAction buildConfiguration = "Debug" selectedDebuggerIdentifier = "Xcode.DebuggerFoundation.Debugger.LLDB" selectedLauncherIdentifier = "Xcode.DebuggerFoundation.Launcher.LLDB" shouldUseLaunchSchemeArgsEnv = "YES">
   </TestAction>
   <LaunchAction buildConfiguration = "Debug" selectedDebuggerIdentifier = "Xcode.DebuggerFoundation.Debugger.LLDB" selectedLauncherIdentifier = "Xcode.DebuggerFoundation.Launcher.LLDB" launchStyle = "0" useCustomWorkingDirectory = "NO" ignoresPersistentStateOnLaunch = "NO" debugDocumentVersioning = "YES" debugServiceExtension = "internal" allowLocationSimulation = "YES">
      <BuildableProductRunnable runnableDebuggingMode = "0">
         <BuildableReference BuildableIdentifier = "primary" BlueprintIdentifier = "{APP_TARGET_ID}" BuildableName = "Cog.app" BlueprintName = "Cog" ReferencedContainer = "container:MobileCog.xcodeproj">
         </BuildableReference>
      </BuildableProductRunnable>
   </LaunchAction>
   <ArchiveAction buildConfiguration = "Release" revealArchiveInOrganizer = "YES">
   </ArchiveAction>
</Scheme>
''')
(scheme_dir / 'CogPlugins.xcscheme').write_text(f'''<?xml version="1.0" encoding="UTF-8"?>
<Scheme LastUpgradeVersion = "2700" version = "1.7">
   <BuildAction parallelizeBuildables = "YES" buildImplicitDependencies = "YES">
      <BuildActionEntries>
         <BuildActionEntry buildForTesting = "YES" buildForRunning = "YES" buildForProfiling = "YES" buildForArchiving = "YES" buildForAnalyzing = "YES">
            <BuildableReference BuildableIdentifier = "primary" BlueprintIdentifier = "{TARGET_ID}" BuildableName = "CogPlugins.framework" BlueprintName = "CogPlugins" ReferencedContainer = "container:MobileCog.xcodeproj">
            </BuildableReference>
         </BuildActionEntry>
      </BuildActionEntries>
   </BuildAction>
   <TestAction buildConfiguration = "Debug" selectedDebuggerIdentifier = "Xcode.DebuggerFoundation.Debugger.LLDB" selectedLauncherIdentifier = "Xcode.DebuggerFoundation.Launcher.LLDB" shouldUseLaunchSchemeArgsEnv = "YES">
      <Testables>
         <TestableReference skipped = "NO">
            <BuildableReference BuildableIdentifier = "primary" BlueprintIdentifier = "{TEST_TARGET_ID}" BuildableName = "CogPluginsTests.xctest" BlueprintName = "CogPluginsTests" ReferencedContainer = "container:MobileCog.xcodeproj">
            </BuildableReference>
         </TestableReference>
      </Testables>
   </TestAction>
   <LaunchAction buildConfiguration = "Debug" selectedDebuggerIdentifier = "Xcode.DebuggerFoundation.Debugger.LLDB" selectedLauncherIdentifier = "Xcode.DebuggerFoundation.Launcher.LLDB" launchStyle = "0" useCustomWorkingDirectory = "NO" ignoresPersistentStateOnLaunch = "NO" debugDocumentVersioning = "YES" debugServiceExtension = "internal" allowLocationSimulation = "YES">
   </LaunchAction>
   <ArchiveAction buildConfiguration = "Release" revealArchiveInOrganizer = "YES">
   </ArchiveAction>
</Scheme>
''')
print(f'Wrote {PROJECT.relative_to(ROOT)} with {len(PLUGINS)} plugins, {len(plugin_sources)} source files')
