#pragma once

#include "arrow_renderer.h"
#include "render_data_base.h"

namespace cgv {
namespace render {

template <typename ColorType = rgb>
class arrow_render_data : public render_data_base<ColorType> {
public:
	// Base class we're going to use virtual functions from
	typedef render_data_base<ColorType> super;

	/// stores an array of directions
	std::vector<vec3> directions;
	/// whether to interpret the direction attribute as the arrow end point position
	bool direction_is_end_point = false;

protected:
	/// @brief See render_data_base::transfer.
	bool transfer(context& ctx, arrow_renderer& r) {
		if(super::transfer(ctx, r)) {
			if(direction_is_end_point) {
				CGV_RDB_TRANSFER_ARRAY(end_point, directions);
			} else {
				CGV_RDB_TRANSFER_ARRAY(direction, directions);
			}
			return true;
		}
		return false;
	}

public:
	void clear() {
		super::clear();
		directions.clear();
	}

	void add_direction(const vec3& direction) {
		directions.push_back(direction);
	}

	// Explicitly use add from the base class since it is shadowed by the overloaded versions
	using super::add;

	void add(const vec3& position, const vec3& direction) {
		super::add_position(position);
		add_direction(direction);
	}

	void add(const vec3& position, const ColorType& color, const vec3& direction) {
		super::add(position, color);
		add_direction(direction);
	}

	void fill_directions(const vec3& direction) {
		super::fill(directions, direction);
	}

	CGV_RDB_BASE_FUNC_DEF(arrow_renderer, arrow_render_style);
};

}
}
