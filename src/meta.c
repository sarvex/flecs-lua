#include "private.h"

static
void serialize_type(
    ecs_world_t *world,
    ecs_vector_t *ser,
    const void *base,
    lua_State *L);

static
void serialize_type_op(
    ecs_world_t *world,
    ecs_type_op_t *op,
    const void *base,
    lua_State *L);

static
void serialize_primitive(
    ecs_world_t *world,
    ecs_type_op_t *op,
    const void *base,
    lua_State *L)
{
    switch(op->is.primitive)
    {
        case EcsBool:
            lua_pushboolean(L, (int)*(bool*)base);
            break;
        case EcsChar:
            lua_pushinteger(L, *(char*)base);
            break;
        case EcsString:
            lua_pushstring(L, *(char**)base);
            break;
        case EcsByte:
            lua_pushinteger(L, *(uint8_t*)base);
            break;
        case EcsU8:
            lua_pushinteger(L, *(uint8_t*)base);
            break;
        case EcsU16:
            lua_pushinteger(L, *(uint16_t*)base);
            break;
        case EcsU32:
            lua_pushinteger(L, *(uint32_t*)base);
            break;
        case EcsU64:
            lua_pushinteger(L, *(uint64_t*)base);
            break;
        case EcsI8:
            lua_pushinteger(L, *(int8_t*)base);
            break;
        case EcsI16:
            lua_pushinteger(L, *(int16_t*)base);
            break;
        case EcsI32:
            lua_pushinteger(L, *(int32_t*)base);
            break;
        case EcsI64:
            lua_pushinteger(L, *(int64_t*)base);
            break;
        case EcsF32:
            lua_pushnumber(L, *(float*)base);
            break;
        case EcsF64:
            lua_pushnumber(L, *(double*)base);
            break;
        case EcsEntity:
            lua_pushinteger(L, *(ecs_entity_t*)base);
            break;
        case EcsIPtr:
            lua_pushinteger(L, *(intptr_t*)base);
            break;
        case EcsUPtr:
            lua_pushinteger(L, *(uintptr_t*)base);
            break;
        default:
            luaL_error(L, "unknown primitive (%d)", op->is.primitive);
    }
}

static
void serialize_enum(
    ecs_world_t *world,
    ecs_type_op_t *op,
    const void *base,
    lua_State *L)
{
    const EcsEnum *enum_type = ecs_get_ref_w_entity(world, &op->is.constant, 0, 0);
    ecs_assert(enum_type != NULL, ECS_INVALID_PARAMETER, NULL);

    int32_t value = *(int32_t*)base;

    lua_pushinteger(L, value);
}

static
void serialize_bitmask(
    ecs_world_t *world,
    ecs_type_op_t *op,
    const void *base,
    lua_State *L)
{
    const EcsBitmask *bitmask_type = ecs_get_ref_w_entity(world, &op->is.constant, 0, 0);
    ecs_assert(bitmask_type != NULL, ECS_INVALID_PARAMETER, NULL);

    int32_t value = *(int32_t*)base;

    lua_pushinteger(L, value);
}

static
void serialize_elements(
    ecs_world_t *world,
    ecs_vector_t *elem_ops,
    const void *base,
    int32_t elem_count,
    int32_t elem_size,
    lua_State *L)
{
    const void *ptr = base;

    lua_createtable(L, elem_count, 0);

    int i;
    for(i=0; i < elem_count; i++)
    {
        serialize_type(world, elem_ops, ptr, L);
        lua_rawseti(L, -2, i + 1);

        ptr = ECS_OFFSET(ptr, elem_size);
    }
}

static
void serialize_array(
    ecs_world_t *world,
    ecs_type_op_t *op,
    const void *base,
    lua_State *L)
{
    const EcsMetaTypeSerializer *ser = ecs_get_ref_w_entity(world, &op->is.collection, 0, 0);
    ecs_assert(ser != NULL, ECS_INTERNAL_ERROR, NULL);

    serialize_elements(world, ser->ops, base, op->count, op->size, L);
}

static
void serialize_vector(
    ecs_world_t *world,
    ecs_type_op_t *op,
    const void *base,
    lua_State *L)
{
    ecs_vector_t *value = *(ecs_vector_t**)base;

    if(!value)
    {
        lua_pushnil(L);
        return;
    }

    const EcsMetaTypeSerializer *ser = ecs_get_ref_w_entity(world, &op->is.collection, 0, 0);
    ecs_assert(ser != NULL, ECS_INTERNAL_ERROR, NULL);

    int32_t count = ecs_vector_count(value);
    void *array = ecs_vector_first_t(value, op->size, op->alignment);
    ecs_vector_t *elem_ops = ser->ops;

    ecs_type_op_t *elem_op_hdr = (ecs_type_op_t*)ecs_vector_first(elem_ops, ecs_type_op_t);
    ecs_assert(elem_op_hdr != NULL, ECS_INTERNAL_ERROR, NULL);
    ecs_assert(elem_op_hdr->kind == EcsOpHeader, ECS_INTERNAL_ERROR, NULL);
    size_t elem_size = elem_op_hdr->size;

    serialize_elements(world, elem_ops, array, count, elem_size, L);
}

static
void serialize_map(
    ecs_world_t *world,
    ecs_type_op_t *op,
    const void *base,
    lua_State *L)
{
    ecs_map_t *value = *(ecs_map_t**)base;

    const EcsMetaTypeSerializer *key_ser = ecs_get_ref_w_entity(world, &op->is.map.key, 0, 0);
    ecs_assert(key_ser != NULL, ECS_INTERNAL_ERROR, NULL);

    const EcsMetaTypeSerializer *elem_ser = ecs_get_ref_w_entity(world, &op->is.map.element, 0, 0);
    ecs_assert(elem_ser != NULL, ECS_INTERNAL_ERROR, NULL);

    /* 2 instructions, one for the header */
    ecs_assert(ecs_vector_count(key_ser->ops) == 2, ECS_INTERNAL_ERROR, NULL);

    ecs_type_op_t *key_op = ecs_vector_first(key_ser->ops, ecs_type_op_t);
    ecs_assert(key_op->kind == EcsOpHeader, ECS_INTERNAL_ERROR, NULL);
    key_op = &key_op[1];

    ecs_map_iter_t it = ecs_map_iter(value);
    ecs_map_key_t key;
    void *ptr;

    lua_createtable(L, 0, 0);

    while((ptr = _ecs_map_next(&it, 0, &key)))
    {
        serialize_type_op(world, key_op, (void*)&key, L);
        serialize_type(world, elem_ser->ops, ptr, L);

        key = 0;
    }
}

static
void serialize_type_op(
    ecs_world_t *world,
    ecs_type_op_t *op,
    const void *base,
    lua_State *L)
{
    switch(op->kind)
    {
    case EcsOpHeader:
    case EcsOpPush:
    case EcsOpPop:
        ecs_abort(ECS_INVALID_PARAMETER, NULL);
        break;
    case EcsOpPrimitive:
        serialize_primitive(world, op, ECS_OFFSET(base, op->offset), L);
        break;
    case EcsOpEnum:
        serialize_enum(world, op, ECS_OFFSET(base, op->offset), L);
        break;
    case EcsOpBitmask:
        serialize_bitmask(world, op, ECS_OFFSET(base, op->offset), L);
        break;
    case EcsOpArray:
        serialize_array(world, op, ECS_OFFSET(base, op->offset), L);
        break;
    case EcsOpVector:
        serialize_vector(world, op, ECS_OFFSET(base, op->offset), L);
        break;
    case EcsOpMap:
       // serialize_map(world, op, ECS_OFFSET(base, op->offset), L);
        break;
    }
}

static
void serialize_type(
    ecs_world_t *world,
    ecs_vector_t *ser,
    const void *base,
    lua_State *L)
{
    ecs_type_op_t *ops = (ecs_type_op_t*)ecs_vector_first(ser, ecs_type_op_t);
    int32_t count = ecs_vector_count(ser);

    int i, depth = 0;

    for(i=0; i < count; i++)
    {
        ecs_type_op_t *op = &ops[i];

        switch(op->kind)
        {
            case EcsOpHeader: break;
            case EcsOpPush:
            {
                depth++;
                if(depth > 1)
                {
                    ecs_assert(op->name != NULL, ECS_INVALID_PARAMETER, NULL);
                    lua_pushstring(L, op->name);
                }
                lua_newtable(L);
                break;
            }
            case EcsOpPop:
            {
                if(depth > 1) lua_settable(L, -3);
                depth--;
                break;
            }
            default:
            {
                serialize_type_op(world, op, base, L);
                if(op->name) lua_setfield(L, -2, op->name);
                break;
            }
        }
    }
}

void ecs_ptr_to_lua(
    ecs_world_t *world,
    lua_State *L,
    ecs_entity_t type,
    const void *ptr)
{
    ecs_entity_t ecs_entity(EcsMetaTypeSerializer) = ecs_lookup_fullpath(world, "flecs.meta.MetaTypeSerializer");
    const EcsMetaTypeSerializer *ser = ecs_get(world, type, EcsMetaTypeSerializer);
    ecs_assert(ser != NULL, ECS_INVALID_PARAMETER, NULL);

    serialize_type(world, ser->ops, ptr, L);
}

#ifdef NDEBUG
    #define ecs_lua_dbg(fmt, ...)
#else
    #define ecs_lua_dbg(fmt, ...) //ecs_os_dbg(fmt, __VA_ARGS__)
#endif

static void deserialize_type(ecs_world_t *world, ecs_meta_cursor_t *c, lua_State *L, int idx)
{
    int ktype, vtype, ret, depth = 0;

    ret = ecs_meta_push(c);

    ecs_assert(!ret, ECS_INTERNAL_ERROR, NULL);

    idx = lua_absindex(L, idx);

    luaL_checktype(L, idx, LUA_TTABLE);

    lua_pushnil(L);

    while(lua_next(L, idx))
    {
        ktype = lua_type(L, -2);
        vtype = lua_type(L, -1);

        if(ktype == LUA_TSTRING)
        {
            const char *key = lua_tostring(L, -2);

            ecs_lua_dbg("move_name field: %s", key);
            ret = ecs_meta_move_name(c, key);

            if(ret) luaL_error(L, "field \"%s\" does not exist", key);
        }
        else if(ktype == LUA_TNUMBER)
        {
            lua_Integer key = lua_tointeger(L, -2) - 1;

            ecs_lua_dbg("move idx: %lld", key);
            ret = ecs_meta_move(c, key);

            if(ret) luaL_error(L, "invalid index %I (Lua [%I])", key, key + 1);
        }

        switch(vtype)
        {
            case LUA_TTABLE:
            {
                ecs_lua_dbg("meta_push (nested)");
                //ecs_meta_push(c);
                //lua_pushnil(L);
                //depth++;
                int top = lua_gettop(L);
                deserialize_type(world, c, L, top);
                break;
            }
            case LUA_TNUMBER:
            {
                if(lua_isinteger(L, -1))
                {
                    lua_Integer integer = lua_tointeger(L, -1);

                    ecs_lua_dbg("  set_int: %lld", integer);
                    ret = ecs_meta_set_int(c, integer);

                    if(ret) luaL_error(L, "integer out of range (%I)", integer);
                }
                else
                {
                    lua_Number number = lua_tonumber(L, -1);

                    ecs_lua_dbg("  set_float %f", number);
                    ret = ecs_meta_set_float(c, number);

                    if(ret) luaL_error(L, "failed to set float (%f)", number);
                }

                break;
            }
            case LUA_TBOOLEAN:
            {
                ecs_lua_dbg("  set_bool: %d", lua_toboolean(L, -1));
                ret = ecs_meta_set_bool(c, lua_toboolean(L, -1));

                ecs_assert(!ret, ECS_INTERNAL_ERROR, NULL);
                break;
            }
            case LUA_TSTRING:
            {
                ecs_lua_dbg("  set_string: %s", lua_tostring(L, -1));
                ret = ecs_meta_set_string(c, lua_tostring(L, -1));

                ecs_assert(!ret, ECS_INTERNAL_ERROR, NULL);
                break;
            }
            case LUA_TNIL:
            {
                ecs_lua_dbg("  set_null");
                ret = ecs_meta_set_null(c);

                ecs_assert(!ret, ECS_INTERNAL_ERROR, NULL);
                break;
            }
            default:
            {
                if(vtype == LUA_TSTRING)
                    luaL_error(L, "invalid type for field '%s' (got %s)", lua_tostring(L, -2), lua_typename(L, vtype));
                else
                    luaL_error(L, "invalid type at index [%d] (got %s)", lua_tointeger(L, -2), lua_typename(L, vtype));
            }
        }

        lua_pop(L, 1);
    }

    ecs_lua_dbg("meta pop");
    ret = ecs_meta_pop(c);
    ecs_assert(!ret, ECS_INTERNAL_ERROR, NULL);
}

void ecs_lua_to_ptr(
    ecs_world_t *world,
    lua_State *L,
    int idx,
    ecs_entity_t type,
    void *ptr)
{
    ecs_meta_cursor_t c = ecs_meta_cursor(world, type, ptr);

    deserialize_type(world, &c, L, idx);
}

static
void serialize_column(
    ecs_world_t *world,
    lua_State *L,
    const EcsMetaTypeSerializer *ser,
    const void *base,
    int32_t count)
{
    ecs_vector_t *ops = ser->ops;
    ecs_type_op_t *hdr = ecs_vector_first(ops, ecs_type_op_t);
    ecs_assert(hdr->kind == EcsOpHeader, ECS_INTERNAL_ERROR, NULL);

    serialize_elements(world, ser->ops, base, count, hdr->size, L);
}

void ecs_iter_to_lua(
    ecs_world_t *world,
    lua_State *L,
    ecs_iter_t *it,
    ecs_type_t select)
{
    ecs_entity_t ecs_entity(EcsMetaTypeSerializer) = ecs_lookup_fullpath(world, "flecs.meta.MetaTypeSerializer");
    ecs_assert(ecs_entity(EcsMetaTypeSerializer) != 0, ECS_INTERNAL_ERROR, NULL);

    /* it */
    lua_createtable(L, 0, 3);

    /* metatable */
    lua_createtable(L, 0, 1);

    /* metatable.__ecs_iter */
    lua_createtable(L, 3, 0);

    lua_pushlightuserdata(L, it);
    lua_rawseti(L, -2, 1);

    lua_setfield(L, -2, "__ecs_iter");
    lua_setmetatable(L, -2);

    /* it.count */
    lua_pushinteger(L, it->count);
    lua_setfield(L, -2, "count");

    /* it.system */
    lua_pushinteger(L, it->system);
    lua_setfield(L, -2, "system");
    
    /* it.delta_time */
    lua_pushnumber(L, it->delta_time);
    lua_setfield(L, -2, "delta_time");

    /* it.delta_system_time */
    lua_pushnumber(L, it->delta_system_time);
    lua_setfield(L, -2, "delta_system_time");

    /* it.world_time */
    lua_pushnumber(L, it->world_time);
    lua_setfield(L, -2, "world_time");

    /* it.columns[] */
    lua_createtable(L, it->column_count, 0);

    if(!it->count)
    {
        lua_setfield(L, -2, "columns");
        return;
    }

    ecs_type_t table_type = ecs_iter_type(it);
    ecs_entity_t *comps = ecs_vector_first(table_type, ecs_entity_t);
    int32_t i, count = ecs_vector_count(table_type);

    for(i=0; i < it->column_count; i++)
    {
        if(select && !ecs_type_has_entity(world, select, comps[i])) continue;

        const EcsMetaTypeSerializer *ser = ecs_get(world, comps[i], EcsMetaTypeSerializer);
        ecs_assert(ser != NULL, ECS_INTERNAL_ERROR, NULL);

        if(!ser) luaL_error(L, "column %d cannot be serialized", i);

        serialize_column(world, L, ser, ecs_table_column(it, i), it->count);

        lua_rawseti(L, -2, i+1);
    }

    lua_setfield(L, -2, "columns");
}

static
void deserialize_column(
    ecs_world_t *world,
    lua_State *L,
    int idx,
    ecs_entity_t type,
    void *base,
    size_t stride,
    int32_t count)
{
    ecs_meta_cursor_t c;

    int j;
    for(j=0; j < count; j++)
    {
        c = ecs_meta_cursor(world, type, (char*)base + j * stride);

        lua_rawgeti(L, idx, j + 1); /* columns[i+1][j+1] */
        deserialize_type(world, &c, L, -1);

        lua_pop(L, 1);
    }
}

void ecs_lua_to_iter(
    ecs_world_t *world,
    lua_State *L,
    int idx)
{
    ecs_lua__prolog(L);
    ecs_iter_t *it = ecs_lua__checkiter(L, idx);

    if(!it->count) return;

    ecs_type_t table_type = ecs_iter_type(it);
    ecs_entity_t *comps = ecs_vector_first(table_type, ecs_entity_t);
    int32_t i, j, count = ecs_vector_count(table_type);

    luaL_getsubtable(L, idx, "columns");
    luaL_checktype(L, -1, LUA_TTABLE);

    for(i=0; i < it->column_count; i++)
    {
        if(ecs_is_readonly(it, i+1)) continue;

        lua_rawgeti(L, -1, i + 1); /* columns[i+1] */

        ecs_assert(it->count == lua_rawlen(L, -1), ECS_INTERNAL_ERROR, NULL);

        deserialize_column(world, L, -1, comps[i], ecs_table_column(it, i), ecs_column_size(it, i+1), it->count);

        lua_pop(L, 1); /* columns[i+1] */
    }

    lua_pop(L, 1); /* columns */

    ecs_lua__epilog(L);
}