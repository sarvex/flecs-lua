#include <flecs_lua.h>

#include "constants.h"

#include <lualib.h>
#include <lauxlib.h>

ecs_world_t *ecs_lua_get_world(lua_State *L)
{
    lua_pushstring(L, "ecs_world");
    lua_gettable(L, LUA_REGISTRYINDEX);
    ecs_world_t *p = lua_touserdata(L, -1);
    lua_pop(L, 1);
    return p;
}

static void set_default_name(ecs_world_t *w, ecs_entity_t e)
{
#ifdef NDEBUG
    char str[32];
    snprintf(str, sizeof(str), "Lua.%llu", e);
    ecs_set(w, e, EcsName, {.alloc_value = str});
    #else
    ecs_set(w, e, EcsName, {.value = "Lua.Entity"});
#endif
}

static int new_entity(lua_State *L)
{
    ecs_world_t *w = ecs_lua_get_world(L);

    ecs_entity_t e = 0;

    const char *name = NULL;
    const char *components = NULL;
    int args = lua_gettop(L);

    if(!args)
    {
        e = ecs_new(w, 0);
    }
    else if(args == 1)
    {
        if(lua_isinteger(L, 1)) e = luaL_checkinteger(L, 1);
        else name = luaL_checkstring(L, 1);
    }
    else if(args == 2)
    {
        if(lua_isinteger(L, 1))
        {
            e = luaL_checkinteger(L, 1);
            name = luaL_checkstring(L, 2);
        }
        else
        {
            name = luaL_checkstring(L, 1);
            components = luaL_checkstring(L, 2);
        }
    }
    else if(args == 3)
    {
        e = luaL_checkinteger(L, 1);
        name = luaL_checkstring(L, 2);
        components = luaL_checkstring(L, 3);
    }
    else luaL_error(L, "too many arguments");

    if(name) e = ecs_new_entity(w, e, name, components);
    else set_default_name(w, e);

    lua_pushinteger(L, e);

    return 1;
}

static int bulk_new(lua_State *L)
{
    ecs_world_t *w = ecs_lua_get_world(L);

    lua_Integer count = 0;
    const char *name = NULL;
    const ecs_entity_t* entities = NULL;

    if(lua_gettop(L) == 2) 
    {
        name = luaL_checkstring(L, 1);
        count = luaL_checkinteger(L, 2);

        ecs_entity_t type_entity = ecs_lookup(w, name);
        ecs_type_t type = ecs_type_from_entity(w, type_entity);

        entities = ecs_bulk_new_w_type(w, type, count);
    }
    else
    {
        count = luaL_checkinteger(L, 1);
        entities = ecs_bulk_new(w, 0, count);
    } 

    lua_newtable(L);

    lua_Integer i;
    for(i=0; i < count; i++)
    {
        lua_pushinteger(L, entities[i]);
        lua_rawseti(L, -2, i+1);
        
        set_default_name(w, entities[i]);
    }

    return 1;
}

static int delete_entity(lua_State *L)
{
    ecs_world_t *w = ecs_lua_get_world(L);

    ecs_entity_t entity = luaL_checkinteger(L, 1);

    ecs_delete(w, entity);

    return 0;
}

static int entity_name(lua_State *L)
{
    ecs_world_t *w = ecs_lua_get_world(L);

    ecs_entity_t e = luaL_checkinteger(L, 1);

    const char *name = ecs_get_name(w, e);

    lua_pushstring(L, name);

    return 1;
}

static int lookup_entity(lua_State *L)
{
    ecs_world_t *w = ecs_lua_get_world(L);

    const char *name = luaL_checkstring(L, 1);

    ecs_entity_t e = ecs_lookup(w, name);

    lua_pushinteger(L, e);

    return 1;
}

static int entity_has(lua_State *L)
{
    ecs_world_t *w = ecs_lua_get_world(L);

    ecs_entity_t e = luaL_checkinteger(L, 1);
    ecs_entity_t type_entity = 0;
    ecs_type_t type = NULL;

    if(lua_isinteger(L, 2)) type_entity = luaL_checkinteger(L, 2);
    else
    {
        const char *name = luaL_checkstring(L, 2);
        type_entity = ecs_lookup_fullpath(w, name);
    }

    type = ecs_type_from_entity(w, type_entity);

    if(ecs_has_type(w, e, type)) lua_pushboolean(L, 1);
    else lua_pushboolean(L, 0);

    return 1;
}

static int add_type(lua_State *L)
{
    ecs_world_t *w = ecs_lua_get_world(L);

    ecs_entity_t e = luaL_checkinteger(L, 1);
    ecs_entity_t type_entity = 0;

    if(lua_isinteger(L, 2)) type_entity = luaL_checkinteger(L, 2);
    else
    {
        const char *name = luaL_checkstring(L, 2);
        type_entity = ecs_lookup_fullpath(w, name);
    }

    ecs_type_t type = ecs_type_from_entity(w, type_entity);

    ecs_add_type(w, e, type);

    return 0;
}

static int remove_type(lua_State *L)
{
    ecs_world_t *w = ecs_lua_get_world(L);

    ecs_entity_t e = luaL_checkinteger(L, 1);
    ecs_entity_t type_entity = 0;

    if(lua_isinteger(L, 2)) type_entity = luaL_checkinteger(L, 2);
    else
    {
        const char *name = luaL_checkstring(L, 2);
        type_entity = ecs_lookup_fullpath(w, name);
    }

    ecs_type_t type = ecs_type_from_entity(w, type_entity);

    ecs_remove_type(w, e, type);

    return 0;
}

static int new_array(lua_State *L)
{
    ecs_world_t *w = ecs_lua_get_world(L);

    const char *name = luaL_checkstring(L, 1);
    const char *desc = luaL_checkstring(L, 2);

    ecs_entity_t e = ecs_new(w, 0);

    EcsMetaType meta =
    {
        .kind = EcsArrayType,
        .size = 0,
        .alignment = 0,
        .descriptor = desc
    };

    ecs_new_meta(w, e, &meta);

    ecs_set(w, e, EcsName, {.alloc_value = (char*)name});

    lua_pushinteger(L, e);

    return 1;
}

static int new_struct(lua_State *L)
{
    ecs_world_t *w = ecs_lua_get_world(L);

    const char *name = luaL_checkstring(L, 1);
    const char *desc = luaL_checkstring(L, 2);

    ecs_entity_t e = ecs_new(w, 0);

    EcsMetaType meta =
    {
        .kind = EcsStructType,
        .size = 0,
        .alignment = 0,
        .descriptor = desc
    };

    ecs_new_meta(w, e, &meta);

    ecs_set(w, e, EcsName, {.alloc_value = (char*)name});

    lua_pushinteger(L, e);

    return 1;
}

static int func(lua_State *L)
{
    ecs_world_t *w = ecs_lua_get_world(L);
    return 1;
}

static const luaL_Reg ecs_lib[] =
{
    { "new", new_entity },
    { "bulk_new", bulk_new },
    { "delete", delete_entity },
    { "name", entity_name },
    { "lookup", lookup_entity },
    { "has", entity_has },
    { "add", add_type },
    { "remove", remove_type },
    { "array", new_array },
    { "struct", new_struct },
#define XX(const) {#const, NULL },
    ECS_LUA_CONSTANTS(XX)
#undef XX
    { NULL, NULL }
};

int luaopen_ecs(lua_State *L)
{
    luaL_newlib(L, ecs_lib);

#define XX(const) lua_pushnumber(L, Ecs##const); lua_setfield(L, -2, #const);
    ECS_LUA_CONSTANTS(XX)
#undef XX

    return 1;
}

int ecs_lua_init(ecs_world_t *world, lua_State *L)
{
    if(world == NULL || L == NULL) return 1;

    lua_pushstring(L, "ecs_world");
    lua_pushlightuserdata(L, world);
    lua_settable(L, LUA_REGISTRYINDEX);

    //luaL_requiref(L, "_G", luaopen_ecs, 1);
    luaL_requiref(L, "ecs", luaopen_ecs, 1);
    lua_pop(L, 1);


    return 0;
}

int ecs_lua_exit(lua_State *L)
{
    return 0;
}