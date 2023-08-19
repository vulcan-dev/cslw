#define SLW_TABLE_MAX_KEYS 32

#include "cslw/cslw.h"
#include <stdlib.h>
#include <assert.h>
#include <memory.h>
#include <string.h>
#include <stdio.h>

// Some Compatibility
// From: https://github.com/lunarmodules/lua-compat-5.3/blob/master/c-api/compat-5.3.h
//------------------------------------------------------------------------
#if !defined(COMPAT53_API)
int lua_isinteger(lua_State *L, int index) {
    if (lua_type(L, index) == LUA_TNUMBER) {
        lua_Number n = lua_tonumber(L, index);
        lua_Integer i = lua_tointeger(L, index);
        if (i == n)
        return 1;
    }
    return 0;
}

int lua_absindex (lua_State *L, int i) {
    if (i < 0 && i > LUA_REGISTRYINDEX)
        i += lua_gettop(L) + 1;
    return i;
}

int luaL_getsubtable (lua_State *L, int i, const char *name) {
    int abs_i = lua_absindex(L, i);
    luaL_checkstack(L, 3, "not enough stack slots");
    lua_pushstring(L, name);
    lua_gettable(L, abs_i);
    if (lua_istable(L, -1))
        return 1;
    lua_pop(L, 1);
    lua_newtable(L);
    lua_pushstring(L, name);
    lua_pushvalue(L, -2);
    lua_settable(L, abs_i);
    return 0;
}

void luaL_requiref (lua_State *L, const char *modname, lua_CFunction openf, int glb) {
    luaL_checkstack(L, 3, "not enough stack slots available");
    luaL_getsubtable(L, LUA_REGISTRYINDEX, "_LOADED");
    lua_getfield(L, -1, modname);
    if (lua_type(L, -1) == LUA_TNIL) {
        lua_pop(L, 1);
        lua_pushcfunction(L, openf);
        lua_pushstring(L, modname);
        lua_call(L, 1, 1);
        lua_pushvalue(L, -1);
        lua_setfield(L, -3, modname);
    }
    if (glb) {
        lua_pushvalue(L, -1);
        lua_setglobal(L, modname);
    }
    lua_replace(L, -2);
}
#endif

// Internal Functions (Mainly helper functions)
//------------------------------------------------------------------------
SLW_INLINE SLW_INTERNAL bool _is_integer(const double d) { return (int)d == d; }

SLW_INTERNAL void
_slwTable_push_value(slwState* slw, slwTableValue el)
{
    lua_State* L = slw->LState;

    switch (el.ltype)
    {
        case LUA_TSTRING:
            lua_pushstring(L, el.value.s);
            break;
        case LUA_TNUMBER:
            lua_pushnumber(L, el.value.d);
            break;
        case LUA_TBOOLEAN:
            lua_pushboolean(L, el.value.b);
            break;
        case LUA_TTABLE:
            slwTable_push(slw, el.value.t);
            break;
        case LUA_TLIGHTUSERDATA:
            lua_pushlightuserdata(L, el.value.u);
            break;
        case LUA_TFUNCTION:
            lua_pushcfunction(L, el.value.f);
            break;
        default:
            // TODO: Actual error/warn functions? (Not really for this, but for everything else)
            printf("[CSLW] Tried pushing unknown type: %d (name: %s)\n", el.ltype, el.name);
            break;
    }
}

SLW_INTERNAL void
_slwTable_set_value(slwTable* slt, const char* key, slwTableValue val)
{
    slwTableValue* sltTable = slwTable_getkey(slt, key);
    if (sltTable)
    {
        sltTable->value = val.value;
        sltTable->ltype = val.ltype;
        return;
    }

    val.name = key;
    slt->elements = (slwTableValue*)slw_realloc(slt->elements, sizeof(slwTableValue) * (slt->size + 1));
    slt->elements[slt->size++] = val;
}

// Functions
//------------------------------------------------------------------------
// Primary Functions
SLW_API void
slwState_destroy(slwState* slw)
{
    SLW_CHECKSTATE(slw);
    if (slw->LState)
        slwState_close(slw);

    free(slw);
    slw = NULL;
}

SLW_API slwState*
slwState_new_empty()
{
    lua_State* L = luaL_newstate();
    if (!L) return NULL;

    slwState* slw = (slwState*)slw_malloc(sizeof(slwState));
    slw->LState = L;

    return slw;
}

SLW_API slwState*
slwState_new_with(const uint32_t libs)
{
    slwState* slw = slwState_new_empty();
    if (!slw) return NULL;

    slwState_openlibraries(slw, libs);
    return slw;
}

SLW_API slwState*
slwState_new_from_slws(slwState* slw)
{
    SLW_CHECKSTATE(slw);
    SLW_CHECKSTATE(slw);

    slwState* newSLW = (slwState*)slw_malloc(sizeof(slwState));
    if (!newSLW)
        return NULL;

    newSLW->LState = slw->LState;

    return newSLW;
}

SLW_API slwState* slwState_new_from_luas(lua_State* L)
{
    SLW_ASSERT(L != NULL);

    slwState* slw = (slwState*)slw_malloc(sizeof(slwState));
    if (!slw)
        return NULL;

    slw->LState = L;

    return slw;
}

SLW_API void
slwState_close(slwState* slw)
{
    SLW_CHECKSTATE(slw);
    lua_close(slw->LState);
    slw->LState = NULL;
}

SLW_API void
slwState_openlibraries(slwState* slw, const uint32_t libs)
{
    SLW_CHECKSTATE(slw);

    luaopen_base(slw->LState);
    if (libs & slw_lib_package)
        slwState_openlib(slw, "package", luaopen_package);
    if (libs & slw_lib_table)
        slwState_openlib(slw, "table", luaopen_table);
    if (libs & slw_lib_string)
        slwState_openlib(slw, "string", luaopen_string);
    if (libs & slw_lib_math)
        slwState_openlib(slw, "math", luaopen_math);
    if (libs & slw_lib_debug)
        slwState_openlib(slw, "debug", luaopen_debug);
    if (libs & slw_lib_io)
        slwState_openlib(slw, "io", luaopen_io);
#if CLW_USING_LUAJIT == 0
    if (libs & slw_lib_coroutine)
        slwState_openlib(slw, "coroutine", luaopen_coroutine);
#endif
    if (libs & slw_lib_os)
        slwState_openlib(slw, "os", luaopen_os);
#if CLW_USING_LUAJIT == 0
    if (libs & slw_lib_utf8)
        slwState_openlib(slw, "utf8", luaopen_utf8);
#else
    if (libs & slw_lib_jit)
        slwState_openlib(slw, "jit", luaopen_jit);
    if (libs & slw_lib_ffi)
        slwState_openlib(slw, "ffi", luaopen_ffi);
#endif
#if __cslw_bit32_manual
    if (libs && slw_lib_bit32)
        slwState_openlib(slw, "bit32", luaopen_bit32);
#endif
}

SLW_API void
slwState_openlib(slwState* slw, const char* name, lua_CFunction func)
{
    SLW_CHECKSTATE(slw);
    lua_State* L = slw->LState;
    luaL_requiref(L, name, func, 1);
    lua_pop(L, 1);
}

SLW_API bool
slwState_runstring(slwState* slw, const char* str)
{
    SLW_CHECKSTATE(slw);
    lua_State* L = slw->LState;
    return (luaL_loadstring(L, str) || lua_pcall(L, 0, LUA_MULTRET, 0)) == 0;
}

SLW_API bool
slwState_runfile(slwState* slw, const char* filename)
{
    SLW_CHECKSTATE(slw);
    lua_State* L = slw->LState;
    return (luaL_loadfile(L, filename) || lua_pcall(L, 0, LUA_MULTRET, 0)) == 0;
}

SLW_API bool
slwState_call_fn_at(slwState* slw, const size_t idx, ...)
{
    SLW_CHECKSTATE(slw);
    lua_State* L = slw->LState;

    if (!lua_isfunction(L, idx))
        return false;

    int nargs = 0;
    
    va_list args;
    va_start(args, idx);

    slwTableValue* arg = NULL;
    while ((arg = va_arg(args, slwTableValue*)) != NULL)
    {
        _slwTable_push_value(slw, *arg);
        ++nargs;
    }
    va_end(args);

    if (lua_pcall(L, nargs, LUA_MULTRET, 0) != 0)
        return false;

    return true;
}

SLW_API bool
slwState_call_fn(slwState* slw, const char* name, ...)
{
    SLW_CHECKSTATE(slw);

    lua_getglobal(slw->LState, name);

    va_list args;
    va_start(args, name);

    bool result = slwState_call_fn_at(slw, -1, args);

    va_end(args);

    return result;
}

// Stack Functions
//------------------------------------------------------------------------
SLW_API SLW_INLINE void
slwStack_pop(slwState* slw, const int32_t n)
{
    SLW_CHECKSTATE(slw);
    lua_pop(slw->LState, n);
}

SLW_NODISCARD SLW_API SLW_INLINE bool
slwStack_isboolean(slwState* slw, const int32_t idx)
{
    SLW_CHECKSTATE(slw);
    return lua_isboolean(slw->LState, idx);
}

SLW_NODISCARD SLW_API SLW_INLINE bool
slwStack_iscfunction(slwState* slw, const int32_t idx)
{
    SLW_CHECKSTATE(slw);
    return lua_iscfunction(slw->LState, idx);
}

SLW_NODISCARD SLW_API SLW_INLINE bool
slwStack_isfunction(slwState* slw, const int32_t idx)
{
    SLW_CHECKSTATE(slw);
    return lua_isfunction(slw->LState, idx);
}

SLW_NODISCARD SLW_API SLW_INLINE bool
slwStack_islightuserdata(slwState* slw, const int32_t idx)
{
    SLW_CHECKSTATE(slw);
    return lua_islightuserdata(slw->LState, idx);
}

SLW_NODISCARD SLW_API SLW_INLINE bool
slwStack_isnil(slwState* slw, const int32_t idx)
{
    SLW_CHECKSTATE(slw);
    return lua_isnil(slw->LState, idx);
}

SLW_NODISCARD SLW_API SLW_INLINE bool
slwStack_isnone(slwState* slw, const int32_t idx)
{
    SLW_CHECKSTATE(slw);
    return lua_isnone(slw->LState, idx);
}

SLW_NODISCARD SLW_API SLW_INLINE bool
slwStack_isnoneornil(slwState* slw, const int32_t idx)
{
    SLW_CHECKSTATE(slw);
    return lua_isnoneornil(slw->LState, idx);
}

SLW_NODISCARD SLW_API SLW_INLINE bool
slwStack_isnumber(slwState* slw, const int32_t idx)
{
    SLW_CHECKSTATE(slw);
    return lua_isnumber(slw->LState, idx);
}

SLW_NODISCARD SLW_API SLW_INLINE bool
slwStack_isstring(slwState* slw, const int32_t idx)
{
    SLW_CHECKSTATE(slw);
    return lua_isstring(slw->LState, idx);
}

SLW_NODISCARD SLW_API SLW_INLINE bool
slwStack_istable(slwState* slw, const int32_t idx)
{
    SLW_CHECKSTATE(slw);
    return lua_istable(slw->LState, idx);
}

SLW_NODISCARD SLW_API SLW_INLINE bool
slwStack_isthread(slwState* slw, const int32_t idx)
{
    SLW_CHECKSTATE(slw);
    return lua_isthread(slw->LState, idx);
}

SLW_NODISCARD SLW_API SLW_INLINE bool
slwStack_isuserdata(slwState* slw, const int32_t idx)
{
    SLW_CHECKSTATE(slw);
    return lua_isuserdata(slw->LState, idx);
}

SLW_NODISCARD SLW_API SLW_INLINE bool
slwStack_toboolean(slwState* slw, const int32_t idx)
{
    SLW_CHECKSTATE(slw);
    return lua_toboolean(slw->LState, idx);
}

SLW_NODISCARD SLW_API SLW_INLINE lua_CFunction
slwStack_tocfunction(slwState* slw, const int32_t idx)
{
    SLW_CHECKSTATE(slw);
    return lua_tocfunction(slw->LState, idx);
}

SLW_NODISCARD SLW_API SLW_INLINE int64_t
slwStack_tointeger(slwState* slw, const int32_t idx)
{
    SLW_CHECKSTATE(slw);
    return lua_tointeger(slw->LState, idx);
}

SLW_NODISCARD SLW_API SLW_INLINE const char*
slwStack_tolstring(slwState* slw, const int32_t idx, size_t* len)
{
    SLW_CHECKSTATE(slw);
    return lua_tolstring(slw->LState, idx, len);
}

SLW_NODISCARD SLW_API SLW_INLINE double
slwStack_tonumber(slwState* slw, const int32_t idx)
{
    SLW_CHECKSTATE(slw);
    return lua_tonumber(slw->LState, idx);
}

SLW_NODISCARD SLW_API SLW_INLINE const void*
slwStack_topointer(slwState* slw, const int32_t idx)
{
    SLW_CHECKSTATE(slw);
    return lua_topointer(slw->LState, idx);
}

SLW_NODISCARD SLW_API SLW_INLINE const char*
slwStack_tostring(slwState* slw, const int32_t idx)
{
    SLW_CHECKSTATE(slw);
    return lua_tostring(slw->LState, idx);
}

SLW_NODISCARD SLW_API SLW_INLINE void*
slwStack_tothread(slwState* slw, const int32_t idx)
{
    SLW_CHECKSTATE(slw);
    return lua_tothread(slw->LState, idx);
}

SLW_NODISCARD SLW_API SLW_INLINE void*
slwStack_touserdata(slwState* slw, const int32_t idx)
{
    SLW_CHECKSTATE(slw);
    return lua_touserdata(slw->LState, idx);
}

SLW_API SLW_INLINE void
slwStack_setglobal(slwState* slw, const char* name)
{
    SLW_CHECKSTATE(slw);
    lua_setglobal(slw->LState, name);
}

// Push Functions
//----------------------------------
SLW_API SLW_INLINE void
slwStack_pushstring(slwState* slw, const char* str)
{
    SLW_CHECKSTATE(slw);
    lua_pushstring(slw->LState, str);
}

SLW_API const char*
slwStack_pushfstring(slwState* slw, const char* fmt, ...)
{
    SLW_CHECKSTATE(slw);

    va_list args;
    va_start(args, fmt);

    const char* result = lua_pushvfstring(slw->LState, fmt, args);
    va_end(args);
    return result;
}

SLW_API SLW_INLINE void
slwStack_pushnumber(slwState* slw, double num)
{
    SLW_CHECKSTATE(slw);
    lua_pushnumber(slw->LState, num);
}

SLW_API SLW_INLINE void
slwStack_pushint(slwState* slw, int64_t num)
{
    SLW_CHECKSTATE(slw);
    lua_pushinteger(slw->LState, num);
}

SLW_API SLW_INLINE void
slwStack_pushboolean(slwState* slw, bool b)
{
    SLW_CHECKSTATE(slw);
    lua_pushboolean(slw->LState, b);
}

SLW_API SLW_INLINE void
slwStack_pushcfunction(slwState* slw, lua_CFunction fn)
{
    SLW_CHECKSTATE(slw);
    lua_pushcfunction(slw->LState, fn);
}

SLW_API SLW_INLINE void
slwStack_pushlightudata(slwState* slw, void* data)
{
    SLW_CHECKSTATE(slw);
    lua_pushlightuserdata(slw->LState, data);
}

SLW_API SLW_INLINE void
slwStack_pushcclosure(slwState* slw, lua_CFunction fn, int n)
{
    SLW_CHECKSTATE(slw);
    lua_pushcclosure(slw->LState, fn, n);
}

SLW_API SLW_INLINE void
slwStack_pushnil(slwState* slw)
{
    SLW_CHECKSTATE(slw);
    lua_pushnil(slw->LState);
}

// Set Functions (Globals)
//------------------------------------------------------------------------
SLW_API SLW_INLINE void
slwState_setstring(slwState* slw, const char* name, const char* str)
{
    SLW_CHECKSTATE(slw);
    lua_pushstring(slw->LState, str);
    lua_setglobal(slw->LState, name);
}

SLW_API const char*
slwset_setfstring(slwState* slw, const char* name, const char* fmt, ...)
{
    SLW_CHECKSTATE(slw);

    va_list args;
    va_start(args, fmt);

    const char* result = lua_pushvfstring(slw->LState, fmt, args);
    va_end(args);

    lua_setglobal(slw->LState, name);
    return result;
}

SLW_API SLW_INLINE void
slwState_setnumber(slwState* slw, const char* name, double num)
{
    SLW_CHECKSTATE(slw);
    lua_pushnumber(slw->LState, num);
    lua_setglobal(slw->LState, name);
}

SLW_API SLW_INLINE void
slwState_setint(slwState* slw, const char* name, uint64_t num)
{
    SLW_CHECKSTATE(slw);
    lua_pushinteger(slw->LState, num);
    lua_setglobal(slw->LState, name);
}

SLW_API SLW_INLINE void
slwState_setbool(slwState* slw, const char* name, bool b)
{
    SLW_CHECKSTATE(slw);
    lua_pushboolean(slw->LState, b);
    lua_setglobal(slw->LState, name);
}

SLW_API SLW_INLINE void
slwState_setcfunction(slwState* slw, const char* name, lua_CFunction fn)
{
    SLW_CHECKSTATE(slw);
    lua_pushcfunction(slw->LState, fn);
    lua_setglobal(slw->LState, name);
}

SLW_API SLW_INLINE void
slwState_setlightudata(slwState* slw, const char* name, void* data)
{
    SLW_CHECKSTATE(slw);
    lua_pushlightuserdata(slw->LState, data);

    lua_setglobal(slw->LState, name);
}

SLW_API SLW_INLINE void
slwState_setcclosure(slwState* slw, const char* name, lua_CFunction fn, int n)
{
    SLW_CHECKSTATE(slw);
    lua_pushcclosure(slw->LState, fn, n);
    lua_setglobal(slw->LState, name);
}

SLW_API SLW_INLINE void
slwState_settable(slwState* slw, const char* name, slwTable* slt)
{
    SLW_CHECKSTATE(slw);
    SLW_ASSERT(slt != NULL);

    slwTable_push(slw, slt);
    lua_setglobal(slw->LState, name);
}

SLW_API SLW_INLINE void
slwState_setnil(slwState* slw, const char* name)
{
    SLW_CHECKSTATE(slw);
    lua_pushnil(slw->LState);
    lua_setglobal(slw->LState, name);
}

// Get Functions (Globals)
//------------------------------------------------------------------------
SLW_API slwReturnValue
slwState_type_to_c(slwState* slw, const int type, const int idx)
{
    SLW_CHECKSTATE(slw);
    slwReturnValue ret;
    ret.exists = true;

    lua_State* L = slw->LState;

    switch(type)
    {
        case LUA_TNONE:
            ret.exists = false;
            return ret;
        case LUA_TBOOLEAN:
            ret.value.b = lua_toboolean(L, idx);
            break;
        case LUA_TNUMBER:
            const double d = lua_tonumber(L, idx);
            if (_is_integer(d))
                ret.value.i = (int)d;
            else
                ret.value.d = d;
            break;
        case LUA_TSTRING:
            ret.value.s = lua_tostring(L, idx);
            break;
        case LUA_TTABLE:
            ret.value.t = slwTable_get_at(slw, idx); // not tested, but I don't see why it wouldn't work.
            break;
        case LUA_TFUNCTION:
            ret.value.f = lua_tocfunction(L, idx);
            break;
        case LUA_TUSERDATA:
            ret.value.u = lua_touserdata(L, idx);
            break;
        case LUA_TTHREAD:
            ret.value.u = lua_tothread(L, idx);
            break;
        default:
            break;
    }

    return ret;
}

SLW_API slwReturnValue
slwState_get(slwState* slw, const char* name)
{
    SLW_CHECKSTATE(slw);
    lua_State* L = slw->LState;

    lua_getglobal(L, name);
    const int type = lua_type(L, -1);

    // no need to assert, `slwState_type_to_c` already has one and it's all we call.
    return slwState_type_to_c(slw, type, -1);
}

SLW_API slwReturnValue
slwState_getstring(slwState* slw, const char* name)
{
    SLW_CHECKSTATE(slw);
    lua_State* L = slw->LState;
    lua_getglobal(L, name);
    if (lua_isstring(L, -1))
        return (slwReturnValue){.exists = true, .value.s = lua_tostring(L, -1)};

    return (slwReturnValue){.exists = false, .value.b = false};
}

SLW_API slwReturnValue
slwState_getnumber(slwState* slw, const char* name)
{
    SLW_CHECKSTATE(slw);
    lua_State* L = slw->LState;
    lua_getglobal(L, name);
    if (lua_isnumber(L, -1))
        return (slwReturnValue){.exists = true, .value.d = lua_tonumber(L, -1)};

    return (slwReturnValue){.exists = false, .value.b = false};
}

SLW_API slwReturnValue
slwState_getint(slwState* slw, const char* name)
{
    SLW_CHECKSTATE(slw);
    lua_State* L = slw->LState;
    lua_getglobal(L, name);
#if CLW_USING_LUAJIT
    if (lua_isnumber(L, -1))
#else
    if (lua_isinteger(L, -1))
#endif
        return (slwReturnValue){.exists = true, .value.i = lua_tointeger(L, -1)};

    return (slwReturnValue){.exists = false, .value.b = false};
}

SLW_API slwReturnValue
slwState_getbool(slwState* slw, const char* name)
{
    SLW_CHECKSTATE(slw);
    lua_State* L = slw->LState;
    lua_getglobal(L, name);
    if (lua_isboolean(L, -1))
        return (slwReturnValue){.exists = true, .value.i = lua_toboolean(L, -1)};

    return (slwReturnValue){.exists = false, .value.b = false};
}

SLW_API slwReturnValue
slwState_getcfunction(slwState* slw, const char* name)
{
    SLW_CHECKSTATE(slw);
    lua_State* L = slw->LState;
    lua_getglobal(L, name);
    if (lua_iscfunction(L, -1))
        return (slwReturnValue){.exists = true, .value.f = lua_tocfunction(L, -1)};

    return (slwReturnValue){.exists = false, .value.b = false};
}

SLW_API slwReturnValue
slwState_getfunction(slwState* slw, const char* name)
{
    SLW_CHECKSTATE(slw);
    lua_State* L = slw->LState;
    lua_getglobal(L, name);
    if (lua_iscfunction(L, -1))
        return (slwReturnValue){.exists = true, .value.b = false};

    return (slwReturnValue){.exists = false, .value.b = false};
}

SLW_API slwReturnValue
slwState_getuserdata(slwState* slw, const char* name)
{
    SLW_CHECKSTATE(slw);
    lua_State* L = slw->LState;
    lua_getglobal(L, name);
    if (lua_isuserdata(L, -1))
        return (slwReturnValue){.exists = true, .value.u = lua_touserdata(L, -1)};

    return (slwReturnValue){.exists = false, .value.b = false};
}

SLW_API slwReturnValue
slwState_getnil(slwState* slw, const char* name)
{
    SLW_CHECKSTATE(slw);
    lua_State* L = slw->LState;
    lua_getglobal(L, name);
    if (lua_isnil(L, -1))
        return (slwReturnValue){.exists = true, .value.b = true};

    return (slwReturnValue){.exists = false, .value.b = false};
}

SLW_API slwTable*
slwTable_create() {
    return (slwTable*)calloc(1, sizeof(slwTable));
}

// Table Functions
SLW_API slwTable*
slwTable_createkv(slwState* slw, ...)
{
    SLW_CHECKSTATE(slw);

    int numEntries = 0;

    va_list args;
    va_start(args, slw);
    while (1)
    {
        const char* key = va_arg(args, const char*);
        if (key == NULL)
            break;
        
        SLW_ASSERT(va_arg(args, slwTableValue*) != NULL);
        numEntries += 2; // We do an assert below to make sure it's in multiples of 2
    }
    va_end(args);
    SLW_ASSERT(numEntries % 2 == 0);
    va_start(args, slw);

    const size_t tableLen = numEntries / 2;

    slwTable* tbl = (slwTable*)slw_malloc(sizeof(slwTable));
    tbl->elements = (slwTableValue*)slw_malloc(sizeof(slwTableValue) * tableLen);
    tbl->size = tableLen;

    for (int i = 0; i < tableLen; i++)
    {
        const char* key = va_arg(args, const char*);
        slwTableValue* value = va_arg(args, slwTableValue*);

        tbl->elements[i].name = key;
        tbl->elements[i].ltype = value->ltype;
        tbl->elements[i].value = value->value;
    }

    va_end(args);

    return tbl;
}

// Creates an indexed table, instead of Key-Value Pairs
SLW_API slwTable*
slwTable_createi(slwState* slw, ...)
{
    SLW_CHECKSTATE(slw);

    int tableLen = 0;

    // Get length
    va_list args;
    va_start(args, slw);
    while (1)
    {
        if (va_arg(args, slwTableValue*) == NULL)
            break;
        tableLen++;
    }
    va_end(args);
    va_start(args, slw);

    slwTable* tbl = (slwTable*)slw_malloc(sizeof(slwTable));
    tbl->elements = (slwTableValue*)slw_malloc(sizeof(slwTableValue) * tableLen);
    tbl->size = tableLen;

    for (int i = 0; i < tableLen; i++)
    {
        slwTableValue* value = va_arg(args, slwTableValue*);

        tbl->elements[i].name = NULL;
        tbl->elements[i].ltype = value->ltype;
        tbl->elements[i].value = value->value;
    }

    va_end(args);

    return tbl;
}

SLW_API void
slwTable_free(slwTable* slt)
{
    SLW_ASSERT(slt != NULL);

    free(slt->elements);
    slt->elements = NULL;
    slt->size = 0;
    free(slt);
}

SLW_API void
slwTable_push(slwState* slw, slwTable* slt)
{
    SLW_CHECKSTATE(slw);
    SLW_ASSERT(slt != NULL);

    lua_State* L = slw->LState;

    if (slt->size == 0)
    {
        // Maybe we just want to push an empty table.
        lua_createtable(L, 0, 0);
        return;
    }

    // KVP Table
    if (slt->elements[0].name)
    {
        lua_createtable(L, 0, slt->size);
        for (size_t i = 0; i < slt->size; i++)
        {
            slwTableValue el = slt->elements[i];
            lua_pushstring(L, el.name);

            _slwTable_push_value(slw, el);

            lua_settable(L, -3);
        }

        return;
    }

    // Indexed Table
    lua_createtable(L, slt->size, 0);
    for (size_t i = 1; i <= slt->size; ++i)
    {
        slwTableValue el = slt->elements[i-1];
        _slwTable_push_value(slw, el);
        lua_rawseti(L, -2, i);
    }
}

SLW_API slwTable*
slwTable_get_at(slwState* slw, const int32_t idx) // TODO: rename to `get_at` and create another one to call this with -1
{
    SLW_CHECKSTATE(slw);
    lua_State* L = slw->LState;

    if (!lua_istable(L, idx))
        return NULL;

    slwTable* tbl = (slwTable*)slw_malloc(sizeof(slwTable));
    tbl->elements = NULL;

#if LUA_VERSION_NUM > 501
    int tableLen = lua_rawlen(L, idx);
#else
    int tableLen = lua_objlen(L, idx);
#endif

    slwTableValue value;

    // KVP Table
    if (tableLen == 0)
    {
        lua_pushnil(L);

        while (lua_next(L, idx - 1) != 0) {
            if (lua_isstring(L, idx - 1)) {
                value.name = lua_tostring(L, idx - 1);
                const int type = lua_type(L, idx);
                switch (type)
                {
                    case LUA_TSTRING:
                        value.ltype = LUA_TSTRING;
                        value.value.s = lua_tostring(L, idx);
                        break;
                    case LUA_TNUMBER:
                        value.ltype = LUA_TNUMBER;
                        value.value.d = lua_tonumber(L, idx);
                        break;
                    case LUA_TBOOLEAN:
                        value.ltype = LUA_TBOOLEAN;
                        value.value.b = lua_toboolean(L, idx);
                        break;
                    case LUA_TTABLE:
                        value.ltype = LUA_TTABLE;
                        value.value.t = slwTable_get(slw);
                        break;
                    case LUA_TLIGHTUSERDATA:
                        value.ltype = LUA_TLIGHTUSERDATA;
                        value.value.u = lua_touserdata(L, idx); // ?
                        break;
                    case LUA_TFUNCTION:
                        value.ltype = LUA_TFUNCTION;
                        value.value.f = lua_tocfunction(L, idx);
                        break;
                }

                tbl->elements = (slwTableValue*)slw_realloc(tbl->elements, sizeof(slwTableValue) * (tableLen + 1));
                tbl->elements[tableLen++] = value;

                lua_pop(L, 1);
            }
        }
    } 
    // Index Based Table
    else
    {
        tbl->elements = (slwTableValue*)slw_malloc(sizeof(slwTableValue) * tableLen);
        for (size_t i = 1; i <= tableLen; ++i)
        {
            value.name = NULL;

            lua_pushinteger(L, i);
            lua_gettable(L, idx - 1);

            const int type = lua_type(L, idx); // I know, duplication. To be fixed...
            switch (type)
            {
                case LUA_TSTRING:
                    value.ltype = LUA_TSTRING;
                    value.value.s = lua_tostring(L, idx);
                    break;
                case LUA_TNUMBER:
                    value.ltype = LUA_TNUMBER;
                    value.value.d = lua_tonumber(L, idx);
                    break;
                case LUA_TBOOLEAN:
                    value.ltype = LUA_TBOOLEAN;
                    value.value.b = lua_toboolean(L, idx);
                    break;
                case LUA_TTABLE:
                    value.ltype = LUA_TTABLE;
                    value.value.t = slwTable_get(slw);
                    break;
                case LUA_TLIGHTUSERDATA:
                    value.ltype = LUA_TLIGHTUSERDATA;
                    value.value.u = lua_touserdata(L, idx); // ?
                    break;
                case LUA_TFUNCTION:
                    value.ltype = LUA_TFUNCTION;
                    value.value.f = lua_tocfunction(L, idx);
                    break;
            }

            tbl->elements[i-1] = value;
            lua_pop(L, 1);
        }
    }

    tbl->size = tableLen;

    return tbl;
}

SLW_API SLW_INLINE slwTable*
slwTable_get(slwState* slw)
{
    return slwTable_get_at(slw, -1);
}

// TODO: variadic arguments so I can easily do something like: `slwTable_get_from_key(slw, "SOME_GLOBAL2", "nested", "another_nested", "fn")`
SLW_API slwTableValue*
slwTable_getkey(slwTable* slt, const char* key)
{
    SLW_ASSERT(slt != NULL);

    for (size_t i = 0; i < slt->size; i++)
    {
        slwTableValue* el = &slt->elements[i];
        if (!el->name)
            continue;

        if (strcmp(el->name, key) == 0)
            return el;
    }

    return NULL;
}

SLW_API void
slwTable_setval(slwTable* slt, ...)
{
    SLW_ASSERT(slt != NULL);

    va_list args;
    va_start(args, slt);
    const char** keys = NULL;

    slwTableValue* value = NULL;

    // Get keys
    int numKeys = 0;
    while (1) {
        const char* key = va_arg(args, const char*);
        if (*key == '\0')
        {
            value = (slwTableValue*)key;
            break;
        }

        keys = (const char**)slw_realloc(keys, sizeof(const char*) * (numKeys + 1));
        keys[numKeys++] = key;
    }

    va_end(args);

    if (value == NULL)
    {
        slw_free(keys);
        return;
    }

    if (value != NULL) {
        slwTable* currentTable = slt;
        for (int i = 0; i < numKeys - 1; i++) {
            slwTableValue* foundValue = NULL;
            for (size_t j = 0; j < currentTable->size; j++) {
                if (currentTable->elements[j].name != NULL && strcmp(currentTable->elements[j].name, keys[i]) == 0) {
                    foundValue = &currentTable->elements[j];
                    break;
                }
            }

            if (foundValue == NULL) {
                // Create table
                slwTableValue newTableValue;
                newTableValue.name = keys[i];
                newTableValue.ltype = LUA_TTABLE;
                newTableValue.value.t = slwTable_create();

                currentTable->elements = (slwTableValue*)slw_realloc(currentTable->elements, sizeof(slwTableValue) * (currentTable->size + 1));
                currentTable->elements[currentTable->size++] = newTableValue;

                foundValue = &currentTable->elements[currentTable->size - 1];
            }

            SLW_ASSERT(foundValue != NULL && foundValue->ltype == LUA_TTABLE);
            currentTable = foundValue->value.t; // Move to the nested table
        }

        // Now `currentTable` is the parent of the final nested table
        // `keys[numKeys - 1]` is the last key, and `value` is the value to be set
        bool keyExists = false;
        for (size_t j = 0; j < currentTable->size; j++) {
            if (currentTable->elements[j].name != NULL &&
                strcmp(currentTable->elements[j].name, keys[numKeys - 1]) == 0)
            {
                slwTableValue* el = &currentTable->elements[j];
                el->ltype = value->ltype;
                el->value = value->value;
                keyExists = true;
                break;
            }
        }

        if (!keyExists) {
            slwTableValue newValue;
            newValue.name = keys[numKeys - 1];
            newValue.ltype = value->ltype;
            newValue.value = value->value;

            currentTable->elements = (slwTableValue*)slw_realloc(currentTable->elements, sizeof(slwTableValue) * (currentTable->size + 1));
            currentTable->elements[currentTable->size++] = newValue;
        }
    }

    slw_free(keys);
}

// Easier way to create/update tables, call via `slwState_settable2(slw, "my_table", "nested", "name", slwt_tstring("bob"));`
// You can also do this for table structures via `slwTable_setval`, I may change this name.
SLW_API void slwState_settable2(slwState* slw, ...)
{
    SLW_CHECKSTATE(slw);

    va_list args;
    va_start(args, slw);

    slwTableValue* value = NULL;

    // Is this the best way to get the values from the variadic args?
    // Probably not, BUT I can't remember the last time I used it. This somehow works.. where's my medal?
    int numKeys = 0;
    const char* keys[SLW_TABLE_MAX_KEYS];
    while (1) {
        const char* key = va_arg(args, const char*);
        if (*key == '\0') {
            value = (slwTableValue*)key;
            break;
        }
        keys[numKeys++] = key;
    }

    va_end(args);
    SLW_ASSERT(value != NULL);
    SLW_ASSERT(numKeys > 1);

    lua_State* L = slw->LState;

    if (keys[0] == NULL)
    {
        SLW_ASSERT(false);
        return; // to make gcc happy
    }
    lua_getglobal(L, keys[0]);

    if (lua_isnil(L, -1)) {
        lua_pop(L, 1);
        lua_newtable(L);
        lua_pushvalue(L, -1); // Duplicate table
        lua_setglobal(L, keys[0]);
    }

    for (int i = 1; i < numKeys - 1; ++i) {
        lua_pushstring(L, keys[i]);
        lua_gettable(L, -2);

        // Create the table
        if (lua_isnil(L, -1)) {
            lua_pop(L, 1);
            lua_newtable(L);
            lua_pushvalue(L, -1); // Duplicate
            lua_setfield(L, -3, keys[i]);
        }
    }

    lua_pushstring(L, keys[numKeys - 1]);
    _slwTable_push_value(slw, *value);
    lua_settable(L, -3);

    lua_pop(L, numKeys - 1);
}

// Table Set Functions
SLW_API void
slwTable_setstring(slwTable* slt, const char* key, const char* val)
{
    SLW_ASSERT(slt != NULL);
    _slwTable_set_value(slt, key, slwt_tstring(val));
}

SLW_API void
slwTable_setfstring(slwTable* slt, const char* name, const char* fmt, ...)
{
    SLW_ASSERT(slt != NULL);
    SLW_ASSERT(fmt != NULL);

    va_list args;
    va_start(args, fmt);

    size_t length = vsnprintf(NULL, 0, fmt, args);

    if (length < 0) {
        va_end(args);
        return;
    }

    char* formattedString = (char*)slw_malloc(length + 1);
    if (formattedString == NULL) {
        va_end(args);
        return;
    }

    vsnprintf(formattedString, length + 1, fmt, args);
    
    va_end(args);
    _slwTable_set_value(slt, name, slwt_tstring(formattedString));
    
    slw_free(formattedString);
}


SLW_API void
slwTable_setnumber(slwTable* slt, const char* name, double num)
{
    SLW_ASSERT(slt != NULL);
    _slwTable_set_value(slt, name, slwt_tnumber(num));
}

SLW_API void
slwTable_setint(slwTable* slt, const char* name, uint64_t num)
{
    SLW_ASSERT(slt != NULL);
    _slwTable_set_value(slt, name, slwt_tnumber(num)); // Todo: make integer
}

SLW_API void
slwTable_setbool(slwTable* slt, const char* name, bool b)
{
    SLW_ASSERT(slt != NULL);
    _slwTable_set_value(slt, name, slwt_tboolean(b));
}

SLW_API void
slwTable_setcfunction(slwTable* slt, const char* name, lua_CFunction fn)
{
    SLW_ASSERT(slt != NULL);
    _slwTable_set_value(slt, name, slwt_tfunction(fn));
}

SLW_API void
slwTable_setlightudata(slwTable* slt, const char* name, void* data)
{
    SLW_ASSERT(slt != NULL);
    _slwTable_set_value(slt, name, slwt_tlightuserdata(data));
}

SLW_API void
slwTable_settable(slwTable* slt, const char* name, slwTable* tbl)
{
    SLW_ASSERT(slt != NULL);
    _slwTable_set_value(slt, name, slwt_ttable(tbl));
}

SLW_API void
slwTable_setnil(slwTable* slt, const char* name)
{
    SLW_ASSERT(slt != NULL);
    _slwTable_set_value(slt, name, slwt_tnil);
}

// Table Dumping Functions
//------------------------------------------------------------------------
SLW_INTERNAL void
_slwTable_print_value(slwState* slw, slwTableValue value, int depth, int idx)
{
    if (idx != -1)
        printf("%d: ", idx);

    switch (value.ltype)
    {
        case LUA_TSTRING:
            printf("%s\n", value.value.s);
            break;
        case LUA_TNUMBER:
            if (_is_integer(value.value.d))
                printf("%d\n", (int)value.value.d);
            else
                printf("%f\n", value.value.d);
            break;
        case LUA_TBOOLEAN:
            printf("%s\n", value.value.b ? "true" : "false");
            break;
        case LUA_TTABLE:
            printf("table (depth: %d)\n", depth);
            if (depth < SLW_RECURSION_DEPTH) {
                slwTable_dump(slw, value.value.t, NULL, depth + 1);
            } else {
                printf("%*sMax recursion depth reached\n", (depth + 1) * 4, "");
            }
            break;
        case LUA_TLIGHTUSERDATA:
            printf("<luserdata: %p>\n", value.value.u);
            break;
        case LUA_TFUNCTION:
            printf("<function: %p>\n", value.value.u);
            break;
        default:
            printf("unknown type\n");
            break;
    }
}

SLW_API void
slwTable_dump(slwState* slw, slwTable* slt, const char* name, int depth)
{
    SLW_CHECKSTATE(slw);
    SLW_ASSERT(slt != NULL);
    SLW_ASSERT(slt->elements != NULL);

    if (name)
        printf("==== Dumping Table: %s ====\n", name);

    for (size_t i = 0; i < slt->size; i++)
    {
        slwTableValue el = slt->elements[i];
        if (el.name)
        {
            printf("%*s%s = ", depth * 4, "", el.name);
            _slwTable_print_value(slw, el, depth, -1);
        } else
        {
            printf("%*s", depth * 4, "");
            _slwTable_print_value(slw, el, depth, i+1);
        }
    }

    if (name)
        printf("==== Dumping End ====\n\n");
}

SLW_API void
slwTable_dumpg(slwState* slw, const char* name)
{
    SLW_CHECKSTATE(slw);

    lua_getglobal(slw->LState, name);
    slwTable* slt = slwTable_get(slw);
    if (!slt)
    {
        printf("Could not dump table: %s: table does not exist\n", name);
        return;
    }

    slwTable_dump(slw, slt, name, 0);
    slwTable_free(slt);
}