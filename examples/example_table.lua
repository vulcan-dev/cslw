print(slwt.myFunction({
    text = "Hello World"
}))

function callMe(...)
    local arg = {...}
    io.write("callMe has been called")
    if #arg > 0 then
        io.write(" with:\n")
        for _, k in ipairs(arg) do
            io.write("    ")
            print(k)
        end
    end
    io.write('\n')

    return {
        myInt = 4,
        intDouble = 32.1,
        myStr = "Hello"
    }
end