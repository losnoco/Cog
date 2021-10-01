This is a half assed attempt to replace Project64 with Mupen64plus, for a hoped better compatibility, and support for nicer recompiler cores for x86 and x86_64.

DONE:
* Get it working at all
* Verify old x86_64 recompiler works
* Converted x86 recompiler

TODO:
* Get new recompiler converted and working

Usage notes:

If your platform is x86 or x86_64, you may define ARCH_MIN_SSE2 for the rsp.c inside the rsp_lle subdirectory. You may also define DYNAREC and include the contents of either the r4300/x86 or r4300/x86_64 directories, and exclude the r4300/empty_dynarec.c file.

If you are not either of the above architectures, you include r4300/empty_dynarec.c and do not define DYNAREC, in which case the fastest you get is a cached interpreter, which "compiles" blocks of opcode function pointers and their pre-decoded parameters.

In either case, you do not include the contents of the new_dynarec folder, since that code is not ready for use with this library yet.
