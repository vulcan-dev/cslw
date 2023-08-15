#define SLW_ENABLE_ASSERTIONS
#include <cslw/cslw.h>

#include <stdio.h>
#include <stdlib.h>

#define mlstring(...) #__VA_ARGS__
const char* lua_code = mlstring(
    print("==== LUA START ====")

    for i, v in ipairs(MY_INDEXED_TABLE) do
        print("MY_INDEXED_TABLE", i, v)
    end

    io.write("\n")

    for k, v in pairs(MY_TABLE) do
        if type(v) == "table" then
            print(k)
            for k1, v1 in pairs(v) do
            print(k1, v1)
            end
        else
            print(k, v)
        end
    end

    SOME_GLOBAL = "Hello World!";

    USER_TABLE = {
        ["user0"] = {
            ["name"] = "dan",
            ["skills"] = {
                "skill1",
                "skill2",
                "skill3",
                "skill4",
                {
                    "please",
                    "work"
                }
            }
        }
    }

    INDEX_TABLE = {
        0,
        1,
        2,
        3,
        4,
        5
    }

    NAME_LIST = {
        "john",
        "marie",
        "doe"
    }

    print("==== LUA END ====")
    io.write("\n")
);

int l_my_function(lua_State* L)
{
    slwState* slw = slwState_new(L);
    slwState_set(slw, "A", 42);
    return 0;
}

// These `print_xxx` functions are all temporary, purely for debugging.
// Maybe I'll add a `slwTable_dump(x)` function. Good idea actually.. Maybe dump and dumps (the s for string)
void print_value(slwState* L, slwTableValue_t value, int depth, int idx);
void print_table(slwState* L, slwTable* tbl, int depth)
{
    for (size_t i = 0; i < tbl->size; i++)
    {
        slwTableValue_t el = tbl->elements[i];
        if (el.name)
        {
            printf("%*s%s = ", depth * 4, "", el.name);
            print_value(L, el, depth, -1);
        } else
        {
            printf("%*s", depth * 4, "");
            print_value(L, el, depth, i+1);
        }
    }
}

void print_value(slwState* L, slwTableValue_t value, int depth, int idx)
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
                print_table(L, value.value.t, depth + 1);
            } else {
                printf("%*sMax recursion depth reached\n", (depth + 1) * 4, "");
            }
            break;
        default:
            printf("unknown type\n");
            break;
    }
}

int main(void)
{
    slwState* slw = slwState_new(slw_lib_all);
    if (!slw)
    {
        fprintf(stderr, "Failed to create state\n");
        slwState_destroy(slw);
        return 1;
    }

    { // Test table
        slwTable* user = slwTable_createkv(slw,
            "name", slwt_tstring("dan"),
            NULL
        );

        slwTable* tbl = slwTable_createkv(slw,
            "C_GLOBAL_STRING", slwt_tstring("Hello"),
            "C_GLOBAL_NUMBER", slwt_tnumber(4.20),
            "user1",           slwt_ttable(user),
            NULL
        );

        slwState_set(slw, "MY_TABLE", tbl);
        slwTable_free(tbl);
    }

    { // Text indexed table
        slwTable* tbl = slwTable_createi(slw,
            slwt_tstring("string1"),
            slwt_tnumber(4.20),
            NULL);

        slwState_set(slw, "MY_INDEXED_TABLE", tbl);
        slwTable_free(tbl);
    }

    slwState_set(slw, "my_func", (lua_CFunction)l_my_function);
    slwState_runstring(slw, lua_code);

    {
        lua_State* L = slw->LState;

        if (lua_getglobal(L, "INDEX_TABLE") == LUA_TTABLE)
        {
            slwTable* tbl = slwTable_get(slw);
            print_table(slw, tbl, 0);
            slwTable_free(tbl);
        }

        if (lua_getglobal(L, "USER_TABLE") == LUA_TTABLE)
        {
            slwTable* users = slwTable_get(slw);
            print_table(slw, users, 0);
            slwTable_free(users);
        }
    }

    printf("SOME_GLOBAL: %s\n", slwState_get(slw, "SOME_GLOBAL").s);
    
    // Cleanup
    slwState_destroy(slw);
    return 0;
}