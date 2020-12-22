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

https://kode54.net/cog/

--Christopher Snowhill (chris@kode54.net)

ADDENDUM - 2018-06-21

You will need to run the following to retrieve all the source code:

```
git submodule update --init --recursive
```

--Christopher Snowhill (chris@kode54.net)

ADDENDUM - 2020-12-22

Please feel free to contribute, but if you need to build it yourself
for testing, please do the following:

1) `git commit` all of your changes in however many steps
2) `git apply Scripts/fix_team_ids.patch` and hopefully it applies
3) Build and test your changes

Please try not to commit any parts of that patch to the tree, as it
breaks code signing and notarization. Maybe someone will know an
alternative solution?
