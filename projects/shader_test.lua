
require "common"
local size=STATE.size
img_buf=img_buf or make_image_buffer(size[1],size[2])
visits=visits or make_float_buffer(size[1],size[2])
function resize( w,h )
	img_buf=make_image_buffer(size[1],size[2])
	visits=make_float_buffer(size[1],size[2])
end

local main_shader=shaders.Make[[
#version 330

out vec4 color;
in vec3 pos;

uniform vec2 res;
uniform sampler2D tex_main;

void main(){
	float v=(sin(pos.x*res.x/4)+1)/2;
	float v2=(sin(pos.y*res.y/4)+1)/2;
	vec2 normed=(pos.xy+vec2(1,1))/2;
	vec4 t=texture(tex_main,normed);
	color = vec4(t.r,t.g,t.b,1);
}
]]
local main_tex = textures.Make()
function update(  )
	__no_redraw()
	__clear()
	for i=1,10000 do
		img_buf:set(math.random(0,size[1]-1),math.random(0,size[2]-1),{math.random(0,255),math.random(0,255),math.random(0,255),255})
	end
	

	--texture_fb:read() -- read framebuffer
	main_shader:use()
	main_tex:use(0)
	main_tex:set(img_buf.d,img_buf.w,img_buf.h)
	main_shader:set("res",STATE.size[1],STATE.size[2])
	main_shader:set_i("tex_main",0)
	main_shader:draw_quad()
	--draw(texture1) --draw into texture1
	--other_shader:use()
	--other_shader:set("s1",texture1)
	--draw() --draw to framebuff
end