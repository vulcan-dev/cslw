#include "cslw/cslw.h"

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

// So I don't forget:
// Don't add a function to remove something from a table, just set it to nil.

int l_my_function(lua_State* L)
{
#if defined(SLW_GENERICS_SUPPORT)
    slwState* slw = slwState_new(L);
#else
    slwState* slw = slwState_new_from_luas(L);
#endif
    printf("l_my_function called\n");

    slwTable* tbl = slwTable_get_at(slw, -1);
    if (!tbl)
    {
        printf("Did not get table for l_my_function!\n");
    } else
    {
        slwTable_dump(slw, tbl, "arg1 l_my_function", 0);
        slwTable_free(tbl);
    }

    slwState_pushstring(slw, "Return from l_my_function");
    return 1;
}

#if 1
int main(void)
{
    // Create State
#if defined(SLW_GENERICS_SUPPORT)
    slwState* slw = slwState_new(slw_lib_all);
#else
    slwState* slw = slwState_new_with(slw_lib_all);
#endif
    if (!slw)
    {
        fprintf(stderr, "Failed to create state\n");
        slwState_destroy(slw);
        return 1;
    }
    
    // Setup Table
    {
        slwTable slwT;
        memset(&slwT, 0, sizeof(slwTable));

#if defined(SLW_GENERICS_SUPPORT)
        slwTable_set(&slwT, "myFunction", l_my_function);
#else
        slwTable_setcfunction(&slwT, "myFunction", l_my_function);
#endif
        slwState_settable(slw, "slwt", &slwT);
    }

    // Run Lua File
    slwState_runfile(slw, "../lua/scripts/main.lua");

    // Call Function
    lua_getglobal(slw->LState, "callMe");
    if (!slwState_call_fn_at(slw, -1,
            slwt_tstring("Hello"),
            slwt_tstring("world"),
            slwt_tnumber(420),
            NULL))
    {
        printf("Failed calling: callMe: %s\n", lua_tostring(slw->LState, -1));
    } else
    {
        slwTable* t = slwTable_get_at(slw, -1);
        if (t)
        {
            slwTable_dump(slw, t, "return callMe", 0);
            slwTable_free(t);
        }
    }

    // Cleanup
    slwState_destroy(slw);
}
#else
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

        slwTable_free(user);
        slwTable_free(tbl);
    }

    { // Test table 2
        slwTable* tbl2 = slwTable_create();
        slwTable_setval(tbl2, "name", slwt_tstring("bob")); // Can call like `slwTable_setval(tbl2, "key1", "key2", "key3", "last_key", slwt_tstring("myValue"))`
        slwTable_setstring(tbl2, "name", "dan"); // overwrites bob or creates it hasn't been created already
        slwTable_set(tbl2, "lang", "c"); // `slwTable_set` is a _Generic (>= C11)
        slwTable_setint(tbl2, "num", 42);
        slwTable_setcfunction(tbl2, "fn", l_my_function);

        slwTable* tbl = slwTable_create();
        slwTable_setval(tbl, "users", "user1", slwt_ttable(tbl2));
        slwTable_setval(tbl, "users", "user1", "age", slwt_tnumber(420));
        slwTable_dump(slw, tbl, 0);

        slwTable_free(tbl2);
        slwTable_free(tbl);
    }

    { // Text indexed table
        slwTable* tbl = slwTable_createi(slw,
                                         slwt_tstring("string1"),
                                         slwt_tnumber(4.20),
                                         NULL
                                         );

        slwState_set(slw, "MY_INDEXED_TABLE", tbl);
        slwTable_free(tbl);
    }

    slwState_set(slw, "my_func", (lua_CFunction)l_my_function);
    slwState_settable2(slw, "SOME_GLOBAL1", "User0", "Name", slwt_tstring("Bob"));
    slwState_settable2(slw, "SOME_GLOBAL2", "nested", "str", slwt_tstring("Ello mate!"));
    slwState_settable2(slw, "SOME_GLOBAL2", "nested", "num", slwt_tnumber(420));
    slwState_settable2(slw, "SOME_GLOBAL2", "nested", "another_nested", "name", slwt_tstring("Bob"));

    slwState_settable2(slw, "FUNC_TABLE", "fn", slwt_tfunction(l_my_function));

    if (slwState_runfile(slw, "../lua/scripts/main.lua") != 0)
    {
        printf("failed running script file: %s\n", lua_tostring(slw->LState, -1));
        goto cleanup;
    }

    slwTable_dumpg(slw, "INDEX_TABLE");
    slwTable_dumpg(slw, "USER_TABLE");

    slwTable_dumpg(slw, "SOME_GLOBAL1");
    slwTable_dumpg(slw, "SOME_GLOBAL2");
    
    // Cleanup
    cleanup:
        slwState_destroy(slw);
    return 0;
}
#endif