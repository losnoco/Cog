#!/usr/bin/env python3
"""Declares the files the iOS app opens, in CogApp-Info.plist.

The macOS app lists its document types by extension, in Info.plist.template.
iOS matches documents by type instead, so each extension is given one:
the system's own, where iOS declares one for audio or playlists, or a type
imported here, named after the macOS document type that claims it. Run this
script again after changing the macOS app's document types.
"""

import plistlib
import re
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parent
TEMPLATE = ROOT / 'Info.plist.template'
INFO_PLIST = HERE / 'CogApp-Info.plist'

# The types iOS declares for these extensions (MobileCoreTypes.bundle).
SYSTEM_TYPES = {
	'aac': 'public.aac-audio', 'adts': 'public.aac-audio',
	'aax': 'com.audible.aax-audiobook',
	'ac3': 'public.ac3-audio',
	'aif': 'public.aiff-audio', 'aiff': 'public.aiff-audio', 'aifc': 'public.aifc-audio',
	'amr': 'org.3gpp.adaptive-multi-rate-audio',
	'asf': 'com.microsoft.advanced-systems-format',
	'au': 'public.au-audio', 'snd': 'public.au-audio',
	'caf': 'com.apple.coreaudio-format',
	'flac': 'org.xiph.flac',
	'kar': 'public.midi-audio', 'mid': 'public.midi-audio', 'midi': 'public.midi-audio',
	'm3u': 'public.m3u-playlist', 'm3u8': 'public.m3u-playlist',
	'm4a': 'com.apple.m4a-audio', 'm4b': 'com.apple.protected-mpeg-4-audio-b',
	'm4r': 'com.apple.mpeg-4-ringtone', 'mp4': 'public.mpeg-4',
	'mp2': 'public.mp2', 'mp3': 'public.mp3',
	'mpeg': 'public.mpeg', 'm15': 'public.mpeg',
	'oga': 'org.xiph.ogg-audio', 'ogg': 'org.xiph.ogg-audio', 'opus': 'org.xiph.ogg-audio',
	'pls': 'public.pls-playlist',
	'ra': 'com.real.realaudio', 'rm': 'com.real.realmedia',
	'ulw': 'public.ulaw-audio',
	'w64': 'com.sony.wave64',
	'wav': 'com.microsoft.waveform-audio', 'wave': 'com.microsoft.waveform-audio',
	'wax': 'com.microsoft.windows-media-wax',
	'webm': 'org.webmproject.webm',
	'wma': 'com.microsoft.windows-media-wma',
	'wvx': 'com.microsoft.windows-media-wvx',
}

# Extensions iOS gives to other kinds of files (source code, images,
# documents). The macOS app opens these too, but on iOS claiming their type
# would offer Cog for every such file, so they are left out.
NOT_AUDIO = {'aaf', 'abc', 'ads', 'ai', 'atx', 'dsym', 'hxx', 'l', 'm', 'psb', 'r', 'sdd', 'sdp', 'svg', 'sxd'}

# What the imported types are, by the macOS document type; audio otherwise.
CONFORMS_TO = {
	'CUE Sheet File': ['public.plain-text', 'public.playlist'],
	'APL Link File': ['public.data', 'public.content'],
	'Matroska Video File': ['public.movie'],
}


def identifier(name):
	"""org.cogx.cog.vgmstream, from "VGMStream Files"."""
	slug = re.sub(r'\b(files?)\b', '', name.lower().replace("'", ''))
	slug = re.sub(r'[^a-z0-9]+', '-', slug).strip('-')
	return f'org.cogx.cog.{slug}'


template = plistlib.loads(TEMPLATE.read_bytes())

# The macOS document types, merged by name (some are listed twice), each
# with its extensions in order; folders are not documents on iOS.
groups = {}
for doc in template['CFBundleDocumentTypes']:
	extensions = [e.lower() for e in doc.get('CFBundleTypeExtensions', []) if e != '*']
	if extensions:
		group = groups.setdefault(doc['CFBundleTypeName'], [])
		group.extend(e for e in extensions if e not in group)

# An extension without a system type belongs to the most specific group
# that claims it, the one with the fewest extensions: .dsf is a DSD stream
# rather than one of vgmstream's many formats.
owner = {}
for name, extensions in sorted(groups.items(), key=lambda item: len(item[1])):
	for extension in extensions:
		if extension not in SYSTEM_TYPES and extension not in NOT_AUDIO:
			owner.setdefault(extension, name)

imported = []
document_types = []
for name, extensions in groups.items():
	own = [e for e in extensions if owner.get(e) == name]
	if own:
		imported.append({
			'UTTypeIdentifier': identifier(name),
			'UTTypeDescription': name,
			'UTTypeConformsTo': CONFORMS_TO.get(name, ['public.audio']),
			'UTTypeTagSpecification': {'public.filename-extension': own},
		})
	types = []
	for extension in extensions:
		if extension in NOT_AUDIO:
			continue
		uti = SYSTEM_TYPES.get(extension) or identifier(owner[extension])
		if uti not in types:
			types.append(uti)
	document_types.append({
		'CFBundleTypeName': name,
		'CFBundleTypeRole': 'Viewer',
		'LSHandlerRank': 'Default',
		'LSItemContentTypes': types,
	})

info = plistlib.loads(INFO_PLIST.read_bytes())
info['CFBundleDocumentTypes'] = document_types
info['UTImportedTypeDeclarations'] = imported
INFO_PLIST.write_bytes(plistlib.dumps(info, sort_keys=True))
print(f'{len(document_types)} document types, {len(imported)} imported types, '
      f'{len(owner)} extensions imported, {len(NOT_AUDIO)} left out')
