os.setlocale("C")

local output = {}
local function writef(...)
    output[#output + 1] = string.format(...)
end
local function printf(...)
    io.stderr:write(string.format(...))
end

local bytes
do
    local inp = io.open("../build/hrir/kemar.bin", "rb")
    bytes = inp:read("a")
    inp:close()
end

do
    writef("#include <stddef.h>\n")
    writef("const size_t        binaural_hrir_kemar_bin_size    = %d;\n", #bytes)
    writef("const unsigned char binaural_hrir_kemar_bin_bytes[] = {\n    ")
    for j = 1, #bytes do
        if j > 1 and (j - 1) % 20 == 0 then
            writef("\n    ")
        end
        local b = bytes:byte(j)
        writef("0x%02x", b)
        if j < #bytes then
            writef(", ")
        end
    end
    writef("\n};\n")
end
local out = io.open("hrir_kemar_bin.c", "w")
out:write(table.concat(output))
out:close()

