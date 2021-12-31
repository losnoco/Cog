update-generated.sh requires the following:

```
brew install libgcrypt xa coreutils
```

For these needs:

1) libgcrypt is required by the autoreconf process, even if it is
   being configured as excluded or disabled.
2) xa is required to assemble the SID player binary objects in the
   repository.
3) coreutils is required for GNU od, because BSD od as included
   with macOS does not support the -w switch. The included autoconf
   scripts will prefer `god` if found.

