#define SLW_ENABLE_ASSERTIONS
#include "cslw/cslw.h"
#include <stdlib.h>
#include <assert.h>
#include <memory.h>

// Functions
//------------------------------------------------------------------------
// Primary Functions
SLW_API void
slwState_destroy(slwState* slw)
{
    slw_assert(slw != NULL);
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
    slw_assert(slw != NULL);
    slw_assert(slw != NULL);

    slwState* newSLW = (slwState*)slw_malloc(sizeof(slwState));
    if (!newSLW)
        return NULL;

    newSLW->LState = slw->LState;

    return newSLW;
}

SLW_API slwState* slwState_new_from_luas(lua_State* L)
{
    slw_assert(L == NULL);

    slwState* slw = (slwState*)slw_malloc(sizeof(slwState));
    if (!slw)
        return NULL;

    slw->LState = L;

    return slw;
}

SLW_API void
slwState_close(slwState* slw)
{
    slw_assert(slw != NULL);
    lua_close(slw->LState);
    slw->LState = NULL;
}

SLW_API void
slwState_openlibraries(slwState* slw, const uint32_t libs)
{
    slw_assert(slw != NULL);

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
    if (libs & slw_lib_coroutine)
        slwState_openlib(slw, "coroutine", luaopen_coroutine);
    if (libs & slw_lib_os)
        slwState_openlib(slw, "os", luaopen_os);
    if (libs & slw_lib_utf8)
        slwState_openlib(slw, "utf8", luaopen_utf8);
#if __cslw_bit32_manual
    if (libs && slw_lib_bit32)
        slwState_openlib(slw, "bit32", luaopen_bit32);
#endif
}

SLW_API void
slwState_openlib(slwState* slw, const char* name, lua_CFunction func)
{
    luaL_requiref(slw->LState, name, func, 1);
    lua_pop(slw->LState, 1);
}

SLW_API int slwState_runstring(slwState* slw, const char* str)
{
    slw_assert(slw != NULL);
    lua_State* L = slw->LState;
    return luaL_loadstring(L, str) || lua_pcall(L, 0, LUA_MULTRET, 0);
}

SLW_API int slwState_runfile(slwState* slw, const char* filename)
{
    slw_assert(slw != NULL);
    lua_State* L = slw->LState;
    return luaL_loadfile(L, filename) || lua_pcall(L, 0, LUA_MULTRET, 0);
}

// Push Functions
//------------------------------------------------------------------------
SLW_API void slwState_pushstring(slwState* slw, const char* str)
{
    slw_assert(slw != NULL);
    lua_pushstring(slw->LState, str);
}

SLW_API const char* slwState_pushfstring(slwState* slw, const char* fmt, ...)
{
    slw_assert(slw != NULL);

    va_list args;
    va_start(args, fmt);

    const char* result = lua_pushvfstring(slw->LState, fmt, args);
    va_end(args);
    return result;
}

SLW_API void slwState_pushnumber(slwState* slw, double num)
{
    slw_assert(slw != NULL);
    lua_pushnumber(slw->LState, num);
}

SLW_API void slwState_pushint(slwState* slw, int64_t num)
{
    slw_assert(slw != NULL);
    lua_pushinteger(slw->LState, num);
}

SLW_API void slwState_pushbool(slwState* slw, bool b)
{
    slw_assert(slw != NULL);
    lua_pushboolean(slw->LState, b);
}

SLW_API void slwState_pushcfunction(slwState* slw, lua_CFunction fn)
{
    slw_assert(slw != NULL);
    lua_pushcfunction(slw->LState, fn);
}

SLW_API void slwState_pushlightudata(slwState* slw, void* data)
{
    slw_assert(slw != NULL);
    lua_pushlightuserdata(slw->LState, data);
}

SLW_API void slwState_pushcclosure(slwState* slw, lua_CFunction fn, int n)
{
    slw_assert(slw != NULL);
    lua_pushcclosure(slw->LState, fn, n);
}

SLW_API void slwState_pushnil(slwState* slw)
{
    slw_assert(slw != NULL);
    lua_pushnil(slw->LState);
}

// Set Functions (Globals)
//------------------------------------------------------------------------
SLW_API void
slwState_setstring(slwState* slw, const char* name, const char* str)
{
    slw_assert(slw != NULL);
    lua_pushstring(slw->LState, str);

    lua_setglobal(slw->LState, name);
}

SLW_API const char*
slwset_setfstring(slwState* slw, const char* name, const char* fmt, ...)
{
    slw_assert(slw != NULL);

    va_list args;
    va_start(args, fmt);

    const char* result = lua_pushvfstring(slw->LState, fmt, args);
    va_end(args);

    lua_setglobal(slw->LState, name);
    return result;
}

SLW_API void
slwState_setnumber(slwState* slw, const char* name, double num)
{
    slw_assert(slw != NULL);
    lua_pushnumber(slw->LState, num);

    lua_setglobal(slw->LState, name);
}

SLW_API void
slwState_setint(slwState* slw, const char* name, int64_t num)
{
    slw_assert(slw != NULL);
    lua_pushinteger(slw->LState, num);

    lua_setglobal(slw->LState, name);
}

SLW_API void
slwState_setbool(slwState* slw, const char* name, bool b)
{
    slw_assert(slw != NULL);
    lua_pushboolean(slw->LState, b);

    lua_setglobal(slw->LState, name);
}

SLW_API void
slwState_setcfunction(slwState* slw, const char* name, lua_CFunction fn)
{
    slw_assert(slw != NULL);
    lua_pushcfunction(slw->LState, fn);

    lua_setglobal(slw->LState, name);
}

SLW_API void
slwState_setlightudata(slwState* slw, const char* name, void* data)
{
    slw_assert(slw != NULL);
    lua_pushlightuserdata(slw->LState, data);

    lua_setglobal(slw->LState, name);
}

SLW_API void
slwState_setcclosure(slwState* slw, const char* name, lua_CFunction fn, int n)
{
    slw_assert(slw != NULL);
    lua_pushcclosure(slw->LState, fn, n);

    lua_setglobal(slw->LState, name);
}

SLW_API void
slwState_settable(slwState* slw, const char* name, slwTable* slt)
{
    slw_assert(slw != NULL);
    slw_assert(slt != NULL);

    slwTable_push(slw, slt);
    lua_setglobal(slw->LState, name);
}

SLW_API void
slwState_setnil(slwState* slw, const char* name)
{
    slw_assert(slw != NULL);
    lua_pushnil(slw->LState);

    lua_setglobal(slw->LState, name);
}

// Get Functions (Globals)
//------------------------------------------------------------------------
SLW_API slwReturnValue
slwState_type_to_c(slwState* slw, const int type, const int idx)
{
    slw_assert(slw != NULL);
    slwReturnValue ret;
    ret.exists = true;

    lua_State* L = slw->LState;

    switch(type)
    {
        case LUA_TNONE:
            ret.exists = false;
            return ret;
        case LUA_TBOOLEAN:
            ret.b = lua_toboolean(L, idx);
            break;
        case LUA_TNUMBER:
            ret.n = lua_tonumber(L, idx);
            break;
        case LUA_TSTRING:
            ret.s = lua_tostring(L, idx);
            break;
        case LUA_TTABLE:
            slw_assert(false); // todo
            break;
        case LUA_TFUNCTION:
            ret.cfn = lua_tocfunction(L, idx);
            break;
        case LUA_TUSERDATA:
            ret.cfn = lua_touserdata(L, idx);
            break;
        case LUA_TTHREAD:
            ret.t = lua_tothread(L, idx);
            break;
        default:
            break;
    }

    return ret;
}

SLW_API slwReturnValue
slwState_getstring(slwState* slw, const char* name)
{
    slw_assert(slw != NULL);
    lua_State* L = slw->LState;
    lua_getglobal(L, name);
    if (lua_isstring(L, -1))
        return (slwReturnValue){.exists = true, .s = lua_tostring(L, -1)};

    return (slwReturnValue){.exists = false, .b = false};
}

SLW_API slwReturnValue
slwState_getnumber(slwState* slw, const char* name)
{
    slw_assert(slw != NULL);
    lua_State* L = slw->LState;
    lua_getglobal(L, name);
    if (lua_isnumber(L, -1))
        return (slwReturnValue){.exists = true, .n = lua_tonumber(L, -1)};

    return (slwReturnValue){.exists = false, .b = false};
}

SLW_API slwReturnValue
slwState_getint(slwState* slw, const char* name)
{
    slw_assert(slw != NULL);
    lua_State* L = slw->LState;
    lua_getglobal(L, name);
    if (lua_isinteger(L, -1))
        return (slwReturnValue){.exists = true, .i = lua_tointeger(L, -1)};

    return (slwReturnValue){.exists = false, .b = false};
}

SLW_API slwReturnValue
slwState_getbool(slwState* slw, const char* name)
{
    slw_assert(slw != NULL);
    lua_State* L = slw->LState;
    lua_getglobal(L, name);
    if (lua_isboolean(L, -1))
        return (slwReturnValue){.exists = true, .i = lua_toboolean(L, -1)};

    return (slwReturnValue){.exists = false, .b = false};
}

SLW_API slwReturnValue
slwState_getcfunction(slwState* slw, const char* name)
{
    slw_assert(slw != NULL);
    lua_State* L = slw->LState;
    lua_getglobal(L, name);
    if (lua_iscfunction(L, -1))
        return (slwReturnValue){.exists = true, .cfn = lua_tocfunction(L, -1)};

    return (slwReturnValue){.exists = false, .b = false};
}

SLW_API slwReturnValue
slwState_getfunction(slwState* slw, const char* name)
{
    slw_assert(slw != NULL);
    lua_State* L = slw->LState;
    lua_getglobal(L, name);
    if (lua_iscfunction(L, -1))
        return (slwReturnValue){.exists = true, .b = false};

    return (slwReturnValue){.exists = false, .b = false};
}

SLW_API slwReturnValue
slwState_getuserdata(slwState* slw, const char* name)
{
    slw_assert(slw != NULL);
    lua_State* L = slw->LState;
    lua_getglobal(L, name);
    if (lua_isuserdata(L, -1))
        return (slwReturnValue){.exists = true, .udata = lua_touserdata(L, -1)};

    return (slwReturnValue){.exists = false, .b = false};
}

SLW_API slwReturnValue
slwState_getnil(slwState* slw, const char* name)
{
    slw_assert(slw != NULL);
    lua_State* L = slw->LState;
    lua_getglobal(L, name);
    if (lua_isnil(L, -1))
        return (slwReturnValue){.exists = true, .b = true};

    return (slwReturnValue){.exists = false, .b = false};
}

// Table Functions
SLW_API slwTable*
slwTable_create(slwState* slw, ...)
{
    slw_assert(slw != NULL);

    int numEntries = 0;

    va_list args;
    va_start(args, slw);
    while (1)
    {
        const char* key = va_arg(args, const char*);
        if (key == NULL)
            break;
        
        slw_assert(va_arg(args, slwTableValue_t*) != NULL);
        numEntries += 2; // We do an assert below to make sure it's in multiples of 2
    }
    va_end(args);
    slw_assert(numEntries % 2 == 0);
    va_start(args, slw);

    const size_t tableLen = numEntries / 2;

    slwTable* tbl = (slwTable*)slw_malloc(sizeof(slwTable));
    tbl->elements = (slwTableValue_t*)slw_malloc(sizeof(slwTableValue_t) * tableLen);
    tbl->size = tableLen;

    for (int i = 0; i < tableLen; i++)
    {
        const char* key = va_arg(args, const char*);
        slwTableValue_t* value = va_arg(args, slwTableValue_t*);

        tbl->elements[i].name = key;
        tbl->elements[i].ltype = value->ltype;
        tbl->elements[i].value = value->value;
    }

    va_end(args);

    return tbl;
}

SLW_API void
slwTable_free(slwTable* slt)
{
    slw_assert(slt != NULL);

    free(slt->elements);
    slt->elements = NULL;
    slt->size = 0;
    free(slt);
}

SLW_API void
slwTable_push(slwState* slw, slwTable* slt)
{
    slw_assert(slw != NULL);
    slw_assert(slt != NULL);
    lua_State* L = slw->LState;

    lua_createtable(L, 0, slt->size);
    for (size_t i = 0; i < slt->size; i++)
    {
        slwTableValue_t el = slt->elements[i];
        lua_pushstring(L, el.name);
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
            default:
                slw_assert(false);
                break;
        }

        lua_settable(L, -3);
    }
}

SLW_API slwTable*
slwTable_get(slwState* slw)
{
    slw_assert(slw != NULL);
    lua_State* L = slw->LState;

    if (!lua_istable(L, -1))
        return NULL;

    slwTable* tbl = (slwTable*)slw_malloc(sizeof(slwTable));
    tbl->elements = NULL;

    int tableLen = lua_rawlen(L, -1);
    slwTableValue_t value;

    // KVP Table
    if (tableLen == 0)
    {
        lua_pushnil(L);

        while (lua_next(L, -2) != 0) {
            if (lua_isstring(L, -2)) {
                value.name = lua_tostring(L, -2);
                if (lua_isstring(L, -1)) {
                    value.ltype = LUA_TSTRING;
                    value.value.s = lua_tostring(L, -1);
                } else if (lua_isnumber(L, -1)) {
                    value.ltype = LUA_TNUMBER;
                    value.value.d = lua_tonumber(L, -1);
                } else if (lua_isboolean(L, -1)) {
                    value.ltype = LUA_TBOOLEAN;
                    value.value.b = lua_toboolean(L, -1);
                } else if (lua_istable(L, -1)) {
                    value.ltype = LUA_TTABLE;
                    value.value.t = slwTable_get(slw);
                }

                tbl->elements = (slwTableValue_t*)slw_realloc(tbl->elements, sizeof(slwTableValue_t) * (tableLen + 1));
                tbl->elements[tableLen++] = value;

                lua_pop(L, 1);
            }
        }
    } 
    // Index Based Table
    else
    {
        tbl->elements = (slwTableValue_t*)slw_malloc(sizeof(slwTableValue_t) * tableLen);
        for (size_t i = 1; i <= tableLen; ++i)
        {
            value.name = NULL;

            lua_pushinteger(L, i);
            lua_gettable(L, -2);

            const int type = lua_type(L, -1);
            switch (type)
            {
                case LUA_TSTRING:
                    value.ltype = LUA_TSTRING;
                    value.value.s = lua_tostring(L, -1);
                    break;
                case LUA_TNUMBER:
                    value.ltype = LUA_TNUMBER;
                    value.value.d = lua_tonumber(L, -1);
                    break;
                case LUA_TBOOLEAN:
                    value.ltype = LUA_TBOOLEAN;
                    value.value.b = lua_toboolean(L, -1);
                    break;
                case LUA_TTABLE:
                    value.ltype = LUA_TTABLE;
                    value.value.t = slwTable_get(slw);
                    break;
            }

            tbl->elements[i-1] = value;
            lua_pop(L, 1);
        }
    }

    tbl->size = tableLen;

    return tbl;
}