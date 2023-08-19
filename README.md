# cslw
C Simple Lua Wrapper (WIP)

## Building
```
gcc src/main.c src/cslw.h -Iyour/dir -Lyour/dir -llua
```
To enable LuaJIT support, add `-DSLW_USE_LUAJIT`

## Examples
- [tables.c](examples/example_table.c)
- More to come... for now, take a peek at [cslw.h](include/cslw/cslw.h).

## Supported C Versions:
- C99
- C11
- C17
- C18
- C2X

\>= C11 has [Generic](https://en.cppreference.com/w/c/language/generic) Support

## Notes
LuaJIT and Lua5.4 have been tested, on Windows and on Linux w/ WSL

This is still work-in-progress, I need to:
- Check more Lua versions on both OS's
- Add more safety
- Possibly add a `slwState_return(slw, slwt_string("retval1"), slwt_tnumber(42));` function of some sort, that way you don't manually have to push each return value.
- Cleanup functions that use variadic args
- Add more stack functions
- Improve performance (checking the generated ASM w/ IDA for Clang & GCC, + timing execution time)
- Move to CMake to make life easier with automatically finding Lua and handling cross-platform situations (Will to tomorrow?)