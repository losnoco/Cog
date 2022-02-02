Cog [![buildable](https://github.com/losnoco/Cog/actions/workflows/debug.yml/badge.svg)](https://github.com/losnoco/Cog/actions/workflows/debug.yml)
===

Cog is authored by Vincent Spader. It is released under the GPL. See COPYING for details.

The libraries folder contains various decoding and tagging libraries, which i have created Xcode projects for, and possibly modified to make compile on OS X. The various libraries are under each of their own licenses/copyrights.

All Cog code is copyrighted by me, and is licensed under the GPL. Cog contains bits of other code from third parties that are under their own licenses/copyright.
    
If you would like the photoshop sources for the various icons and graphics, please send me an email, and I will be happy to get them to you.

Share and enjoy.
--Vincent Spader (vspader@users.sf.net)


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