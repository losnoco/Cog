#!/usr/bin/env python3
"""Declares the files Cog's Quick Look extension previews.

The macOS app lists its document types by extension, in Info.plist.template,
but Quick Look matches files by type. This gives the extensions types the
way iOS/generate_document_types.py does for the iOS app, and writes:

- ImportedTypes.plist: the types imported for Cog, which
  Scripts/genversion.sh merges into the app's Info.plist.
- CogQuickLook/Info.plist: the types the extension previews. Those are the
  imported ones, besides playlists and video, and the system's types for
  audio that macOS itself does not preview.

Run this script again after changing the macOS app's document types.
"""

import importlib.util
import plistlib
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
IMPORTED_PLIST = ROOT / 'ImportedTypes.plist'
EXTENSION_PLIST = ROOT / 'CogQuickLook' / 'Info.plist'

spec = importlib.util.spec_from_file_location('document_types', ROOT / 'iOS' / 'generate_document_types.py')
document_types = importlib.util.module_from_spec(spec)
spec.loader.exec_module(document_types)

# Imported types the extension leaves alone: playlists, links and video.
NOT_PREVIEWED = {'CUE Sheet File', 'APL Link File', 'Matroska Video File'}

# The system's types that macOS declares but does not play in Quick Look.
# The rest of SYSTEM_TYPES (MP3, AAC, WAV, AIFF, FLAC…) macOS previews itself.
SYSTEM_PREVIEWED = [
	'public.midi-audio',
	'org.xiph.ogg-audio',
	'com.microsoft.windows-media-wma',
	'com.microsoft.advanced-systems-format',
	'com.real.realaudio',
	'com.real.realmedia',
]

groups = document_types.document_groups()
imported = document_types.imported_types(groups, document_types.extension_owners(groups))
IMPORTED_PLIST.write_bytes(plistlib.dumps({'UTImportedTypeDeclarations': imported}, sort_keys=True))

previewed = [t['UTTypeIdentifier'] for t in imported if t['UTTypeDescription'] not in NOT_PREVIEWED]
previewed += SYSTEM_PREVIEWED

info = plistlib.loads(EXTENSION_PLIST.read_bytes())
info['NSExtension']['NSExtensionAttributes']['QLSupportedContentTypes'] = previewed
EXTENSION_PLIST.write_bytes(plistlib.dumps(info, sort_keys=True))
print(f'{len(imported)} imported types, {len(previewed)} previewed')
