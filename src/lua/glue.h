#ifndef __LUAGLUE_H__
#define __LUAGLUE_H__ 1

// ARGUMENT IDENTIFIER
#define sn__lua_arg(POS) sn__lua_arg_##POS
// OPTIONAL ARGUMENT STUFF
#define sn__lua_glue_optarg(POS, LUA_ISA, DEFAULT, VALUE)\
  (((POS) <= sn__lua_stack_size && LUA_ISA(L, (POS))) ? (VALUE) : (DEFAULT))
// GLUE FUNCTION NAME
#define sn__lua_glue_function_name(NAME) sn_##NAME##_glue



///// RETURNS

#define sn__lua_glue_return(COUNT, NAME, RET)\
  sn__lua_glue_return_##RET(COUNT, NAME)



///// RETURN HANDLING PER TYPE

// NO RETURN VALUE
#define sn__lua_glue_return_void(COUNT, NAME)\
  sn__lua_glue_call(COUNT, NAME); return 0;

// POINTER RETURN
#define sn__lua_glue_return_voidptr(COUNT, NAME)\
  lua_pushlightuserdata(L, sn__lua_glue_call(COUNT, NAME))

// INTEGER RETURN
#define sn__lua_glue_return_generic_int(COUNT, NAME)\
  lua_pushinteger(L, (lua_Integer) (sn__lua_glue_call(COUNT, NAME))); return 1;
#define sn__lua_glue_return_int(COUNT, NAME)\
  sn__lua_glue_return_generic_int(COUNT, NAME)
#define sn__lua_glue_return_uint32_t(COUNT, NAME)\
  sn__lua_glue_return_generic_int(COUNT, NAME)
#define sn__lua_glue_return_int32_t(COUNT, NAME)\
  sn__lua_glue_return_generic_int(COUNT, NAME)
#define sn__lua_glue_return_int16_t(COUNT, NAME)\
  sn__lua_glue_return_generic_int(COUNT, NAME)
#define sn__lua_glue_return_uint_t(COUNT, NAME)\
  sn__lua_glue_return_generic_int(COUNT, NAME)
#define sn__lua_glue_return_uint8_t(COUNT, NAME)\
  sn__lua_glue_return_generic_int(COUNT, NAME)
#define sn__lua_glue_return_int8_t(COUNT, NAME)\
  sn__lua_glue_return_generic_int(COUNT, NAME)

// NUMBER RETURN
#define sn__lua_glue_return_number(COUNT, NAME)\
  lua_pushnumber(L, (lua_Number) (sn__lua_glue_call(COUNT, NAME))); return 1;
#define sn__lua_glue_return_float(COUNT, NAME)\
  sn__lua_glue_return_number(COUNT, NAME)
#define sn__lua_glue_return_double(COUNT, NAME)\
  sn__lua_glue_return_number(COUNT, NAME)

// STRING RETURN
#define sn__lua_glue_return_string_t(COUNT, NAME)\
  lua_pushstring(L, (const char *)(sn__lua_glue_call(COUNT, NAME))); return 1;

// OBJECT RETURN
#define sn__lua_glue_return_object_t(COUNT, NAME)\
  lua_pushlightuserdata(L, (void *) (sn__lua_glue_call(COUNT, NAME))); return 1;

// MATHS RETURNS
#define sn__lua_glue_return_maths_generic(COUNT, NAME, CTYPE, PUSHTYPE)\
  CTYPE sn__lua_ret_value = {0.0};\
  sn__lua_glue_call(COUNT, NAME, sn__lua_ret_value);\
  lua_push##PUSHTYPE(L, sn__lua_ret_value);\
  return 1;
#define sn__lua_glue_return_vec3_t(COUNT, NAME)\
  sn__lua_glue_return_maths_generic(COUNT, NAME, vec3_t, vec3)
#define sn__lua_glue_return_vec4_t(COUNT, NAME)\
  sn__lua_glue_return_maths_generic(COUNT, NAME, vec4_t, vec4)
#define sn__lua_glue_return_quat_t(COUNT, NAME)\
  sn__lua_glue_return_maths_generic(COUNT, NAME, quat_t, quat)
#define sn__lua_glue_return_mat4_t(COUNT, NAME)\
  sn__lua_glue_return_maths_generic(COUNT, NAME, mat4_t, mat4)


///// FUNCTION CALLS

#define sn__lua_glue_call(COUNT, NAME, EXTRA_ARGS...)\
  NAME(sn__lua_glue_args_##COUNT, ##EXTRA_ARGS)



///// ARGUMENTS

#define sn__lua_glue_args_0 

#define sn__lua_glue_args_1\
  sn__lua_arg(1)

#define sn__lua_glue_args_2\
  sn__lua_arg(1),\
  sn__lua_arg(2)

#define sn__lua_glue_args_3\
  sn__lua_arg(1),\
  sn__lua_arg(2),\
  sn__lua_arg(3)

#define sn__lua_glue_args_4\
  sn__lua_arg(1),\
  sn__lua_arg(2),\
  sn__lua_arg(3),\
  sn__lua_arg(4)

#define sn__lua_glue_args_5\
  sn__lua_arg(1),\
  sn__lua_arg(2),\
  sn__lua_arg(3),\
  sn__lua_arg(4),\
  sn__lua_arg(5)

#define sn__lua_glue_args_6\
  sn__lua_arg(1),\
  sn__lua_arg(2),\
  sn__lua_arg(3),\
  sn__lua_arg(4),\
  sn__lua_arg(5),\
  sn__lua_arg(6)

#define sn__lua_glue_args_7\
  sn__lua_arg(1),\
  sn__lua_arg(2),\
  sn__lua_arg(3),\
  sn__lua_arg(4),\
  sn__lua_arg(5),\
  sn__lua_arg(6),\
  sn__lua_arg(7)

#define sn__lua_glue_args_\
  sn__lua_arg(1),\
  sn__lua_arg(2),\
  sn__lua_arg(3),\
  sn__lua_arg(4),\
  sn__lua_arg(5),\
  sn__lua_arg(6),\
  sn__lua_arg(7),\
  sn__lua_arg(8)

#define sn__lua_glue_args_\
  sn__lua_arg(1),\
  sn__lua_arg(2),\
  sn__lua_arg(3),\
  sn__lua_arg(4),\
  sn__lua_arg(5),\
  sn__lua_arg(6),\
  sn__lua_arg(7),\
  sn__lua_arg(8),\
  sn__lua_arg(9)

#define sn__lua_glue_args_\
  sn__lua_arg(1),\
  sn__lua_arg(2),\
  sn__lua_arg(3),\
  sn__lua_arg(4),\
  sn__lua_arg(5),\
  sn__lua_arg(6),\
  sn__lua_arg(7),\
  sn__lua_arg(8),\
  sn__lua_arg(9),\
  sn__lua_arg(10)



///// SUPPORTED TYPES

// VOID
#define sn__lua_glue_type_void void

// POINTER TYPES
#define sn__lua_glue_type_voidptr void *

// INTEGER TYPES
#define sn__lua_glue_type_int int
#define sn__lua_glue_type_uint32_t uint32_t
#define sn__lua_glue_type_int32_t int32_t
#define sn__lua_glue_type_int16_t int16_t
#define sn__lua_glue_type_uint_t uint_t
#define sn__lua_glue_type_uint8_t uint8_t
#define sn__lua_glue_type_int8_t int8_t

// FLOAT TYPES
#define sn__lua_glue_type_float float
#define sn__lua_glue_type_double double

// STRING TYPES
#define sn__lua_glue_type_string_t string_t

// GENERIC OBJECT
#define sn__lua_glue_type_object_t object_t *
#define sn__lua_glue_type(TYPE) sn__lua_glue_type_##TYPE



///// ARGUMENT HANDLING

#define sn__lua_glue_handle_arg(POS, TYPE)\
  sn__lua_glue_handle_arg_##TYPE(POS)
#define sn__lua_glue_handle_arg_generic(POS, TYPE)\
  const sn__lua_glue_type(TYPE) sn__lua_arg(POS) = sn__lua_glue_arg_value(POS, TYPE);

// VOID
#define sn_lua_glue_arg_handle_void(POS) 

// POINTER ARGUMENTS
#define sn_lua_glue_arg_handle_voidptr(POS)\
  sn__lua_glue_handle_arg_generic(POS, voidptr)

// INTEGER ARGUMENTS
#define sn__lua_glue_handle_arg_int(POS)\
  sn__lua_glue_handle_arg_generic(POS, int)
#define sn__lua_glue_handle_arg_uint32_t(POS)\
  sn__lua_glue_handle_arg_generic(POS, uint32_t)
#define sn__lua_glue_handle_arg_int32_t(POS)\
  sn__lua_glue_handle_arg_generic(POS, int32_t)
#define sn__lua_glue_handle_arg_int16_t(POS)\
  sn__lua_glue_handle_arg_generic(POS, int16_t)
#define sn__lua_glue_handle_arg_uint_t(POS)\
  sn__lua_glue_handle_arg_generic(POS, uint_t)
#define sn__lua_glue_handle_arg_uint8_t(POS)\
  sn__lua_glue_handle_arg_generic(POS, uint8_t)
#define sn__lua_glue_handle_arg_int8_t(POS)\
  sn__lua_glue_handle_arg_generic(POS, int8_t)

// FLOAT ARGUMENTS
#define sn__lua_glue_handle_arg_float(POS)\
  sn__lua_glue_handle_arg_generic(POS, float)
#define sn__lua_glue_handle_arg_double(POS)\
  sn__lua_glue_handle_arg_generic(POS, double)

// STRING ARGUMENTS
#define sn__lua_glue_handle_arg_string_t(POS)\
  sn__lua_glue_handle_arg_generic(POS, string_t)

// GENERIC OBJECT
#define sn__lua_glue_handle_arg_object_t(POS)\
  sn__lua_glue_handle_arg_generic(POS, object_t)

// MATHS TYPES
#define sn__lua_glue_handle_arg_maths_generic(POS, TYPENAME, TONAME)\
  TYPENAME sn__lua_arg(POS) = {0.0};\
  if ((POS) <= sn__lua_stack_size) lua_to##TONAME(L, POS, sn__lua_arg(POS));
#define sn__lua_glue_handle_arg_vec3_t(POS)\
  sn__lua_glue_handle_arg_maths_generic(POS, vec3_t, vec3)
#define sn__lua_glue_handle_arg_vec4_t(POS)\
  sn__lua_glue_handle_arg_maths_generic(POS, vec4_t, vec4)
#define sn__lua_glue_handle_arg_quat_t(POS)\
  sn__lua_glue_handle_arg_maths_generic(POS, quat_t, quat)
#define sn__lua_glue_handle_arg_mat4_t(POS)\
  sn__lua_glue_handle_arg_maths_generic(POS, mat4_t, mat4)



///// ARGUMENT VALUES

// VOID
#define sn_lua_glue_arg_value_void(POS) 

// POINTER TYPES
#define sn__lua_glue_arg_value_voidptr(POS)\
  sn__lua_glue_optarg(POS, lua_isuserdata, NULL, lua_touserdata(L, (POS)))

// INTEGER TYPES
#define sn__lua_glue_arg_value_generic_int(POS, TYPE)\
  sn__lua_glue_optarg(POS, lua_isnumber, 0, (TYPE)lua_tointeger(L, (POS)))
#define sn__lua_glue_arg_value_int(POS)\
  sn__lua_glue_arg_value_generic_int(POS, int)
#define sn__lua_glue_arg_value_uint32_t(POS)\
  sn__lua_glue_arg_value_generic_int(POS, uint32_t)
#define sn__lua_glue_arg_value_int32_t(POS)\
  sn__lua_glue_arg_value_generic_int(POS, int32_t)
#define sn__lua_glue_arg_value_int16_t(POS)\
  sn__lua_glue_arg_value_generic_int(POS, int16_t)
#define sn__lua_glue_arg_value_uint_t(POS)\
  sn__lua_glue_arg_value_generic_int(POS, uint_t)
#define sn__lua_glue_arg_value_uint8_t(POS)\
  sn__lua_glue_arg_value_generic_int(POS, uint8_t)
#define sn__lua_glue_arg_value_int8_t(POS)\
  sn__lua_glue_arg_value_generic_int(POS, int8_t)

// FLOAT TYPES
#define sn__lua_glue_arg_value_generic_number(POS, TYPE)\
  sn__lua_glue_optarg(POS, lua_isnumber, 0.0, (TYPE)lua_tonumber(L, (POS)))
#define sn__lua_glue_arg_value_float(POS)\
  sn__lua_glue_arg_value_generic_number(POS, float)
#define sn__lua_glue_arg_value_double(POS)\
  sn__lua_glue_arg_value_generic_number(POS, double)

// STRING TYPES
#define sn__lua_glue_arg_value_string_t(POS)\
  sn__lua_glue_optarg(POS, lua_isstring, NULL, (string_t)lua_tolstring(L, (POS), NULL))

// GENERIC OBJECT TYPE
#define sn__lua_glue_arg_value_object_t(POS)\
  sn__lua_glue_optarg(POS, lua_isuserdata, NULL, (object_t)lua_touserdata(L, (POS)))

// GENERIC ARGUMENT
#define sn__lua_glue_arg_value(POS, TYPE) sn__lua_glue_arg_value_##TYPE(POS)



///// ARGUMENT DECLARATION
// currently does nothing

#define sn__lua_glue_decl_arg(POS, TYPE) 



///// FUNCTION OPEN/CLOSE

#define sn__lua_glue_function_open(NAME)\
  int sn__lua_glue_function_name(NAME)(lua_State *L) {\
    const int sn__lua_stack_size = lua_gettop(L);

#define sn__lua_glue_function_close() }



///// PUSHING / DECLARING SCRIPT FUNCTIONS

#define push_lua_glue_function(STATE, NAME, SCRIPTNAME)\
  (lua_pushcclosure(STATE, sn__lua_glue_function_name(NAME), 0),\
   lua_setfield(STATE, LUA_GLOBALSINDEX, SCRIPTNAME))\

#define decl_lua_glue_function_0(NAME, RETURNTYPE)\
  sn__lua_glue_function_open(NAME)\
    sn__lua_glue_return(0, NAME, RETURNTYPE)\
  sn__lua_glue_function_close()

#define decl_lua_glue_function_1(NAME, RETURNTYPE, ARGTYPE1)\
  sn__lua_glue_function_open(NAME)\
    sn__lua_glue_decl_arg(1, ARGTYPE1)\
    sn__lua_glue_handle_arg(1, ARGTYPE1)\
    sn__lua_glue_return(1, NAME, RETURNTYPE)\
  sn__lua_glue_function_close()

#define decl_lua_glue_function_2(NAME, RETURNTYPE, ARGTYPE1, ARGTYPE2)\
  sn__lua_glue_function_open(NAME)\
    sn__lua_glue_decl_arg(1, ARGTYPE1)\
    sn__lua_glue_decl_arg(2, ARGTYPE2)\
    sn__lua_glue_handle_arg(1, ARGTYPE1)\
    sn__lua_glue_handle_arg(2, ARGTYPE2)\
    sn__lua_glue_return(2, NAME, RETURNTYPE)\
  sn__lua_glue_function_close()

#define decl_lua_glue_function_3(NAME, RETURNTYPE, ARGTYPE1, ARGTYPE2, ARGTYPE3)\
  sn__lua_glue_function_open(NAME)\
    sn__lua_glue_decl_arg(1, ARGTYPE1)\
    sn__lua_glue_decl_arg(2, ARGTYPE2)\
    sn__lua_glue_decl_arg(3, ARGTYPE3)\
    sn__lua_glue_handle_arg(1, ARGTYPE1)\
    sn__lua_glue_handle_arg(2, ARGTYPE2)\
    sn__lua_glue_handle_arg(3, ARGTYPE3)\
    sn__lua_glue_return(3, NAME, RETURNTYPE)\
  sn__lua_glue_function_close()

#define decl_lua_glue_function_4(NAME, RETURNTYPE, ARGTYPE1, ARGTYPE2, ARGTYPE3, ARGTYPE4)\
  sn__lua_glue_function_open(NAME)\
    sn__lua_glue_decl_arg(1, ARGTYPE1)\
    sn__lua_glue_decl_arg(2, ARGTYPE2)\
    sn__lua_glue_decl_arg(3, ARGTYPE3)\
    sn__lua_glue_decl_arg(4, ARGTYPE4)\
    sn__lua_glue_handle_arg(1, ARGTYPE1)\
    sn__lua_glue_handle_arg(2, ARGTYPE2)\
    sn__lua_glue_handle_arg(3, ARGTYPE3)\
    sn__lua_glue_handle_arg(4, ARGTYPE4)\
    sn__lua_glue_return(4, NAME, RETURNTYPE)\
  sn__lua_glue_function_close()

#define decl_lua_glue_function_5(NAME, RETURNTYPE, ARGTYPE1, ARGTYPE2, ARGTYPE3, ARGTYPE4, ARGTYPE5)\
  sn__lua_glue_function_open(NAME)\
    sn__lua_glue_decl_arg(1, ARGTYPE1)\
    sn__lua_glue_decl_arg(2, ARGTYPE2)\
    sn__lua_glue_decl_arg(3, ARGTYPE3)\
    sn__lua_glue_decl_arg(4, ARGTYPE4)\
    sn__lua_glue_decl_arg(5, ARGTYPE5)\
    sn__lua_glue_handle_arg(1, ARGTYPE1)\
    sn__lua_glue_handle_arg(2, ARGTYPE2)\
    sn__lua_glue_handle_arg(3, ARGTYPE3)\
    sn__lua_glue_handle_arg(4, ARGTYPE4)\
    sn__lua_glue_handle_arg(5, ARGTYPE5)\
    sn__lua_glue_return(5, NAME, RETURNTYPE)\
  sn__lua_glue_function_close()

#define decl_lua_glue_function_6(NAME, RETURNTYPE, ARGTYPE1, ARGTYPE2, ARGTYPE3, ARGTYPE4, ARGTYPE5, ARGTYPE6)\
  sn__lua_glue_function_open(NAME)\
    sn__lua_glue_decl_arg(1, ARGTYPE1)\
    sn__lua_glue_decl_arg(2, ARGTYPE2)\
    sn__lua_glue_decl_arg(3, ARGTYPE3)\
    sn__lua_glue_decl_arg(4, ARGTYPE4)\
    sn__lua_glue_decl_arg(5, ARGTYPE5)\
    sn__lua_glue_decl_arg(6, ARGTYPE6)\
    sn__lua_glue_handle_arg(1, ARGTYPE1)\
    sn__lua_glue_handle_arg(2, ARGTYPE2)\
    sn__lua_glue_handle_arg(3, ARGTYPE3)\
    sn__lua_glue_handle_arg(4, ARGTYPE4)\
    sn__lua_glue_handle_arg(5, ARGTYPE5)\
    sn__lua_glue_handle_arg(6, ARGTYPE6)\
    sn__lua_glue_return(6, NAME, RETURNTYPE)\
  sn__lua_glue_function_close()

#define decl_lua_glue_function_7(NAME, RETURNTYPE, ARGTYPE1, ARGTYPE2, ARGTYPE3, ARGTYPE4, ARGTYPE5, ARGTYPE6, ARGTYPE7)\
  sn__lua_glue_function_open(NAME)\
    sn__lua_glue_decl_arg(1, ARGTYPE1)\
    sn__lua_glue_decl_arg(2, ARGTYPE2)\
    sn__lua_glue_decl_arg(3, ARGTYPE3)\
    sn__lua_glue_decl_arg(4, ARGTYPE4)\
    sn__lua_glue_decl_arg(5, ARGTYPE5)\
    sn__lua_glue_decl_arg(6, ARGTYPE6)\
    sn__lua_glue_decl_arg(7, ARGTYPE7)\
    sn__lua_glue_handle_arg(1, ARGTYPE1)\
    sn__lua_glue_handle_arg(2, ARGTYPE2)\
    sn__lua_glue_handle_arg(3, ARGTYPE3)\
    sn__lua_glue_handle_arg(4, ARGTYPE4)\
    sn__lua_glue_handle_arg(5, ARGTYPE5)\
    sn__lua_glue_handle_arg(6, ARGTYPE6)\
    sn__lua_glue_handle_arg(7, ARGTYPE7)\
    sn__lua_glue_return(7, NAME, RETURNTYPE)\
  sn__lua_glue_function_close()

#define decl_lua_glue_function_8(NAME, RETURNTYPE, ARGTYPE1, ARGTYPE2, ARGTYPE3, ARGTYPE4, ARGTYPE5, ARGTYPE6, ARGTYPE7, ARGTYPE8)\
  sn__lua_glue_function_open(NAME)\
    sn__lua_glue_decl_arg(1, ARGTYPE1)\
    sn__lua_glue_decl_arg(2, ARGTYPE2)\
    sn__lua_glue_decl_arg(3, ARGTYPE3)\
    sn__lua_glue_decl_arg(4, ARGTYPE4)\
    sn__lua_glue_decl_arg(5, ARGTYPE5)\
    sn__lua_glue_decl_arg(6, ARGTYPE6)\
    sn__lua_glue_decl_arg(7, ARGTYPE7)\
    sn__lua_glue_decl_arg(8, ARGTYPE8)\
    sn__lua_glue_handle_arg(1, ARGTYPE1)\
    sn__lua_glue_handle_arg(2, ARGTYPE2)\
    sn__lua_glue_handle_arg(3, ARGTYPE3)\
    sn__lua_glue_handle_arg(4, ARGTYPE4)\
    sn__lua_glue_handle_arg(5, ARGTYPE5)\
    sn__lua_glue_handle_arg(6, ARGTYPE6)\
    sn__lua_glue_handle_arg(7, ARGTYPE7)\
    sn__lua_glue_handle_arg(8, ARGTYPE8)\
    sn__lua_glue_return(8, NAME, RETURNTYPE)\
  sn__lua_glue_function_close()

#define decl_lua_glue_function_9(NAME, RETURNTYPE, ARGTYPE1, ARGTYPE2, ARGTYPE3, ARGTYPE4, ARGTYPE5, ARGTYPE6, ARGTYPE7, ARGTYPE8, ARGTYPE9)\
  sn__lua_glue_function_open(NAME)\
    sn__lua_glue_decl_arg(1, ARGTYPE1)\
    sn__lua_glue_decl_arg(2, ARGTYPE2)\
    sn__lua_glue_decl_arg(3, ARGTYPE3)\
    sn__lua_glue_decl_arg(4, ARGTYPE4)\
    sn__lua_glue_decl_arg(5, ARGTYPE5)\
    sn__lua_glue_decl_arg(6, ARGTYPE6)\
    sn__lua_glue_decl_arg(7, ARGTYPE7)\
    sn__lua_glue_decl_arg(8, ARGTYPE8)\
    sn__lua_glue_decl_arg(9, ARGTYPE9)\
    sn__lua_glue_handle_arg(1, ARGTYPE1)\
    sn__lua_glue_handle_arg(2, ARGTYPE2)\
    sn__lua_glue_handle_arg(3, ARGTYPE3)\
    sn__lua_glue_handle_arg(4, ARGTYPE4)\
    sn__lua_glue_handle_arg(5, ARGTYPE5)\
    sn__lua_glue_handle_arg(6, ARGTYPE6)\
    sn__lua_glue_handle_arg(7, ARGTYPE7)\
    sn__lua_glue_handle_arg(8, ARGTYPE8)\
    sn__lua_glue_handle_arg(9, ARGTYPE9)\
    sn__lua_glue_return(9, NAME, RETURNTYPE)\
  sn__lua_glue_function_close()

#define decl_lua_glue_function_10(NAME, RETURNTYPE, ARGTYPE1, ARGTYPE2, ARGTYPE3, ARGTYPE4, ARGTYPE5, ARGTYPE6, ARGTYPE7, ARGTYPE8, ARGTYPE9, ARGTYPE10)\
  sn__lua_glue_function_open(NAME)\
    sn__lua_glue_decl_arg(1, ARGTYPE1)\
    sn__lua_glue_decl_arg(2, ARGTYPE2)\
    sn__lua_glue_decl_arg(3, ARGTYPE3)\
    sn__lua_glue_decl_arg(4, ARGTYPE4)\
    sn__lua_glue_decl_arg(5, ARGTYPE5)\
    sn__lua_glue_decl_arg(6, ARGTYPE6)\
    sn__lua_glue_decl_arg(7, ARGTYPE7)\
    sn__lua_glue_decl_arg(8, ARGTYPE8)\
    sn__lua_glue_decl_arg(9, ARGTYPE9)\
    sn__lua_glue_decl_arg(10, ARGTYPE10)\
    sn__lua_glue_handle_arg(1, ARGTYPE1)\
    sn__lua_glue_handle_arg(2, ARGTYPE2)\
    sn__lua_glue_handle_arg(3, ARGTYPE3)\
    sn__lua_glue_handle_arg(4, ARGTYPE4)\
    sn__lua_glue_handle_arg(5, ARGTYPE5)\
    sn__lua_glue_handle_arg(6, ARGTYPE6)\
    sn__lua_glue_handle_arg(7, ARGTYPE7)\
    sn__lua_glue_handle_arg(8, ARGTYPE8)\
    sn__lua_glue_handle_arg(9, ARGTYPE9)\
    sn__lua_glue_handle_arg(10, ARGTYPE10)\
    sn__lua_glue_return(10, NAME, RETURNTYPE)\
  sn__lua_glue_function_close()

#endif /* end __LUAGLUE_H__ include guard */
