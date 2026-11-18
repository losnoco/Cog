# Tag editor plan

Writing tags back to files from the playlist, in parallel with XPCog's editor
(`../xpcog/docs/TAGEDITOR.md` and commits e75e39a..26f180b), adapted to how Cog
already stores tags. It also covers reading and writing chapters through TagLib.

## What Cog has today

- `CogMetadataWriter` in `Audio/Plugin.h` (`+putMetadataInURL:tagData:`) has one
  conformer, `Plugins/TagLib/TagLibMetadataWriter.m` (2008). That writer edits a
  copy of the `PropertyMap` and never calls `setProperties`, so it changes
  nothing. `PluginController` never registers writers, its
  `-putMetadataInURL:` returns 0, and `AudioMetadataWriter` passes through to
  that. `TagEditor/TagEditorController.m` is commented out, and so is
  `showTagEditor:` in `PlaylistController.m`. Treat all of it as greenfield,
  and delete what the new code replaces.
- Tags live in `PlaylistEntry.metadataBlob`: lowercased keys, each holding an
  array of strings, as `-setMetadata:` stores whatever the readers merged. The
  named accessors read alias chains (`albumartist`/`album artist`/`album_artist`,
  `date`/`recording_date`/`year`, `unsyncedlyrics`/`unsynced lyrics`/`lyrics`).
  Art goes through `albumArtInternal`/`artHash`. Unlike XPCog, there are no
  promoted columns to fold back, so XPCog's `fullTagMap()` step (its phase 3)
  has no counterpart here.
- Several readers merge per file. TagLib is one of them (it skips mp4/m4a,
  which FFmpeg reads), and decoders add their own tags. Folder art is injected
  into `albumArt` (`Cached_Folder_Art` in `PluginController.mm`). So the
  playlist blob is **not** a faithful picture of the file's tags.
- TagLib is 2.2.1 (`ThirdParty/Frameworks/taglib.txt`), prebuilt with no
  patches. It exposes chapters only per format: ID3v2 `CHAP`/`CTOC` frames
  (`chapterframe.h`, `tableofcontentsframe.h`), and Matroska
  `Matroska::File::chapters()`. **It has no MP4 chapter API**, and it has no
  generic `CHAPTERS` complex property. `TagLibMetadataReader.m:56` tests for
  one, so that branch never runs.
- Playback already gets chapters from FFmpeg: `FFMPEGContainer` lists
  `file#N` per `AVChapter`, and the decoder takes each chapter's metadata.

## Decisions carried over from XPCog

- **Probe, don't guess.** Save support is a property of the file TagLib opens,
  not of its extension. A probe drives what the editor offers; there are no
  format checks in the UI.
- **Capabilities:** `writable`, `arbitraryKeys` (ID3v2 folds unknown names
  into `TXXX`, MP4 into `----` atoms), `multiValue`, `pictures`, and new here,
  `chapters`.
- **Strip companions:** MPEG, WAV and DSDIFF save with `StripOthers`, so a
  stale ID3v1 or APE tag doesn't read back over the new one. Merges erase keys
  case-insensitively (TagLib uppercases on read).
- **Art:** an empty `PICTURE` list removes art. A FLAC picture needs a real
  mime type (sniff the magic bytes; an empty mime means a link). Folder art is
  never written into files.
- **Modes:** a single file is a whole-map replace. A batch merges only the
  fields that were touched, and also clears those fields' losing alias
  spellings (XPCog 280796c). Free-form rows and the cover are single-file only.
- **No undo** for a save. Rewriting the old tags is wrong once another program
  has touched the file.
- **After saving, reload from the file;** don't trust the edit.
- **Reject cue fragments:** writing "tags" to `album.cue#3` would mean editing
  the cue sheet. Embedded cue tracks (`file.flac#03` with a `cuesheet`) are
  rejected too.

## Where Cog differs

1. **The editor's document is the file, read by the writer, not the blob.**
   The writer plugin answers `tagsForURL:` from TagLib alone: no other readers,
   no folder art, no audio properties. Because of that, an embedded cover and
   a folder cover can be told apart, and nothing a decoder made up gets
   written back. It names tags the way the blob does: every tag under its
   raw, lowercased name (with the reader's `track`/`disc`/`soundcheck`
   mapping), so the editor's free-form rows are the same raw fields the blob
   and the Info Inspector show for the entry.
2. **Crash safety is ours to do.** TagLib 2.x rewrites the file in place. Save
   a copy in the same folder (`NSFileManager` copy, TagLib on the copy), then
   `replaceItemAtURL:withItemAtURL:…`. A failed or interrupted save leaves the
   original untouched. The folder is already inside the bookmark's scope.
3. **Sandbox:** every access goes through `SandboxBroker`
   `beginFolderAccess:`/`endFolderAccess:`. Bookmarks are made without
   `…SecurityScopeAllowOnlyReadAccess`, so they allow writing. Test the App
   Store build itself, not only a local unsandboxed one.
4. **Caches:** `PluginController`'s tag cache is keyed by URL and keeps an
   entry until it has gone 10 s unused, so a reload right after a save gets
   the old tags. Add `-forgetMetadataForURL:` (both cue variants, and the
   file's `#N` subsongs).
5. **Reload must not merge.** `-setMetadata:` merges into the existing blob,
   so a deleted tag (and removed art) survives "Reload Info". Clear the blob
   and the art hash before the reload after a save, and fix `reloadTags:` the
   same way, since it has the same bug.
6. **iOS:** the TagLib plugin is compiled into `CogPlugins.framework`, so the
   writer must build there too, even though the editor UI is macOS only for
   now. Build both platforms (CLAUDE.md).

## Chapters

Reading and writing both go through TagLib's per-format APIs. They live in the
writer plugin, inside `TagLibMetadataWriter`'s source or a small shared
`TagLibChapters.mm`.

**Model.** Each chapter is `{start, end, title, id}`, with times in
milliseconds, ordered by start. `id` is the ID3v2 element ID or the Matroska
UID, so an unchanged chapter keeps its identity.

**Reading** (`tagsForURL:` returns a `chapters` array, and the probe sets
`chapters`):
- ID3v2 (MPEG, WAV, AIFF, and DSF if TagLib gives it an ID3v2 tag): follow the
  top-level `CTOC`'s child order when there is one, otherwise sort the `CHAP`
  frames by start. The title is the `TIT2` subframe.
- Matroska: the first (default) edition's chapters, with the display string in
  the file's language or the first one.

**Writing:**
- ID3v2: remove every `CHAP` and `CTOC`, then add one top-level, ordered `CTOC`
  (`ctoc` element ID) and one `CHAP` per chapter (`chp0`… or the kept IDs),
  each with a `TIT2` subframe. Set start/end offsets to `0xFFFFFFFF`, which
  means "use the times".
- Matroska: replace the chapters with one edition rebuilt from the list. Keep
  UIDs where they're known.
- Each chapter's end is the next one's start, and the last one's end is the
  file's length. The editor edits titles and starts, and can add and remove
  chapters.

**Chapter entries in the playlist** (`file.mp3#N` from `FFMPEGContainer`):
- They become editable as their file. The editor opens on that file with
  chapter N highlighted, and the "Title" field edits chapter N's title.
- What tells them apart from cue tracks: the URL isn't a `.cue`, the entry has
  no `cuesheet`, and the probe reports chapters.
- **To verify:** FFmpeg's chapter index matches TagLib's start-sorted order.
  FFmpeg sorts ID3v2 chapters by start (`ff_id3v2_parse_chapters`). Check this
  against a file whose `CTOC` order differs.
- After saving, reload the file's `#N` entries too. Reloading can't add or
  remove playlist rows when the number of chapters changes, so either say so
  in the editor or re-expand the file in place. That is open; see below.

**Fix the reader's dead check.** `TagLibMetadataReader` should detect chapters
through the same helper, so a chaptered file's `TITLE` is read as `album` the
way that code intends.

**MP4/M4B, the main audiobook format:** TagLib 2.2.1 can neither read nor
write its chapters (Nero `chpl` or QuickTime chapter tracks). The probe
reports `chapters = NO`, the editor lists them read-only from FFmpeg or not at
all, and saving tags leaves the chapter atoms alone. This needs a test.
Writing MP4 chapters ourselves is out of scope.

## Phases

1. **Plugin contract** (`Audio/Plugin.h`, `PluginController`, `CogPluginMulti`)
   ```objc
   @protocol CogMetadataWriter <NSObject>
   + (NSArray *)fileTypes;
   + (float)priority;
   + (NSDictionary *)capabilitiesForURL:(NSURL *)url;   // the probe; @{} = not writable
   + (NSDictionary *)tagsForURL:(NSURL *)url;           // the file's own tags, art bytes, chapters
   + (BOOL)writeTags:(NSDictionary *)tags toURL:(NSURL *)url
             replace:(BOOL)replace error:(NSError **)error;
   @end
   ```
   Register writers by extension the way readers are. Writing walks claimants
   by descending priority, and the first one whose probe says writable wins
   (several writers on one file would fight). The registry rejects cue
   fragments before any writer runs. Delete `putMetadataInURL:` and
   `AudioMetadataWriter`.
2. **TagLib writer** (`Plugins/TagLib`): the probe, `tagsForURL:`, write with
   save-to-copy-and-replace, `StripOthers`, art, and chapters. Use the key
   mapping the reader uses (`track`→`TRACKNUMBER`, `disc`→`DISCNUMBER`,
   `soundcheck`→`ITUNNORM`), and pass TagLib errors back through `NSError`.
   Fix the reader's chapter check.
3. **App plumbing** (`PlaylistController`/`PlaylistLoader`): an `editTags`
   path that groups the selected entries by file, so one file is saved once
   however many rows show it; skips unwritable files; saves on a background
   queue; forgets the cache; clears the blob and reloads; and reports
   "Saved tags for N files" or the failures.
4. **Editor UI** (`TagEditor/`, SwiftUI in an `NSWindowController`, like
   `InfoInspector`/`SignalInspector`). The model is a plain Swift
   `TagEditorModel`, a port of XPCog's (`uicore/src/TagEditorModel.hpp`).
   - Fields: title, artist, album artist, album, genre, composer, date, track,
     disc, comment, lyrics. A batch shows "values differ" where the files
     disagree.
   - Free-form rows (single file): any name, and values a format can hold
     several of, with notes where the format folds or joins them.
   - Cover (single file, when the format holds pictures): keep, remove, or
     replace from a file. Shown with a note when the art comes from the folder.
   - Chapters (single file, when the probe reports them): a table of start and
     title, with add and remove.
   - Save is enabled only when something changed.
   - Menu items: "Edit Tags…" in the playlist's context menu and the Edit menu
     (`Base.lproj/MainMenu.xib`). They are enabled only when every selected
     entry is a local file (or chapter) whose probe says writable. Strings go
     in `Localizable.xcstrings`, in English and Spanish.
5. **Tests**
   - Writer round-trips in `iOS/CogPluginsTests`, which loads the real
     plugins: FLAC, Ogg Vorbis/Opus, MP3 (with a `TXXX` fold), MP4, WAV,
     Matroska. Cover added and removed, a custom key, a two-value key, a
     merge clearing alias spellings, ID3v2 and Matroska chapters
     (write → read → compare), and an MP4 file's chapters surviving a tag
     save.
   - The read-back goes through the **whole merged reader path**
     (`PlaylistEntryInfo`), not only TagLib, so a decoder's own reader (FLAC,
     Vorbis, FFmpeg) is shown to see the change.
   - Manual: the App Store build, sandboxed, on a bookmarked folder and on a
     dropped file; a chapter entry from an MP3; a cue track's menu item
     disabled.

## Out of scope for v1

ReplayGain writing; `.lrc` writing; MP4 chapter writing; archive members, HTTP
streams and cue tracks; the iOS editor UI; AppleScript and Remote Control tag
writes.

## Decided

- After a chapter add or remove, the file's `#N` rows are rebuilt in place to
  follow the new chapters, as a prompted option or a setting.
- ID3v2 saves as 2.4, TagLib's default, with no setting for now.
- The phases land and are committed one at a time, with the UI last.
