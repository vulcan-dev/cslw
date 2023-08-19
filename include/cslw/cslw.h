#ifndef CSLW_H
#define CSLW_H

#if defined(CLW_USE_LUAJIT)
    #include "luajit.h"
#endif

#include "lualib.h"
#include "lauxlib.h"

#include <stdint.h>
#include <stdbool.h>

// Type Definitions
//------------------------------------------------------------------------
typedef struct slwTableValue slwTableValue;
typedef struct slwTable slwTable;

// Definitions
//------------------------------------------------------------------------
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

#if defined(NDEBUG) && !defined(_DEBUG)
    #define SLW_RELEASE
#else
    #define SLW_DEBUG
#endif

#if !defined(SLW_ENABLE_ASSERTIONS)
    #if defined(NDEBUG) && !defined(_DEBUG)
        #define SLW_ENABLE_ASSERTIONS 0
    #else
        #define SLW_ENABLE_ASSERTIONS 1
    #endif
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

#if defined(__GNUC__) || defined(__clang__)
    #if __GNUC__ >= 10 || __clang_major__ >= 9
        #define SLW_NODISCARD [[nodiscard]]
    #else
        #define SLW_NODISCARD
    #endif

    #define SLW_INLINE inline __attribute__((always_inline))
#elif defined(_MSC_VER)
    #define SLW_INLINE __forceinline
    #define SLW_NODISCARD
#else
    #define SLW_INLINE inline
#endif

#if SLW_ENABLE_ASSERTIONS == 1
    #include <assert.h>
    #define SLW_ASSERT(c) assert(c)
#else
    #define SLW_ASSERT(c) ((void)0)
#endif

#define SLW_INTERNAL static

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

#if defined(SLW_DEBUG)
    #define SLW_CHECKSTATE(x)   \
        SLW_ASSERT((x) != NULL); \
        SLW_ASSERT((x)->LState != NULL)
#else
    #define SLW_CHECKSTATE(x)
#endif

// Lua Definitions
//------------------------------------------------------------------------
#if defined(CLW_USE_LUAJIT)
    #if CLW_USE_LUAJIT != 0 && !defined(CLW_USING_LUAJIT_I_)
        #define CLW_USING_LUAJIT 1
    #else
        #define CLW_USING_LUAJIT 0
    #endif
#endif

#if LUA_VERSION_NUM >= 504 && defined(LUA_COMPAT_BITLIB)
    #define __cslw_bit32_manual 1
#else
    #define __cslw_bit32_manual 0
#endif

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

#if defined(CLW_USE_LUAJIT)
    #define slw_lib_jit     1 << 10
    #define slw_lib_ffi     1 << 11
#else
    #define slw_lib_jit     0
    #define slw_lib_ffi     0
#endif

#define slw_lib_all slw_lib_package   | slw_lib_table | slw_lib_string |                 \
                    slw_lib_math      | slw_lib_debug | slw_lib_io     |                 \
                    slw_lib_coroutine | slw_lib_os    | slw_lib_utf8   | slw_lib_bit32 | \
                    slw_lib_jit       | slw_lib_ffi

// Structures
//------------------------------------------------------------------------
typedef struct slwState
{
    lua_State* LState;
} slwState;

typedef union slwValue
{
    const char* s;
    double d;
    int i;
    bool b;
    slwTable* t;
    void* u;
    lua_CFunction f;
} slwValue;

typedef struct slwReturnValue
{
    slwValue value;
    bool exists;
} slwReturnValue;

// Table Stuff
struct slwTableValue
{
    const char* name;
    uint8_t ltype;
    slwValue value;
};

struct slwTable
{
    slwTableValue* elements;
    size_t size;
};

// Functions
//------------------------------------------------------------------------
// TODO: Maybe all stack functions should be: `slwStack_xxx`

/**
 * Destroys a `slwState*`, make sure it's valid before you try to destroy it.
 */
SLW_API void slwState_destroy(slwState* slw);

/**
 * Closes the Lua state inside of the slwState
 */
SLW_API void slwState_close(slwState* slw);

/**
 * Opens libraries for the `slwState`
 * 
 * Example:
 * `slwState_openlibraries(slw, slw_lib_package | slw_lib_os)`
 */
SLW_API void slwState_openlibraries(slwState* slw, const uint32_t libs);

/**
 * Opens a library for the `slwState`, it takes the entry name and the function.
 */
SLW_API void slwState_openlib(slwState* slw, const char* name, lua_CFunction func);

/**
 * Executes the passed Lua string, returns false on failiure.
 */
SLW_NODISCARD SLW_API bool slwState_runstring(slwState* slw, const char* str);

/**
 * Executes the passed Lua file path, returns false on failiure.
 */
SLW_NODISCARD SLW_API bool slwState_runfile(slwState* slw, const char* filename);

/**
 * Calls a function at a specific index on the stack, returns false on failiure.
 * Important! Arguments must be of type: `slwTableValue`, so use `slwt_t*` macros.
 * 
 * Example: `slwState_call_fn_at(slw, -1, slwt_tstring("arg1"), slwt_tnumber(4), slwt_tstring("arg3")`
 */
SLW_NODISCARD SLW_API bool slwState_call_fn_at(slwState* slw, const size_t idx, ...);

/**
 * Calls `lua_getglobal` and then `slwState_call_fn_at` with -1 as the index.
 * Check that function for example usage.
 */
SLW_NODISCARD SLW_API bool slwState_call_fn(slwState* slw, const char* name, ...);

/**
 * Creates an empty `slwState` and an empty Lua State
 */
SLW_NODISCARD SLW_API slwState* slwState_new_empty();

/**
 * Returns a `slwState` with the specified libraries.
 * 
 * Example:
 * `slwState_new_with(slw, slw_lib_package | slw_lib_os)`
 */
SLW_NODISCARD SLW_API slwState* slwState_new_with(const uint32_t libs);

/**
 * Returns a `slwState` from another `slwState`
 */
SLW_NODISCARD SLW_API slwState* slwState_new_from_slws(slwState* slw);

/**
 * Returns a `slwState` from a Lua State
 */
SLW_NODISCARD SLW_API slwState* slwState_new_from_luas(lua_State* L);

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

// Stack Functions
//------------------------------------------------------------------------
SLW_API void slwStack_pop(slwState* slw, const int32_t n);

SLW_NODISCARD SLW_API bool slwStack_isboolean(slwState* slw, const int32_t idx);
SLW_NODISCARD SLW_API bool slwStack_iscfunction(slwState* slw, const int32_t idx);
SLW_NODISCARD SLW_API bool slwStack_isfunction(slwState* slw, const int32_t idx);
SLW_NODISCARD SLW_API bool slwStack_islightuserdata(slwState* slw, const int32_t idx);
SLW_NODISCARD SLW_API bool slwStack_isnil(slwState* slw, const int32_t idx);
SLW_NODISCARD SLW_API bool slwStack_isnone(slwState* slw, const int32_t idx);
SLW_NODISCARD SLW_API bool slwStack_isnoneornil(slwState* slw, const int32_t idx);
SLW_NODISCARD SLW_API bool slwStack_isnumber(slwState* slw, const int32_t idx);
SLW_NODISCARD SLW_API bool slwStack_isstring(slwState* slw, const int32_t idx);
SLW_NODISCARD SLW_API bool slwStack_istable(slwState* slw, const int32_t idx);
SLW_NODISCARD SLW_API bool slwStack_isthread(slwState* slw, const int32_t idx);
SLW_NODISCARD SLW_API bool slwStack_isuserdata(slwState* slw, const int32_t idx);

SLW_NODISCARD SLW_API bool          slwStack_toboolean(slwState* slw, const int32_t idx);
SLW_NODISCARD SLW_API lua_CFunction slwStack_tocfunction(slwState* slw, const int32_t idx);
SLW_NODISCARD SLW_API int64_t       slwStack_tointeger(slwState* slw, const int32_t idx);
SLW_NODISCARD SLW_API const char*   slwStack_tolstring(slwState* slw, const int32_t idx, size_t* len);
SLW_NODISCARD SLW_API double        slwStack_tonumber(slwState* slw, const int32_t idx);
SLW_NODISCARD SLW_API const void*   slwStack_topointer(slwState* slw, const int32_t idx);
SLW_NODISCARD SLW_API const char*   slwStack_tostring(slwState* slw, const int32_t idx);
SLW_NODISCARD SLW_API void*         slwStack_tothread(slwState* slw, const int32_t idx);
SLW_NODISCARD SLW_API void*         slwStack_touserdata(slwState* slw, const int32_t idx);

SLW_API void slwStack_setglobal(slwState* slw, const char* name);

SLW_API const char* slwStack_pushfstring(slwState* slw, const char* fmt, ...);
SLW_API void slwStack_pushstring(slwState* slw, const char* str);
SLW_API void slwStack_pushnumber(slwState* slw, double num);
SLW_API void slwStack_pushint(slwState* slw, int64_t num);
SLW_API void slwStack_pushboolean(slwState* slw, bool b);
SLW_API void slwStack_pushcfunction(slwState* lwState, lua_CFunction fn);
SLW_API void slwStack_pushlightudata(slwState* lwState, void* data);
SLW_API void slwStack_pushcclosure(slwState* lwState, lua_CFunction fn, int n);
SLW_API void slwStack_pushnil(slwState* slw);

#if defined(SLW_GENERICS_SUPPORT)
    #define slwState_push(s, x) _Generic((x),           \
        lua_CFunction:          slwStack_pushcfunction, \
        char*:                  slwStack_pushstring,    \
        int:                    slwStack_pushint,       \
        uint16_t:               slwStack_pushint,       \
        uint64_t:               slwStack_pushint,       \
        double:                 slwStack_pushnumber,    \
        float:                  slwStack_pushnumber,    \
        bool:                   slwStack_pushboolean,      \
        void*:                  slwStack_pushlightudata \
    )(s, x)
#endif
// TODO: Add slwTable* to ^

// Set Functions (Globals)
//------------------------------------------------------------------------
SLW_API void slwState_setstring(slwState* slw, const char* name, const char* str);
SLW_API const char* slwState_setfstring(slwState* slw, const char* name, const char* fmt, ...);
SLW_API void slwState_setnumber(slwState* slw, const char* name, double num);
SLW_API void slwState_setint(slwState* slw, const char* name, uint64_t num);
SLW_API void slwState_setbool(slwState* slw, const char* name, bool b);
SLW_API void slwState_setcfunction(slwState* slw, const char* name, lua_CFunction fn);
SLW_API void slwState_setlightudata(slwState* slw, const char* name, void* data);
SLW_API void slwState_setcclosure(slwState* slw, const char* name, lua_CFunction fn, int n);
SLW_API void slwState_settable(slwState* slw, const char* name, slwTable* slt);

/**
 * `slwState_settable2`
 * This function is similar to `slwState_settable`, the difference is that it uses variadic arguments for the keys
 * 
 * Example:
 * slwState_settable2(slw, "myKey1", "myKey2", "finalKey", slwt_tstring("Hello Sailor!")
 */
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

// Get Functions (Globals)
//------------------------------------------------------------------------
SLW_NODISCARD SLW_API slwReturnValue slwState_type_to_c(slwState* slw, const int type, const int idx);
SLW_NODISCARD SLW_API slwReturnValue slwState_get(slwState* slw, const char* name);
SLW_NODISCARD SLW_API slwReturnValue slwState_getstring(slwState* slw, const char* name);
SLW_NODISCARD SLW_API slwReturnValue slwState_getnumber(slwState* slw, const char* name);
SLW_NODISCARD SLW_API slwReturnValue slwState_getint(slwState* slw, const char* name);
SLW_NODISCARD SLW_API slwReturnValue slwState_getbool(slwState* slw, const char* name);
SLW_NODISCARD SLW_API slwReturnValue slwState_getfunction(slwState* slw, const char* name);
SLW_NODISCARD SLW_API slwReturnValue slwState_getcfunction(slwState* lwState, const char* name);
SLW_NODISCARD SLW_API slwReturnValue slwState_getuserdata(slwState* lwState, const char* name);
SLW_NODISCARD SLW_API slwReturnValue slwState_getnil(slwState* slw, const char* name);

#define slwt_tlightuserdata(x) ((slwTableValue) {.ltype = LUA_TLIGHTUSERDATA,   .value.u = x})
#define slwt_tfunction(x)      ((slwTableValue) {.ltype = LUA_TFUNCTION,   .value.f = x})
#define slwt_tboolean(x)       ((slwTableValue) {.ltype = LUA_TBOOLEAN, .value.b = x})
#define slwt_tstring(x)        ((slwTableValue) {.ltype = LUA_TSTRING,  .value.s = x})
#define slwt_tnumber(x)        ((slwTableValue) {.ltype = LUA_TNUMBER,  .value.d = x})
#define slwt_ttable(x)         ((slwTableValue) {.ltype = LUA_TTABLE,   .value.t = x})
#define slwt_tnil              ((slwTableValue) {.ltype = LUA_TNIL})

SLW_NODISCARD SLW_API slwTable*        slwTable_create();
SLW_NODISCARD SLW_API slwTable*        slwTable_createkv(slwState* slw, ...);
SLW_NODISCARD SLW_API slwTable*        slwTable_createi(slwState* slw, ...);
SLW_API void                           slwTable_free(slwTable* slt);

SLW_API void                           slwTable_push(slwState* slw, slwTable* slt);
SLW_NODISCARD SLW_API slwTable*        slwTable_get_at(slwState* slw, const int32_t idx);
SLW_NODISCARD SLW_API slwTable*        slwTable_get(slwState* slw);
SLW_NODISCARD SLW_API slwTableValue*   slwTable_getkey(slwTable* slt, const char* key);

/**
 * This is the same as `slwState_settable2`, the difference is that it modifies the table in the Lua State instead of the table structure.
 * You can use this in conjuction with `slwState_settable2`.
 * 
 * Note: I may create a macro to do both, that way if you update the table struct, it will also update it in the Lua State.
 */
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
/**
 * Some comment
 */
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

/**
 * This function dumps the table to stdout, `name` is optional.
 */
SLW_API void             slwTable_dump(slwState* slw, slwTable* slt, const char* name, int depth);

/**
 * Similar to `slwTable_dump`, this one takes the name of the table and it uses `lua_getglobal` to get it.
 */
SLW_API void             slwTable_dumpg(slwState* slw, const char* name);

#endif