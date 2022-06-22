Cog [![buildable](https://github.com/losnoco/Cog/actions/workflows/debug.yml/badge.svg)](https://github.com/losnoco/Cog/actions/workflows/debug.yml)
===

Cog is authored by Vincent Spader. It is released under the GPL. See COPYING for details.

The libraries folder contains various decoding and tagging libraries, which i have created Xcode projects for, and possibly modified to make compile on OS X. The various libraries are under each of their own licenses/copyrights.

All Cog code is copyrighted by me, and is licensed under the GPL. Cog contains bits of other code from third parties that are under their own licenses/copyright.
    
If you would like the photoshop sources for the various icons and graphics, please send me an email, and I will be happy to get them to you.

Share and enjoy.
--Vincent Spader (vspader@users.sf.net)


ADDENDUM - 2022-06-21

Cog now uses App Sandbox. This requires permission paths to be granted in the
settings, under the General tab. Right clicking the listing allows one to
manipulate the list, including suggesting additions based on the current
playlist. The suggestion dialog will pop up every path and permutation that
isn't covered by an existing valid sandbox token. It is suggested to check
only the paths that are most appropriate to cover most files you will ever
play with Cog. It is not necessary to add either your default Music folder,
your default Downloads folder, or your default Movies folder.


ADDENDUM - 2022-06-22

This branch is the App Store version. The only real difference between it and
the sparkle branch is that two commits which removed the Sparkle framework
were reverted in that branch. This branch contains an update to the README
and an extra empty commmit so that the version numbers sync up between the
two.

The App Store page for Cog is [here](https://apps.apple.com/us/app/cog-kode54/id1630499622), when it finally goes live.


ADDENDUM - 2013-09-30

I have forked this player to continue maintaining it for others to use, as its
original author appears to have left it by the wayside. I will try to continue
updating as I find more things that need fixing, or if I find new features to
add which seem like they would be useful to me or others.

Up to date binaries will be available at the following link:

https://cog.losno.co/

--Christopher Snowhill (chris@kode54.net)

ADDENDUM - 2018-06-21

You will need to run the following to retrieve all the source code:

```
git submodule update --init --recursive
```

Setup your `DEVELOPMENT_TEAM` like described in [Xcode-config/Shared.xcconfig](https://github.com/losnoco/Cog/blob/main/Xcode-config/Shared.xcconfig) to build the project.

(Add 2022-05-22) Also, be sure to configure the hooks path, so you won't accidentally commit your team ID to a project file:

```
git config core.hooksPath .githooks
```

# Screenshots

## Main window and Info Inspector

![Main window](https://github.com/losnoco/Cog/blob/main/.github/images/MainWindow.png)

## Mini window

![Mini window](https://github.com/losnoco/Cog/blob/main/.github/images/MiniWindow.png)


ADDENDUM - 2022-01-25

I've added an HRIR convolver, based on the HeSuVi HRIR format. A preset is
bundled with the app, but external presets may be selected, in either RIFF
WAVE, or Wavpack format, 7 or 14 channel, 32 bit floating point. A bad or
missing preset will fall back on the built-in preset. The settings dialog
won't let you assign a bad preset, either.

The channel layout of the HeSuVi HRIR is as follows:

![HeSuVi HRIR channel layout](https://github.com/losnoco/Cog/blob/main/.github/images/HeSuVi-channel-map.png)

I altered this image from the original on the HeSuVi HRIR developer readme,
as their image incorrectly swapped the left/right output order for the right
input channels, which are actually supposed to be right output first, then
left output. Otherwise, if observed as the original image, all output comes
from either the center or the left side.

The layout for 7 channel no-reverb presets is the same, except it only
includes the first 7 channels of the layout, and the opposite speakers
simply mirror their respective input channels, and the center input is
used for both left and right outputs.


[Here are some preset HRIR files](https://cogcdn.cog.losno.co/HeSuVi-hrir-basic.7z).

The original HeSuVi project is located [here](https://sourceforge.net/projects/hesuvi/).


ADDENDUM - 2022-02-01

I've replaced the resampler with libsoxr, with no quality control settings exposed.


ADDENDUM - 2022-03-03

I've replaced the resampler yet again. This time, with [R8Brain](https://github.com/avaneev/r8brain-free-src), using the 24 bit preset, since the player doesn't go over the depth of 24 bit integer or 32 bit floating point, and outputs 32 bit floating point. It still processes the resampler in 64 bits, though.


ADDENDUM - 2022-03-04

It has come to my attention that my recent commits may have assumed that Aleksey Vaneev, and possibly any other Russian or Belarusian people are in support of the war in Ukraine. I did not intend to make that assumption. I know that a lot of people are either against it, have family and/or friends affected by it, or in some unfortunate cases, have the wool pulled over their eyes by the local propaganda machine. The only thing I knew at all, was comments to the effect of him being anti-vaccines, which is a bit of a sore subject with me. I do not know anyone personally who has died from COVID-19, but I do support using the greatest caution to avoid it, including vaccines, and masks, and social distancing. I will maintain my fork of the library in case anything happens to upstream, and keep an open mind for now, I hope. Hope this long winded mess finds my users well, and doesn't further piss off anyone.
