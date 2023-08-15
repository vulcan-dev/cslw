#define SLW_ENABLE_ASSERTIONS 1
#include <cslw/cslw.h>

#include <stdio.h>

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

    slwState_set(slw, "my_func", (lua_CFunction)l_my_function);
    slwState_set(slw, "my_string", "Hello World!");
    slwState_runstring(slw, "my_func()");

    printf("my_string: %s\n", slwState_get(slw, "my_string").s);
    
    // Cleanup
    slwState_destroy(slw);
    return 0;
}