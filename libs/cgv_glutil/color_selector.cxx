#include "color_selector.h"

#include <cgv/gui/mouse_event.h>
#include <cgv/gui/theme_info.h>
#include <cgv_gl/gl/gl.h>

namespace cgv {
namespace glutil {

color_selector::color_selector() {

	set_name("Color Selector");

	layout.padding = 13; // 10px plus 3px border

	set_overlay_alignment(AO_END, AO_END);
	set_overlay_stretch(SO_NONE);
	set_overlay_margin(ivec2(-3));
	set_overlay_size(ivec2(300, 280));
	
	register_shader("rectangle", canvas::shaders_2d::rectangle);
	register_shader("circle", canvas::shaders_2d::circle);
	
	color_points.set_constraint(layout.color_rect);
	color_points.set_drag_callback(std::bind(&color_selector::handle_color_point_drag, this));

	hue_points.set_constraint(layout.hue_rect);
	hue_points.set_drag_callback(std::bind(&color_selector::handle_hue_point_drag, this));
}

void color_selector::clear(cgv::render::context& ctx) {

	canvas_overlay::clear(ctx);

	color_tex.destruct(ctx);
	hue_tex.destruct(ctx);

	ref_msdf_font(ctx, -1);
	ref_msdf_gl_canvas_font_renderer(ctx, -1);
}

bool color_selector::handle_event(cgv::gui::event& e) {

	// return true if the event gets handled and stopped here or false if you want to pass it to the next plugin
	unsigned et = e.get_kind();
	unsigned char modifiers = e.get_modifiers();

	if (!show)
		return false;

	if (et == cgv::gui::EID_MOUSE) {
		cgv::gui::mouse_event& me = (cgv::gui::mouse_event&)e;
		cgv::gui::MouseAction ma = me.get_action();

		if (me.get_button_state() & cgv::gui::MB_LEFT_BUTTON) {
			if (ma == cgv::gui::MA_PRESS) {
				ivec2 mpos = get_local_mouse_pos(ivec2(me.get_x(), me.get_y()));

				bool hit = false;

				if(layout.color_rect.is_inside(mpos)) {
					ivec2 local_mpos = mpos - layout.color_rect.pos();
					vec2 val = static_cast<vec2>(local_mpos) / static_cast<vec2>(layout.color_rect.size());
					color_points[0].val = val;
					color_points[0].update_pos(layout);
					hit = true;
				}

				if(layout.hue_rect.is_inside(mpos)) {
					ivec2 local_mpos = mpos - layout.color_rect.pos();
					float val = static_cast<float>(local_mpos.y()) / static_cast<float>(layout.color_rect.size().y());
					hue_points[0].val = val;
					hue_points[0].update_pos(hue_points.get_constraint());
					update_color_texture();
					hit = true;
				}

				if(hit) {
					update_color();
					has_damage = true;
					post_redraw();
				}
			}
		}

		if(color_points.handle(e, last_viewport_size, container))
			return true;
		if(hue_points.handle(e, last_viewport_size, container))
			return true;
	}
	return false;
}

void color_selector::on_set(void* member_ptr) {

	if(member_ptr == &color) {
		set_color(color);
	}

	has_damage = true;
	update_member(member_ptr);
	post_redraw();
}

bool color_selector::init(cgv::render::context& ctx) {
	
	bool success = canvas_overlay::init(ctx);

	msdf_font& font = ref_msdf_font(ctx, 1);
	ref_msdf_gl_canvas_font_renderer(ctx, 1);

	if(success)
		init_styles(ctx);
	
	init_textures(ctx);
	
	if(font.is_initialized()) {
		texts.set_msdf_font(&font);
		texts.set_font_size(14.0f);

		texts.add_text("R: ", ivec2(0), TA_BOTTOM_LEFT);
		texts.add_text("0", ivec2(0), TA_BOTTOM_RIGHT);
		texts.add_text("G: ", ivec2(0), TA_BOTTOM_LEFT);
		texts.add_text("0", ivec2(0), TA_BOTTOM_RIGHT);
		texts.add_text("B: ", ivec2(0), TA_BOTTOM_LEFT);
		texts.add_text("0", ivec2(0), TA_BOTTOM_RIGHT);
	}

	color_point cp;
	cp.val = vec2(0.0f);
	color_points.add(cp);

	hue_point hp;
	hp.val = 0.0f;
	hue_points.add(hp);

	set_color(rgb(0.0f));

	return success;
}

void color_selector::init_frame(cgv::render::context& ctx) {

	if(ensure_layout(ctx)) {
		ivec2 container_size = get_overlay_size();
		layout.update(container_size);

		color_points.set_constraint(layout.color_rect);

		rect hue_constraint = layout.hue_rect;
		hue_constraint.set_pos(ivec2(hue_constraint.pos().x(), hue_constraint.pos().y() - 5.0f));
		hue_constraint.set_size(ivec2(0, hue_constraint.size().y()));
		hue_points.set_constraint(hue_constraint);

		color_points[0].update_pos(layout);
		hue_points[0].update_pos(hue_constraint);

		ivec2 text_position = ivec2(layout.preview_rect.b().x() + 10, layout.preview_rect.y() + 5);
		for(size_t i = 0; i < texts.size(); ++i) {
			texts.set_position(i, text_position);
			text_position.x() += i & 1 ? 15 : 40;
		}
	}

	if(ensure_theme())
		init_styles(ctx);
}

void color_selector::draw_content(cgv::render::context& ctx) {
	
	begin_content(ctx);
	enable_blending();

	ivec2 container_size = get_overlay_size();

	// draw container
	auto& rect_prog = content_canvas.enable_shader(ctx, "rectangle");
	container_style.apply(ctx, rect_prog);
	content_canvas.draw_shape(ctx, ivec2(0), container_size);

	// draw inner border
	border_style.apply(ctx, rect_prog);

	auto& ti = cgv::gui::theme_info::instance();
	rgba border_color = rgba(ti.border(), 1.0f);
	rgba background_color = rgba(ti.background(), 1.0f);
	content_canvas.draw_shape(ctx, layout.border_rect, border_color);
	content_canvas.draw_shape(ctx, layout.preview_rect, color);

	if(texts.size() > 5) {
		rect text_bg = layout.preview_rect;
		text_bg.set_w(48);
		for(size_t i = 0; i < 3; ++i) {
			text_bg.set_x(texts.ref_texts()[2*i].position.x() - 4);
			content_canvas.draw_shape(ctx, text_bg, background_color);
		}
	}

	color_texture_style.apply(ctx, rect_prog);
	color_tex.enable(ctx, 0);
	content_canvas.draw_shape(ctx, layout.color_rect.pos(), layout.color_rect.size());
	color_tex.disable(ctx);

	hue_texture_style.apply(ctx, rect_prog);
	hue_tex.enable(ctx, 0);
	content_canvas.draw_shape(ctx, layout.hue_rect.pos(), layout.hue_rect.size());
	hue_tex.disable(ctx);

	content_canvas.disable_current_shader(ctx);

	glEnable(GL_SCISSOR_TEST);
	glScissor(layout.color_rect.x(), layout.color_rect.y(), layout.color_rect.w(), layout.color_rect.h());

	const auto& cp = color_points[0];
	auto& circle_prog = content_canvas.enable_shader(ctx, "circle");
	color_handle_style.apply(ctx, circle_prog);
	content_canvas.draw_shape(ctx, cp.get_render_position(), cp.get_render_size());
	content_canvas.disable_current_shader(ctx);

	glScissor(layout.hue_rect.x(), layout.hue_rect.y(), layout.hue_rect.w(), layout.hue_rect.h());

	const auto& hp = hue_points[0];
	rect_prog = content_canvas.enable_shader(ctx, "rectangle");
	hue_handle_style.apply(ctx, rect_prog);
	content_canvas.draw_shape(ctx, hp.get_render_position(), hp.get_render_size());
	content_canvas.disable_current_shader(ctx);
	
	glDisable(GL_SCISSOR_TEST);

	ref_msdf_gl_canvas_font_renderer(ctx).render(ctx, content_canvas, texts, text_style);

	disable_blending();
	end_content(ctx);
}

void color_selector::create_gui() {

	create_overlay_gui();

	if(begin_tree_node("Settings", layout, false)) {
		align("\a");
		//std::string height_options = "min=";
		//height_options += supports_opacity ? "80" : "40";
		//height_options += ";max=500;step=10;ticks=true";
		//add_member_control(this, "Height", layout.total_height, "value_slider", height_options);
		//add_member_control(this, "Resolution", resolution, "dropdown", "enums='2=2,4=4,8=8,16=16,32=32,64=64,128=128,256=256,512=512,1024=1024,2048=2048'");
		//add_member_control(this, "Opacity Scale Exponent", opacity_scale_exponent, "value_slider", "min=1.0;max=5.0;step=0.001;ticks=true");
		align("\b");
		end_tree_node(layout);
	}

	add_member_control(this, "Color", color);

	/*if(begin_tree_node("Color Points", color_points, true)) {
		align("\a");
		auto& points = color_points;
		for(unsigned i = 0; i < points.size(); ++i)
			add_member_control(this, "#" + std::to_string(i), points[i].col, "", &points[i] == color_points.get_selected() ? "label_color=" + highlight_color_hex : "");
		align("\b");
		end_tree_node(color_points);
	}

	if(begin_tree_node("Opacity Points", hue_points, true)) {
		align("\a");
		auto& points = hue_points;
		for(unsigned i = 0; i < points.size(); ++i)
			add_member_control(this, "#" + std::to_string(i), points[i].val[1], "", &points[i] == hue_points.get_selected() ? "label_color=" + highlight_color_hex : "");
		align("\b");
		end_tree_node(hue_points);
	}*/
}

bool color_selector::was_updated() {
	bool temp = has_updated;
	has_updated = false;
	return temp;
}

void color_selector::set_color(rgb color) {
	
	this->color = color;

	float h = color.H();
	float v = v = color.S() * std::min(color.L(), 1.0f - color.L()) + color.L();
	float s = v ? 2.0f - 2.0f * color.L() / v : 0.0f;

	color_points[0].val = vec2(s, v);
	hue_points[0].val = h;

	color_points[0].update_pos(layout);
	hue_points[0].update_pos(hue_points.get_constraint());

	update_color_texture();
	update_texts();

	update_member(&color);
	has_damage = true;
	post_redraw();
}

void color_selector::init_styles(context& ctx) {
	// get theme colors
	auto& ti = cgv::gui::theme_info::instance();
	rgba background_color = rgba(ti.background(), 1.0f);
	rgba group_color = rgba(ti.group(), 1.0f);
	rgba border_color = rgba(ti.border(), 1.0f);

	// configure style for the container rectangle
	container_style.apply_gamma = false;
	container_style.fill_color = group_color;
	container_style.border_color = background_color;
	container_style.border_width = 3.0f;
	container_style.feather_width = 0.0f;
	
	// configure style for the border rectangles
	border_style = container_style;
	//border_style.fill_color = border_color;
	border_style.border_width = 0.0f;
	border_style.use_fill_color = false;
	
	// configure style for the color and hue texture rectangles
	color_texture_style = border_style;
	color_texture_style.use_texture = true;
	
	hue_texture_style = color_texture_style;
	color_texture_style.texcoord_scaling = vec2(0.5f);
	color_texture_style.texcoord_offset = vec2(0.25f);

	// configure style for color handle
	color_handle_style.use_blending = true;
	color_handle_style.apply_gamma = false;
	color_handle_style.use_fill_color = true;
	color_handle_style.position_is_center = true;
	color_handle_style.border_color = rgba(rgb(1.0f), 0.75f);
	color_handle_style.border_width = 1.0f;
	color_handle_style.fill_color = rgba(rgb(0.0f), 1.0f);
	color_handle_style.ring_width = 2.5f;

	hue_handle_style = color_handle_style;
	hue_handle_style.position_is_center = false;
	
	// configure style for hue handle
	cgv::glutil::shape2d_style hue_handle_style;
	hue_handle_style.use_blending = true;
	hue_handle_style.apply_gamma = false;
	hue_handle_style.use_fill_color = false;
	hue_handle_style.position_is_center = true;
	hue_handle_style.border_color = rgba(ti.border(), 1.0f);
	hue_handle_style.border_width = 1.5f;

	// configure text style
	float label_border_alpha = 0.0f;
	float border_width = 0.25f;
	
	text_style.fill_color = rgba(ti.text(), 1.0f);
	text_style.border_color = rgba(ti.text(), label_border_alpha);
	text_style.border_width = border_width;
	text_style.feather_origin = 0.5f;
	text_style.use_blending = true;
}

void color_selector::init_textures(context& ctx) {

	std::vector<uint8_t> data(3*4, 0u);

	data[6] = 255u;
	data[7] = 255u;
	data[8] = 255u;
	data[9] = 255u;
	
	color_tex.destruct(ctx);
	cgv::data::data_view color_dv = cgv::data::data_view(new cgv::data::data_format(2, 2, TI_UINT8, cgv::data::CF_RGB), data.data());
	color_tex = texture("uint8[R,G,B]", TF_LINEAR, TF_LINEAR);
	color_tex.create(ctx, color_dv, 0);

	std::vector<uint8_t> hue_data(2*3*256);

	for(size_t i = 0; i < 256; ++i) {
		float hue = static_cast<float>(i) / 255.0f;
		rgb color = cgv::media::color<float, cgv::media::HLS>(hue, 0.5f, 1.0f);

		uint8_t col8[3];
		col8[0] = static_cast<uint8_t>(round(color.R() * 255.0f));
		col8[1] = static_cast<uint8_t>(round(color.G() * 255.0f));
		col8[2] = static_cast<uint8_t>(round(color.B() * 255.0f));

		hue_data[6 * i + 0] = col8[0];
		hue_data[6 * i + 1] = col8[1];
		hue_data[6 * i + 2] = col8[2];
		hue_data[6 * i + 3] = col8[0];
		hue_data[6 * i + 4] = col8[1];
		hue_data[6 * i + 5] = col8[2];
	}

	hue_tex.destruct(ctx);
	cgv::data::data_view hue_dv = cgv::data::data_view(new cgv::data::data_format(2, 256, TI_UINT8, cgv::data::CF_RGB), hue_data.data());
	hue_tex = texture("uint8[R,G,B]", TF_LINEAR, TF_LINEAR);
	hue_tex.create(ctx, hue_dv, 0);
}

void color_selector::update_color_texture() {

	if(!color_tex.is_created())
		return;
	
	std::vector<uint8_t> data(3 * 4, 0u);
	data[6] = 255u;
	data[7] = 255u;
	data[8] = 255u;
	
	const auto& hp = hue_points[0];
	rgb color = cgv::media::color<float, cgv::media::HLS>(hp.val, 0.5f, 1.0f);

	data[9]  = static_cast<uint8_t>(round(color.R() * 255.0f));
	data[10] = static_cast<uint8_t>(round(color.G() * 255.0f));
	data[11] = static_cast<uint8_t>(round(color.B() * 255.0f));

	cgv::data::data_view color_dv = cgv::data::data_view(new cgv::data::data_format(2, 2, TI_UINT8, cgv::data::CF_RGB), data.data());

	if(auto* ctx_ptr = get_context())
		color_tex.replace(*ctx_ptr, 0, 0, color_dv);
}

void color_selector::update_color() {

	const auto& hp = hue_points[0];
	const auto& cp = color_points[0];
	color = cgv::media::color<float, cgv::media::HLS>(hp.val, 0.5f, 1.0f);

	float s = cp.val.x(); // saturation
	float v = cp.val.y(); // value

	color = v * color;
	color = (1.0f - s)*rgb(v) + s * color;

	update_texts();

	update_member(&color);
	has_updated = true;
}

void color_selector::update_texts() {

	ivec3 components;
	components[0] = static_cast<int>(round(color.R() * 255.0f));
	components[1] = static_cast<int>(round(color.G() * 255.0f));
	components[2] = static_cast<int>(round(color.B() * 255.0f));

	components = cgv::math::clamp(components, 0, 255);

	if(texts.size() > 5) {
		texts.set_text(1, std::to_string(components[0]));
		texts.set_text(3, std::to_string(components[1]));
		texts.set_text(5, std::to_string(components[2]));
	}
}

void color_selector::handle_color_point_drag() {

	auto* p = color_points.get_dragged();
	p->update_val(layout);
	
	update_color();

	has_damage = true;
	post_redraw();
}

void color_selector::handle_hue_point_drag() {

	auto* p = hue_points.get_dragged();
	p->update_val(hue_points.get_constraint());
	
	update_color();
	update_color_texture();

	has_damage = true;
	post_redraw();
}

}
}
