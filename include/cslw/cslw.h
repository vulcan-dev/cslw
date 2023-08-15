#ifndef CSLW_H
#define CSLW_H

#if __cplusplus > 0
    #define SLW_LANGUAGE_CPP
#else
    #if defined(__STDC_VERSION__)
        #if __STDC_VERSION__ >= 201710L
            #define SLW_LANGUAGE_C18
        #elif __STDC_VERSION__ >= 201112L
            #define SLW_LANGUAGE_C11
            #define SLW_GENERICS_SUPPORT
        #elif __STDC_VERSION__ >= 199901L
            #define SLW_LANGUAGE_C99
        #endif
    #endif
#endif

#define SLW_EXTERN_C_BEGIN extern "C" {
#define SLW_EXTERN_C_END }

#if defined(SLW_LANGUAGE_CPP)
    #define SLW_IGNORE_BOOL
    SLW_EXTERN_C_BEGIN
#endif

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"

#if !defined(LUA_GLOBALSINDEX)
    #define LUA_GLOBALSINDEX (-10002)
#endif

#include <stdint.h>

// Definitions
//------------------------------------------------------------------------
#ifndef SLW_IGNORE_BOOL
    typedef enum { false, true } bool;
#endif

#if defined(_WIN32)
    #if defined(SLW_EXPORT)
        #define SLW_API __declspec(dllexport)
    #else
        #define SLW_API extern
    #endif
#else
    #if defined(SLW_EXPORT)
        #define SLW_API __attribute__((visibility("default")))
    #else
        #define SLW_API
    #endif
#endif

#if defined(SLW_ENABLE_ASSERTIONS)
    #include <assert.h>
    #define slw_assert(c) assert(c)
#else
    #define slw_assert(c) ((void)0)
#endif

#define slw_internal static
#define slw_malloc(sz) malloc(sz)
#define slw_free free

#if LUA_VERSION_NUM >= 504 && defined(LUA_COMPAT_BITLIB)
#define __cslw_bit32_manual 1
#else
#define __cslw_bit32_manual 0
#endif

// Libraries
#define slw_lib_package     1 << 0
#define slw_lib_table       1 << 1
#define slw_lib_string      1 << 2
#define slw_lib_math        1 << 3
#define slw_lib_debug       1 << 4
#define slw_lib_io          1 << 5
#define slw_lib_coroutine   1 << 6
#define slw_lib_os          1 << 7
#define slw_lib_utf8        1 << 8
#if __slw_bit32_manual
    #define slw_lib_bit32   1 << 9
#else
    #define slw_lib_bit32   0
#endif

#define slw_lib_all slw_lib_package   | slw_lib_table | slw_lib_string |               \
                    slw_lib_math      | slw_lib_debug | slw_lib_io     |               \
                    slw_lib_coroutine | slw_lib_os    | slw_lib_utf8   | slw_lib_bit32

// Structures
//------------------------------------------------------------------------
typedef struct slwState
{
    lua_State* LState;
} slwState;

// Functions
//------------------------------------------------------------------------
SLW_API void slwState_destroy(slwState* slw);
SLW_API void slwState_close(slwState* slw);

SLW_API void slwState_openlibraries(slwState* slw, const uint32_t libs);
SLW_API void slwState_openlib(slwState* slw, const char* name, lua_CFunction func);

SLW_API int slwState_runstring(slwState* slw, const char* str);
SLW_API int slwState_runfile(slwState* slw, const char* filename);

// New Functions
SLW_API slwState* slwState_new_empty();
SLW_API slwState* slwState_new_with(const uint32_t libs);
SLW_API slwState* slwState_new_from_slws(slwState* slw);
SLW_API slwState* slwState_new_from_luas(lua_State* L);

#if defined(SLW_GENERICS_SUPPORT)
    #define slwState_new(x) _Generic((x),   \
        default:    slwState_new_empty,     \
        int:        slwState_new_with,      \
        uint64_t:   slwState_new_with,      \
        uint32_t:   slwState_new_with,      \
        uint16_t:   slwState_new_with,      \
        slwState*:  slwState_new_from_slws, \
        lua_State*: slwState_new_from_luas  \
    )(x)
#endif

// Push Functions
SLW_API void slwState_pushstring(slwState* slw, const char* str);
SLW_API const char* slwState_pushfstring(slwState* slw, const char* fmt, ...);
SLW_API void slwState_pushnumber(slwState* slw, double num);
SLW_API void slwState_pushint(slwState* slw, int64_t num);
SLW_API void slwState_pushbool(slwState* slw, bool b);
SLW_API void slwState_pushcfunction(slwState* lwState, lua_CFunction fn);
SLW_API void slwState_pushlightudata(slwState* lwState, void* data);
SLW_API void slwState_pushcclosure(slwState* lwState, lua_CFunction fn, int n);
SLW_API void slwState_pushnil(slwState* slw);

#if defined(SLW_GENERICS_SUPPORT)
    #define slwState_push(s, x) _Generic((x),           \
        lua_CFunction:          slwState_pushcfunction, \
        char*:                  slwState_pushstring,    \
        int:                    slwState_pushint,       \
        uint16_t:               slwState_pushint,       \
        uint64_t:               slwState_pushint,       \
        double:                 slwState_pushnumber,    \
        float:                  slwState_pushnumber,    \
        bool:                   slwState_pushbool,      \
        void*:                  slwState_pushlightudata \
    )(s, x)
#endif

// Set Functions (Globals)
SLW_API void slwState_setstring(slwState* slw, const char* name, const char* str);
SLW_API const char* slwState_setfstring(slwState* slw, const char* name, const char* fmt, ...);
SLW_API void slwState_setnumber(slwState* slw, const char* name, double num);
SLW_API void slwState_setint(slwState* slw, const char* name, int64_t num);
SLW_API void slwState_setbool(slwState* slw, const char* name, bool b);
SLW_API void slwState_setcfunction(slwState* lwState, const char* name, lua_CFunction fn);
SLW_API void slwState_setlightudata(slwState* lwState, const char* name, void* data);
SLW_API void slwState_setcclosure(slwState* lwState, const char* name, lua_CFunction fn, int n);
SLW_API void slwState_setnil(slwState* slw, const char* name);

#if defined(SLW_GENERICS_SUPPORT)
#define slwState_set(s, x, y) _Generic((y),        \
    lua_CFunction:          slwState_setcfunction, \
    char*:                  slwState_setstring,    \
    int:                    slwState_setint,       \
    uint16_t:               slwState_setint,       \
    uint64_t:               slwState_setint,       \
    double:                 slwState_setnumber,    \
    float:                  slwState_setnumber,    \
    bool:                   slwState_setbool,      \
    void*:                  slwState_setlightudata \
)(s, x, y)
#endif

typedef struct slwReturnValue
{
    union
    {
        const char* s;
        double n;
        int i;
        bool b;
        lua_CFunction cfn;
        lua_State* t;
        void* udata;
    };
    bool exists;
} slwReturnValue;

// Get Functions (Globals)
SLW_API slwReturnValue slwState_type_to_c(slwState* slw, const int type, const int idx);
#define slwState_get(slw, name) slwState_type_to_c(slw, lua_getglobal((slw)->LState, name), -1)

SLW_API slwReturnValue slwState_getstring(slwState* slw, const char* name);
SLW_API slwReturnValue slwState_getnumber(slwState* slw, const char* name);
SLW_API slwReturnValue slwState_getint(slwState* slw, const char* name);
SLW_API slwReturnValue slwState_getbool(slwState* slw, const char* name);
SLW_API slwReturnValue slwState_getfunction(slwState* slw, const char* name);
SLW_API slwReturnValue slwState_getcfunction(slwState* lwState, const char* name);
SLW_API slwReturnValue slwState_getuserdata(slwState* lwState, const char* name);
SLW_API slwReturnValue slwState_getnil(slwState* slw, const char* name);

#if defined(SLW_LANGUAGE_CPP)
    SLW_EXTERN_C_END
#endif

#endif