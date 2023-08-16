#ifndef LUA_CODE_H
#define LUA_CODE_H

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

print("SOME_GLOBAL2 = " .. tostring(SOME_GLOBAL2))

    if SOME_GLOBAL1 then
        printf("SOME_GLOBAL1")
        for k, v in pairs(SOME_GLOBAL1) do
            printf(k, v)
        end
    end

print("==== LUA END ====")
io.write("\n")
);

#endif