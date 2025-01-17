#include "private.h"

int get_child_count(lua_State *L)
{
    ecs_world_t *w = ecs_lua_world(L);

    lua_Integer e = luaL_checkinteger(L, 1);

    int32_t count = ecs_get_child_count(w, e);

    lua_pushinteger(L, count);

    return 1;
}

int scope_iter(lua_State *L)
{
    ecs_world_t *w = ecs_lua_world(L);

    ecs_entity_t parent = luaL_checkinteger(L, 1);

    ecs_iter_t it;

    if(lua_gettop(L) > 1)
    {
        ecs_filter_t filter;
        checkfilter(L, w, &filter, 2);
        it = ecs_scope_iter_w_filter(w, parent, &filter);
        ecs_filter_fini(&filter);
    }
    else it = ecs_scope_iter(w, parent);

    ecs_iter_to_lua(&it, L, true);

    return 1;
}

int scope_next(lua_State *L)
{
    ecs_iter_t *it = ecs_lua_to_iter(L, 1);

    int b = ecs_scope_next(it);

    if(b) ecs_lua_iter_update(L, 1, it);

    lua_pushboolean(L, b);

    return 1;
}

int set_scope(lua_State *L)
{
    ecs_world_t *w = ecs_lua_world(L);

    ecs_entity_t scope = luaL_checkinteger(L, 1);

    ecs_entity_t prev = ecs_set_scope(w, scope);

    lua_pushinteger(L, prev);

    return 1;
}

int get_scope(lua_State *L)
{
    ecs_world_t *w = ecs_lua_world(L);

    ecs_entity_t e = ecs_get_scope(w);

    lua_pushinteger(L, e);

    return 1;
}

int set_name_prefix(lua_State *L)
{
    ecs_world_t *w = ecs_lua_world(L);
    ecs_lua_ctx *ctx = ecs_lua_get_context(L, w);

    const char *prefix = NULL;

    if(!lua_isnil(L, 1)) prefix = luaL_checkstring(L, 1);

    const char *prev = ecs_set_name_prefix(w, prefix);

    lua_pushstring(L, prev);

    if(ctx->prefix_ref != LUA_NOREF)
    {
        ecs_lua_unref(L, w, ctx->prefix_ref);
        ctx->prefix_ref = LUA_NOREF;
    }

    lua_pushvalue(L, 1);
    ctx->prefix_ref = ecs_lua_ref(L, w);

    return 1;
}