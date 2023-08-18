#ifndef CSLW_H
#define CSLW_H

#define SLW_C11_VERSION 201112L

#if __cplusplus > 0
    #define SLW_LANGUAGE_CPP
#else
    #if defined(__STDC_VERSION__)
        #if __STDC_VERSION__ >= SLW_C11_VERSION
            #define SLW_LANGUAGE_C11
            #define SLW_GENERICS_SUPPORT
        #elif __STDC_VERSION__ >= 199901L
            #define SLW_LANGUAGE_C99 // Not really used, I can probably remove this.
        #endif
    #endif
#endif

#if !defined(SLW_RECURSION_DEPTH)
    #define SLW_RECURSION_DEPTH 32
#endif

#if !defined(SLW_ENABLE_ASSERTIONS)
    #if defined(NDEBUG) && !defined(_DEBUG)
        #define SLW_ENABLE_ASSERTIONS 0
    #else
        #define SLW_ENABLE_ASSERTIONS 1
    #endif
#endif

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"

#include <stdint.h>
#include <stdbool.h>

// Type Definitions
//------------------------------------------------------------------------
typedef struct slwTableValue_t slwTableValue_t;
typedef struct slwTable slwTable;

// Definitions
//------------------------------------------------------------------------
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

#if defined(__GNUC__) || defined(__clang__)
    #define SLW_INLINE inline __attribute__((always_inline))
#elif defined(_MSC_VER)
    #define SLW_INLINE __forceinline
#else
    #define SLW_INLINE inline
#endif

#if SLW_ENABLE_ASSERTIONS == 1
    #include <assert.h>
    #define slw_assert(c) assert(c)
#else
    #define slw_assert(c) ((void)0)
#endif

#define slw_internal static

#if !defined(slw_malloc)
#define slw_malloc(sz) malloc(sz)
#endif

#if !defined(slw_calloc)
#define slw_calloc(c, sz) calloc(c, sz)
#endif

#if !defined(slw_realloc)
#define slw_realloc(b, sz) realloc(b, sz)
#endif

#if !defined(slw_free)
#define slw_free(b) free(b)
#endif

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
// TODO: Maybe all stack functions should be: `slwStack_xxx`

SLW_API void slwState_destroy(slwState* slw);
SLW_API void slwState_close(slwState* slw);

SLW_API void slwState_openlibraries(slwState* slw, const uint32_t libs);
SLW_API void slwState_openlib(slwState* slw, const char* name, lua_CFunction func);

SLW_API int slwState_runstring(slwState* slw, const char* str);
SLW_API int slwState_runfile(slwState* slw, const char* filename);

SLW_API bool slwState_call_fn_at(slwState* slw, size_t idx, ...);
SLW_API bool slwState_call_fn(slwState* slw, const char* name, ...);

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
// TODO: Add slwTable* to ^

// Set Functions (Globals)
SLW_API void slwState_setstring(slwState* slw, const char* name, const char* str);
SLW_API const char* slwState_setfstring(slwState* slw, const char* name, const char* fmt, ...);
SLW_API void slwState_setnumber(slwState* slw, const char* name, double num);
SLW_API void slwState_setint(slwState* slw, const char* name, uint64_t num);
SLW_API void slwState_setbool(slwState* slw, const char* name, bool b);
SLW_API void slwState_setcfunction(slwState* slw, const char* name, lua_CFunction fn);
SLW_API void slwState_setlightudata(slwState* slw, const char* name, void* data);
SLW_API void slwState_setcclosure(slwState* slw, const char* name, lua_CFunction fn, int n);
SLW_API void slwState_settable(slwState* slw, const char* name, slwTable* slt);
SLW_API void slwState_settable2(slwState* slw, ...);
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
    slwTable*:              slwState_settable,     \
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

// Table Stuff
typedef union slwValue_u
{
    const char* s;
    double d;
    int i;
    bool b;
    slwTable* t;
    void* u;
    lua_CFunction f;
} slwValue_u;

struct slwTableValue_t
{
    const char* name; // optional for indexed tables
    uint8_t ltype;
    slwValue_u value;
};

struct slwTable
{
    slwTableValue_t* elements;
    size_t size;
};

#define slwt_tlightuserdata(x) ((slwTableValue_t) {.ltype = LUA_TLIGHTUSERDATA,   .value.u = x})
#define slwt_tfunction(x)      ((slwTableValue_t) {.ltype = LUA_TFUNCTION,   .value.f = x})
#define slwt_tboolean(x)       ((slwTableValue_t) {.ltype = LUA_TBOOLEAN, .value.b = x})
#define slwt_tstring(x)        ((slwTableValue_t) {.ltype = LUA_TSTRING,  .value.s = x})
#define slwt_tnumber(x)        ((slwTableValue_t) {.ltype = LUA_TNUMBER,  .value.d = x})
#define slwt_ttable(x)         ((slwTableValue_t) {.ltype = LUA_TTABLE,   .value.t = x})
#define slwt_tnil              ((slwTableValue_t) {.ltype = LUA_TNIL})

SLW_API slwTable*        slwTable_create();
SLW_API slwTable*        slwTable_createkv(slwState* slw, ...);
SLW_API slwTable*        slwTable_createi(slwState* slw, ...);
SLW_API void             slwTable_free(slwTable* slt);

SLW_API void             slwTable_push(slwState* slw, slwTable* slt);
SLW_API slwTableValue_t* slwTable_get_from_key(slwTable* slt, const char* key);
SLW_API slwTable*        slwTable_get_at(slwState* slw, const int32_t idx);
SLW_API slwTable* slwTable_get(slwState* slw);

SLW_API void             slwTable_setval(slwTable* slt, ...);

SLW_API void             slwTable_setstring(slwTable* slt, const char* name, const char* str);
SLW_API void             slwTable_setfstring(slwTable* slt, const char* name, const char* fmt, ...);
SLW_API void             slwTable_setnumber(slwTable* slt, const char* name, double num);
SLW_API void             slwTable_setint(slwTable* slt, const char* name, uint64_t num);
SLW_API void             slwTable_setbool(slwTable* slt, const char* name, bool b);
SLW_API void             slwTable_setcfunction(slwTable* slt, const char* name, lua_CFunction fn);
SLW_API void             slwTable_setlightudata(slwTable* slt, const char* name, void* data);
SLW_API void             slwTable_settable(slwTable* slt, const char* name, slwTable* tbl);
SLW_API void             slwTable_setnil(slwTable* slt, const char* name);

#if defined(SLW_GENERICS_SUPPORT)
#define slwTable_set(s, x, y) _Generic((y),        \
    lua_CFunction:          slwTable_setcfunction, \
    char*:                  slwTable_setstring,    \
    int:                    slwTable_setint,       \
    uint16_t:               slwTable_setint,       \
    uint64_t:               slwTable_setint,       \
    double:                 slwTable_setnumber,    \
    float:                  slwTable_setnumber,    \
    bool:                   slwTable_setbool,      \
    slwTable*:              slwTable_settable,     \
    void*:                  slwTable_setlightudata \
    )(s, x, y)
#endif

// Can pass NULL to `slwTable_dump` if you don't want the header and footer prints.
SLW_API void             slwTable_dump(slwState* slw, slwTable* slt, const char* name, int depth);

// Dumps a table from a Lua global.
SLW_API void             slwTable_dumpg(slwState* slw, const char* name);

#endif