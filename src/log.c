#include "private.h"

#define ECS_LUA__LOG 0
#define ECS_LUA__ERROR 1
#define ECS_LUA__DEBUG 2
#define ECS_LUA__WARN 3

int vararg2str(lua_State *L, int n, ecs_strbuf_t *buf)
{
    lua_getglobal(L, "tostring");

    int i;
    for(i=1; i <= n; i++)
    {
        lua_pushvalue(L, -1);
        lua_pushvalue(L, i);
        lua_call(L, 1, 1);
        const char *arg = lua_tostring(L, -1);

        if(!arg) return luaL_error(L, "expected string from 'tostring'");

        if(i>1) ecs_strbuf_appendstr(buf, " ");

        ecs_strbuf_appendstr(buf, arg);

        lua_pop(L, 1);
    }

    return 0;
}

int print_type(lua_State *L, int type)
{
    int n = lua_gettop(L);

    ecs_strbuf_t buf = ECS_STRBUF_INIT;

    vararg2str(L, n, &buf);

    char *str = ecs_strbuf_get(&buf);

    switch(type)
    {
        case ECS_LUA__LOG:
            ecs_os_log(str);
            break;
        case ECS_LUA__ERROR:
            ecs_os_err(str);
            break;
        case ECS_LUA__DEBUG:
            ecs_os_dbg(str);
            break;
        case ECS_LUA__WARN:
            ecs_os_warn(str);
            break;
        default:
            break;
    }

    ecs_os_free(str);

    return 0;
}

int print_log(lua_State *L)
{
    return print_type(L, ECS_LUA__LOG);
}

int print_err(lua_State *L)
{
    return print_type(L, ECS_LUA__ERROR);
}

int print_dbg(lua_State *L)
{
    return print_type(L, ECS_LUA__DEBUG);
}

int print_warn(lua_State *L)
{
    return print_type(L, ECS_LUA__WARN);
}

int tracing_enable(lua_State *L)
{
    int level = luaL_checkinteger(L, 1);

    ecs_tracing_enable(level);

    return 0;
}

int tracing_color_enable(lua_State *L)
{
    int enable = lua_toboolean(L, 1);

    ecs_tracing_color_enable(enable);

    return 0;
}