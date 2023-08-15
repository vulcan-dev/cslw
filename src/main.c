#define SLW_ENABLE_ASSERTIONS
#include <cslw/cslw.h>

#include <stdio.h>
#include <stdlib.h>

#define mlstring(...) #__VA_ARGS__
const char* lua_code = mlstring(
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

    HASH_TABLE =
    {
        a = 1,
        b = 2,
        c = "Hello World"
    }

    NAME_LIST =
    {
        "john",
        "marie",
        "doe"
    }
);

int l_my_function(lua_State* L)
{
    slwState* slw = slwState_new(L);
    slwState_set(slw, "A", 42);
    return 0;
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

    {
        slwTable* user = slwTable_create(slw,
            "name", slwt_tstring("dan"),
            NULL
        );

        slwTable* tbl = slwTable_create(slw,
            "C_GLOBAL_STRING", slwt_tstring("Hello"),
            "C_GLOBAL_NUMBER", slwt_tnumber(4.20),
            "user1", slwt_ttable(user),
            NULL
        );

        slwTable_push(slw, tbl);
        lua_setglobal(slw->LState, "MY_TABLE");
        slw_free(tbl);
    }

    slwState_set(slw, "my_func", (lua_CFunction)l_my_function);
    slwState_runstring(slw, lua_code);

    printf("SOME_GLOBAL: %s\n", slwState_get(slw, "SOME_GLOBAL").s);
    
    // Cleanup
    slwState_destroy(slw);
    return 0;
}