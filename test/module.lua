local t = require "test"
local ecs = require "ecs"
local u = require "util"

u.test_defaults()

local m = require "modules.test"
--u.print_packages()
local m2 = require "modules.test"

assert(m.imported == 1)
assert(m.fixed_id == 16000)

--Sanity check
assert(m.random_id == m2.random_id)
assert(m.fixed_id == m2.fixed_id)
assert(m.name_only == m2.name_only)

local lua = ecs.import("flecs.lua")
assert(lua.Host == ecs.lookup_fullpath("flecs.lua.Host"))

local m3 = ecs.import("lua.test")

assert(m.fixed_id == m3.named)
assert(m.name_only == m3.rand_id)
assert(m.component == m3.MStruct)
assert(m.sys == m3.HelloWorld)

--reimplementation of ecs.import()
local function import_func(name)
    local module = ecs.lookup_fullpath(name)
    assert(module ~= 0)
    print("MODULE ID: " .. module)

    local it = ecs.term_iter(ecs.pair(ecs.ChildOf, module))
    local export = {}

    while ecs.term_next(it) do
        for i=1, it.count do
            local e = it.entities[i]
            if ecs.name(e) then
                export[ecs.name(e)] = e
            end
        end
    end

    return export
end

local m4 = import_func "lua.test"

local function asserteq(a, b, msg)
    if(a ~= b) then
        a = a or "(nil)"
        b = b or "(nil)"
        local str = a .. " ~= " .. b
        if msg then str = str .. " (" .. msg .. ")" end
        error(str)
    end
end

local count = 0

for k, v in pairs(m4) do
    count = count + 1
    asserteq(m3[k], v, k)
    asserteq(m[k], v, k)
end

assert(count > 2)

assert(ecs.lookup_fullpath("lua.test.MStruct") ~= 0)
assert(ecs.lookup_fullpath("lua.test.MStruct") == m.component)
assert(ecs.lookup_fullpath("lua.test.NoSuchComponent") == 0)

assert(ecs.get_child_count(m.id) ~= 0)

local scope = ecs.get_scope()
assert(scope == 0)
scope = ecs.set_scope(m.id)
assert(scope == 0)
scope = ecs.get_scope()
assert(scope == m.id)

local prev = ecs.set_name_prefix("tst")
assert(prev == nil)

prev = ecs.set_name_prefix(nil)
assert(prev == "tst")

local it = ecs.scope_iter(m.id)

while ecs.scope_next(it) do
    print("it.count: " .. it.count)
    assert(it.count ~= 0)
end