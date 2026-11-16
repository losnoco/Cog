Presets and textures bundled with the projectM pane (macOS). The folder is copied into the app's Resources as
`projectM`.

- `presets/`: 250 of the 9,795 `.milk` presets in "Cream of the Crop", curated and sorted by ISOSCELES, from
  https://github.com/projectM-visualizer/presets-cream-of-the-crop at 0180df21f5e0bd39b9060cc5de420ed2f1f9e509.
  Up to 25 per theme folder, spread evenly through each folder's sorted list, skipping presets that need a texture
  the texture pack lacks. See `presets/LICENSE.md` for their status.
- `textures/`: projectM's whole texture pack for those presets, from
  https://github.com/projectM-visualizer/presets-milkdrop-texture-pack at 6368812f27bc747b517218fbf89d21d59afce4d9.

The full pack is downloaded on request from Settings > Appearance > projectM: `ProjectMPresetPack.swift` fetches
https://cogcdn.cog.losno.co/cream-of-the-crop-lzma2.7z (the same commit, solid LZMA2 at 7-Zip's highest settings),
checks its SHA-256, and unpacks it with File_Extractor into Application Support/Cog/projectM/Cream of the Crop. A new
pack needs a new file on the CDN and a new hash in that file.
