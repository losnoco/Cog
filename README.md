Cog [![buildable](https://github.com/losnoco/Cog/actions/workflows/debug.yml/badge.svg)](https://github.com/losnoco/Cog/actions/workflows/debug.yml)
===

Cog uses [Lokalise](https://lokalise.com/) for translation. If you want to contribute translations to Cog, send an email to kevin@kddlb.cl.

![Lokalise logo](https://github.com/losnoco/Cog/blob/main/.github/images/Lokalise_logo_colour_black_text.svg | width=200)

---

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

(Add 2022-06-24) Also, you need to unpack the static and dynamic library dependencies, and update them any time the library path changes:

```
cd ThirdParty
tar xvf libraries.tar.xz
```

# Screenshots

## Main window and Info Inspector

![Main window](https://github.com/losnoco/Cog/blob/main/.github/images/MainWindow.png)

## Mini window

![Mini window](https://github.com/losnoco/Cog/blob/main/.github/images/MiniWindow.png)
