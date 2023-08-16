#define SLW_ENABLE_ASSERTIONS
#define SLW_TABLE_MAX_KEYS 32

#include "cslw/cslw.h"
#include <stdlib.h>
#include <assert.h>
#include <memory.h>
#include <string.h>

// Internal Functions (Mainly helper functions)
//------------------------------------------------------------------------
slw_internal void
_slwTable_push_value(slwState* slw, slwTableValue_t el)
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

slw_internal void
_slwTable_set_value(slwTable* slt, const char* key, slwTableValue_t val)
{
    slwTableValue_t* sltTable = slwTable_get_from_key(slt, key);
    if (sltTable)
    {
        sltTable->value = val.value;
        sltTable->ltype = val.ltype;
        return;
    }

    val.name = key;
    slt->elements = (slwTableValue_t*)slw_realloc(slt->elements, sizeof(slwTableValue_t) * (slt->size + 1));
    slt->elements[slt->size++] = val;
}

slw_internal void
_slwTable_print_value(slwState* slw, slwTableValue_t value, int depth, int idx)
{
    if (idx != -1)
        printf("%d: ", idx);

    switch (value.ltype)
    {
        case LUA_TSTRING:
            printf("%s\n", value.value.s);
            break;
        case LUA_TNUMBER:
            printf("%f\n", value.value.d);
            break;
        case LUA_TBOOLEAN:
            printf("%s\n", value.value.b ? "true" : "false");
            break;
        case LUA_TTABLE:
            printf("table (depth: %d)\n", depth);
            if (depth < SLW_RECURSION_DEPTH) {
                slwTable_dump(slw, value.value.t, depth + 1);
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
slwState_setint(slwState* slw, const char* name, uint64_t num)
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

SLW_API slwTable*
slwTable_create() {
    return (slwTable*)calloc(1, sizeof(slwTable));
}

// Table Functions
SLW_API slwTable*
slwTable_createkv(slwState* slw, ...)
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

// Creates an indexed table, instead of Key-Value Pairs
SLW_API slwTable*
slwTable_createi(slwState* slw, ...)
{
    slw_assert(slw != NULL);

    int tableLen = 0;

    // Get length
    va_list args;
    va_start(args, slw);
    while (1)
    {
        if (va_arg(args, slwTableValue_t*) == NULL)
            break;
        tableLen++;
    }
    va_end(args);
    va_start(args, slw);

    slwTable* tbl = (slwTable*)slw_malloc(sizeof(slwTable));
    tbl->elements = (slwTableValue_t*)slw_malloc(sizeof(slwTableValue_t) * tableLen);
    tbl->size = tableLen;

    for (int i = 0; i < tableLen; i++)
    {
        slwTableValue_t* value = va_arg(args, slwTableValue_t*);

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

    if (slt->size == 0)
        return;

    lua_State* L = slw->LState;

    // KVP Table
    if (slt->elements[0].name)
    {
        lua_createtable(L, 0, slt->size);
        for (size_t i = 0; i < slt->size; i++)
        {
            slwTableValue_t el = slt->elements[i];
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
        slwTableValue_t el = slt->elements[i-1];
        _slwTable_push_value(slw, el);
        lua_rawseti(L, -2, i);
    }
}

SLW_API slwTable*
slwTable_get_at(slwState* slw, const int32_t idx) // TODO: rename to `get_at` and create another one to call this with -1
{
    slw_assert(slw != NULL);
    lua_State* L = slw->LState;

    if (!lua_istable(L, idx))
        return NULL;

    slwTable* tbl = (slwTable*)slw_malloc(sizeof(slwTable));
    tbl->elements = NULL;

    int tableLen = lua_rawlen(L, idx);
    slwTableValue_t value;

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

SLW_API slwTable* slwTable_get(slwState* slw)
{
    return slwTable_get_at(slw, -1);
}

// TODO: variadic arguments so I can easily do something like: `slwTable_get_from_key(slw, "SOME_GLOBAL2", "nested", "another_nested", "fn")`
SLW_API slwTableValue_t*
slwTable_get_from_key(slwTable* slt, const char* key)
{
    slw_assert(slt != NULL);
    for (size_t i = 0; i < slt->size; i++)
    {
        slwTableValue_t* el = &slt->elements[i];
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
    slw_assert(slt != NULL);

    va_list args;
    va_start(args, slt);
    const char** keys = NULL;

    slwTableValue_t* value = NULL;

    // Get keys
    int numKeys = 0;
    while (1) {
        const char* key = va_arg(args, const char*);
        if (*key == '\0')
        {
            value = (slwTableValue_t*)key;
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
            slwTableValue_t* foundValue = NULL;
            for (size_t j = 0; j < currentTable->size; j++) {
                if (currentTable->elements[j].name != NULL && strcmp(currentTable->elements[j].name, keys[i]) == 0) {
                    foundValue = &currentTable->elements[j];
                    break;
                }
            }

            if (foundValue == NULL) {
                // Create table
                slwTableValue_t newTableValue;
                newTableValue.name = keys[i];
                newTableValue.ltype = LUA_TTABLE;
                newTableValue.value.t = slwTable_create();

                currentTable->elements = (slwTableValue_t*)slw_realloc(currentTable->elements, sizeof(slwTableValue_t) * (currentTable->size + 1));
                currentTable->elements[currentTable->size++] = newTableValue;

                foundValue = &currentTable->elements[currentTable->size - 1];
            }

            slw_assert(foundValue != NULL && foundValue->ltype == LUA_TTABLE);
            currentTable = foundValue->value.t; // Move to the nested table
        }

        // Now `currentTable` is the parent of the final nested table
        // `keys[numKeys - 1]` is the last key, and `value` is the value to be set
        bool keyExists = false;
        for (size_t j = 0; j < currentTable->size; j++) {
            if (currentTable->elements[j].name != NULL &&
                strcmp(currentTable->elements[j].name, keys[numKeys - 1]) == 0)
            {
                slwTableValue_t* el = &currentTable->elements[j];
                el->ltype = value->ltype;
                el->value = value->value;
                keyExists = true;
                break;
            }
        }

        if (!keyExists) {
            slwTableValue_t newValue;
            newValue.name = keys[numKeys - 1];
            newValue.ltype = value->ltype;
            newValue.value = value->value;

            currentTable->elements = (slwTableValue_t*)slw_realloc(currentTable->elements, sizeof(slwTableValue_t) * (currentTable->size + 1));
            currentTable->elements[currentTable->size++] = newValue;
        }
    }

    slw_free(keys);
}

// Easier way to create/update tables, call via `slwState_settable2(slw, "my_table", "nested", "name", slwt_tstring("bob"));`
// You can also do this for table structures via `slwTable_setval`, I may change this name.
SLW_API void slwState_settable2(slwState* slw, ...)
{
    slw_assert(slw != NULL);

    va_list args;
    va_start(args, slw);

    slwTableValue_t* value = NULL;

    // Is this the best way to get the values from the variadic args?
    // Probably not, BUT I can't remember the last time I used it. This somehow works.. where's my medal?
    int numKeys = 0;
    const char* keys[SLW_TABLE_MAX_KEYS];
    while (1) {
        const char* key = va_arg(args, const char*);
        if (*key == '\0') {
            value = (slwTableValue_t*)key;
            break;
        }
        keys[numKeys++] = key;
    }

    va_end(args);
    slw_assert(value != NULL);
    slw_assert(numKeys > 1);

    lua_State* L = slw->LState;

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
    slw_assert(slt != NULL);
    _slwTable_set_value(slt, key, slwt_tstring(val));
}

SLW_API void
slwTable_setfstring(slwTable* slt, const char* name, const char* fmt, ...)
{
    slw_assert(slt != NULL);
    slw_assert(fmt != NULL);

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
    slw_assert(slt != NULL);
    _slwTable_set_value(slt, name, slwt_tnumber(num));
}

SLW_API void
slwTable_setint(slwTable* slt, const char* name, uint64_t num)
{
    slw_assert(slt != NULL);
    _slwTable_set_value(slt, name, slwt_tnumber(num)); // Todo: make integer
}

SLW_API void
slwTable_setbool(slwTable* slt, const char* name, bool b)
{
    slw_assert(slt != NULL);
    _slwTable_set_value(slt, name, slwt_tboolean(b));
}

SLW_API void
slwTable_setcfunction(slwTable* slt, const char* name, lua_CFunction fn)
{
    slw_assert(slt != NULL);
    _slwTable_set_value(slt, name, slwt_tfunction(fn));
}

SLW_API void
slwTable_setlightudata(slwTable* slt, const char* name, void* data)
{
    slw_assert(slt != NULL);
    _slwTable_set_value(slt, name, slwt_tlightuserdata(data));
}

SLW_API void
slwTable_settable(slwTable* slt, const char* name, slwTable* tbl)
{
    slw_assert(slt != NULL);
    _slwTable_set_value(slt, name, slwt_ttable(tbl));
}

SLW_API void
slwTable_setnil(slwTable* slt, const char* name)
{
    slw_assert(slt != NULL);
    _slwTable_set_value(slt, name, slwt_tnil);
}

SLW_API void
slwTable_dump(slwState* slw, slwTable* slt, int depth)
{
    slw_assert(slw != NULL);
    slw_assert(slt != NULL);

    for (size_t i = 0; i < slt->size; i++)
    {
        slwTableValue_t el = slt->elements[i];
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
}

SLW_API void
slwTable_dumpg(slwState* slw, const char* name)
{
    slw_assert(slw != NULL);

    printf("==== Dumping Table: %s ====\n", name);

    lua_getglobal(slw->LState, name);
    slwTable* slt = slwTable_get(slw);
    if (slt)
        slwTable_dump(slw, slt, 0);
    else
        printf("Could not find table\n");
    slwTable_free(slt);

    printf("==== Dumping End ====\n\n");
}