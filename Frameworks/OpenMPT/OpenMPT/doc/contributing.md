
Contributing
============

OpenMPT, libopenmpt, openmpt123, xmp-openmpt and in_openmpt are developed in
the Subversion repository at
[https://source.openmpt.org/browse/openmpt/trunk/OpenMPT/](https://source.openmpt.org/browse/openmpt/trunk/OpenMPT/).
Patches can be provided either against this Subversion repository or against our
GitHub mirror at
[https://github.com/OpenMPT/openmpt/](https://github.com/OpenMPT/openmpt/).

We do not have a developer mailing list. Discussions about new features or
problems can happen at:
 *  [Issue Tracker](https://bugs.openmpt.org/), preferred for specific bug
    reports or bug fixes and feature development discussion
 *  [Forum](https://forum.openmpt.org/), preferred for long-term discussion of
    new features or specific questions about development
 *  [IRC channel (`EsperNET/#modplug`)](irc://irc.esper.net:5555/#modplug),
    preferred for shorter questions
 *  [GitHub pull requests](https://github.com/OpenMPT/openmpt/pulls), please
    only use for rather tiny fixes, see below

For patch submissions, please also see
[OpenMPT style guide](openmpt_styleguide.md) and
[libopenmpt style guide](libopenmpt_styleguide.md).

### Contributing via GitHub

As OpenMPT is developed in a Subversion repository and the GitHub repository is
just mirrored from that, we cannot directly take pull requests via GitHub. We
recognize that, especially for tiny bug fixes, the burden to choose a different
way than GitHub for contributing can be too high. Thus, we will of course react,
give feedback, and take patches also via GitHub pull requests. However, as the
GitHub repository is strictly downstream from our Subversion repository (and
this will not change, due to considerable complications when synchronizing this
two-way), we cannot directly merge pull requests on GitHub. We will merge
contributions to our Subversion repository, which will then in turn be mirrored
to GitHub automatically, after which we will close the pull request. Authorship
attribution in git relies on the email address used in the commit header, which
is not how it usually works in Subversion. We will thus add an additional line
to the commit message in the form of `Patch-by: John Doe <user@example.com>`. If
you prefer to be attributed with your nickname and/or without your email
address, that would also be fine for us.
