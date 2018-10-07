#include "textures.hpp"

#include "lua.hpp"
#include "lualib.h"
#include "lauxlib.h"

#include "gl_lite.h"

struct texture {
	GLuint id;
};
static texture* check(lua_State* L, int id) { return *reinterpret_cast<texture**>(luaL_checkudata(L, id, "texture")); }
static int use_texture(lua_State* L)
{
	auto s = check(L, 1);
	auto num=luaL_checkint(L, 2);
	glActiveTexture(num+GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, s->id);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	return 0;
}
struct gl_tex_format {
	GLint internal_format;
	GLint format;
	GLenum type;
};
static const gl_tex_format formats[] = {
	{ GL_RGBA8,GL_RGBA,GL_UNSIGNED_BYTE },
	{ GL_RGBA32F,GL_RGBA,GL_FLOAT },
	{ GL_R32F,GL_RED,GL_FLOAT },
};
static int set_texture_data(lua_State* L)
{
	auto s = check(L, 1);
	auto data = lua_topointer(L, 2); //TODO: check pointer?
	auto w = luaL_checkint(L, 3);
	auto h = luaL_checkint(L, 4);
	auto format = luaL_optint(L, 5, 0);

	auto f = formats[format];
	glTexImage2D(GL_TEXTURE_2D, 0, f.internal_format, w, h, 0, f.format, f.type, data);
	return 0;
}
static int del_texture(lua_State* L)
{
	auto s = check(L, 1);
	glDeleteTextures(1, &s->id);
	return 0;
}

static int make_lua_texture(lua_State* L)
{
	auto ret = new texture;
	auto np = lua_newuserdata(L, sizeof(ret));
	*reinterpret_cast<texture**>(np) = ret;
	glGenTextures(1,&ret->id);

	if (luaL_newmetatable(L, "texture"))
	{
		lua_pushcfunction(L, del_texture);
		lua_setfield(L, -2, "__gc");

		lua_pushcfunction(L, use_texture);
		lua_setfield(L, -2, "use");

		lua_pushcfunction(L, set_texture_data);
		lua_setfield(L, -2, "set");

		lua_pushvalue(L, -1);
		lua_setfield(L, -2, "__index");
	}
	lua_setmetatable(L, -2);
	return 1;
}

static const luaL_Reg lua_textures_lib[] = {
	{ "Make",make_lua_texture },
	{ NULL, NULL }
};

int lua_open_textures(lua_State * L)
{
	luaL_newlib(L, lua_textures_lib);

	lua_setglobal(L, "textures");

	return 1;
}