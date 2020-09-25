--EmmyLua annotations and documentation for flecs-lua
--NOTE: ecs is a native module, do not use this at runtime.

local ecs = {}

---Create a new entity, all parameters are optional
---@param id integer @optional
---@param name string @optional
---@param components string
---@return integer @entity
function ecs.new(id, name, components)
end

---Create N new entities with optional components
---@param type string @optional
---@param n integer
---@return integer[]
function ecs.bulk_new(type, n)
end

---Delete an entity and all of its components
---@param entity string|integer|integer[]
function ecs.delete(entity)
end

---Create a tag
---@param name integer
---@return integer @entity
function ecs.tag(name)
end

---Get the name of an entity
---@param id integer
---@return string|nil @entity name
function ecs.name(id)
end

---Look up an entity by name
---@param name string
---@return string @entity name
function ecs.lookup(name)
end

---Lookup an entity from a full path
---@param name string
---@return string @entity name
function ecs.lookup_fullpath(name)
end

---Test if an entity has a component, type or tag
---@param id integer @entity
---@param type string|integer
---@return boolean
function ecs.has(id, type)
end

---Test if an entity has a role
---@param id integer @entity
---@param role integer
---@return boolean
function ecs.has_role(id, role)
end

---Add a component, type or tag to an entity
---@param id integer entity
---@param type string|integer
function ecs.add(id, type)
end

---Remove a component, type or tag from an entity
---@param id integer @entity
---@param type string|integer
function ecs.remove(id, type)
end

---Clear all components
---@param id integer entity
function ecs.clear(id)
end

---Create a meta array component
---@param name string entity name
---@param description string @format: "(type,N)"
---@return integer @entity
function ecs.array(name, description)
end

---Create a meta struct component
---@param name string entity name
---@param description string @format: "{type member; ...}"
---@return integer @entity
function ecs.struct(name, description)
end

---Get an immutable table to a component
---@param entity integer
---@param component integer
---@return table
function ecs.get(entity, component)
end

---Get a mutable table to a component
---@param entity integer
---@param component integer
---@return table, boolean
function ecs.get_mut(entity, component)
end

---Signal that a component has been modified
---@param t table
function ecs.modified(t)
end

---Set the value of a component
---@param entity integer
---@param component integer
---@param v table
---@return integer entity
function ecs.set(entity, component, v)
end

---Create an alias for a meta type
---@param meta_type string @name
---@param alias string
---@return integer @entity
function ecs.alias(meta_type, alias)
end

---Create a system - DRAFT
---@param func function
---@param name string
---@param phase integer
---@param signature string @optional
---@return integer @entity
function ecs.system(func, name, phase, signature)
end

---Create a module - DRAFT
---@param name string
---@param cb function import callback
---@return integer @entity
function ecs.module(name, cb)
end

--[[
---Import a Lua module - DRAFT
---@param module string
---@return table
function ecs.import(module)
    --only C modules are loaded once?
    if(package.loaded[module]) then
        return package.loaded[module]
    end

    local m = require(module)
    m.import()
    return m
end
]]--

---Print log message
function ecs.log(...)
end

---Print error message
function ecs.err(...)
end

---Print debug message
function ecs.dbg(...)
end

---Print warning message
function ecs.warn(...)
end

---Similar to the standard assert(), but the assertion
---will always succeed when built with -DNDEBUG
---@param v any
---@param message any
function ecs.assert(v, message)
end

---Set target fps
---@param fps number
function ecs.set_target_fps(fps)
end

---Set progress function callback
---@param cb function
function ecs.progress(cb)
end

---@class world_info_t
---@field last_component_id integer
---@field last_id integer
---@field min_id integer
---@field max_id integer

---@field delta_time_raw number
---@field delta_time number
---@field time_scale number
---@field target_fps number
---@field frame_time_total number
---@field system_time_total number
---@field merge_time_total number
---@field world_time_total number
---@field world_time_total_raw number
---@field sleep_err number

---@field frame_count_total integer
---@field merge_count_total integer
---@field pipeline_build_count_total integer
---@field systems_ran_frame integer
local world_info_t = {}

---Get world info
---@return world_info_t
function ecs.world_info()
end

ecs.MatchDefault = 0
ecs.MatchAll = 1
ecs.MatchAny = 2
ecs.MatchExact = 3
ecs.Module = 256
ecs.Prefab = 257
ecs.Hidden = 258
ecs.Disabled = 259
ecs.DisabledIntern = 260
ecs.Inactive = 261
ecs.OnDemand = 262
ecs.Monitor = 263
ecs.Pipeline = 264
ecs.OnAdd = 265
ecs.OnRemove = 266
ecs.OnSet = 267
ecs.UnSet = 268
ecs.PreFrame = 269
ecs.OnLoad = 270
ecs.PostLoad = 271
ecs.PreUpdate = 272
ecs.OnUpdate = 273
ecs.OnValidate = 274
ecs.PostUpdate = 275
ecs.PreStore = 276
ecs.OnStore = 277
ecs.PostFrame = 278
ecs.Flecs = 279
ecs.FlecsCore = 280
ecs.World = 281
ecs.Singleton = 282
ecs.Wildcard = 283
ecs.INSTANCEOF = -144115188075855872
ecs.CHILDOF = -216172782113783808
ecs.TRAIT = -288230376151711744
ecs.AND = -360287970189639680
ecs.OR = -432345564227567616
ecs.XOR = -504403158265495552
ecs.NOT = -576460752303423488
ecs.CASE = -648518346341351424
ecs.SWITCH = -720575940379279360
ecs.OWNED = -792633534417207296

return ecs