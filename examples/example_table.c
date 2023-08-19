#include "cslw/cslw.h"

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

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

    slwStack_pushstring(slw, "Return from l_my_function");
    return 1;
}

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
    if (!slwState_runfile(slw, "../lua/scripts/main.lua"))
    {
        slwState_destroy(slw);
        return 1;
    }

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