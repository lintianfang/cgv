#pragma once

#include "time_series.h"
#include <plot/plot_base.h>
#include <cgv/render/render_types.h>
#include <cgv/render/texture.h>
#include <cgv/render/render_buffer.h>
#include <cgv/render/frame_buffer.h>

namespace stream_vis {

	/// store multiple accesses to a time series
	struct attribute_definition
	{
		///
		uint16_t  time_series_index : 16;
		///
		TimeSeriesAccessor accessor : 16;
	};

	/// reference to a component of an indexed quantity
	struct component_reference
	{
		///
		uint16_t index : 16;
		///
		uint16_t component_index : 16;
		/// constructor
		component_reference(uint16_t _index = 0, uint16_t _component_index = 0) { index = _index; component_index = _component_index; }
	};

	/// for each subplot we store per attribute its definition and reference to the ringbuffer where the data is stored
	struct subplot_info
	{
		std::vector<attribute_definition> attribute_definitions;
		std::vector<component_reference> ringbuffer_references;
	};

	enum DomainAdjustment
	{
		DA_FIXED,
		DA_COMPUTE,
		DA_TIME_SERIES
	};

	/// per plot information
	struct plot_info : public cgv::render::render_types
	{
		/// flag used to check whether offline rendering of 2d plot has to be performed
		bool outofdate;
		/// plot dimension
		int dim;
		/// plot name
		std::string name;
		/// fixed domain definition via view_min and view_max
		box3 fixed_domain;
		/// for each domain component the mode of adjustment
		DomainAdjustment domain_adjustment[2][3];
		/// for each domain component the index of the time series used for adjustment
		uint16_t domain_bound_ts_index[2][3];
		/// flags telling which domain bounds have been set 
		ivec2 offline_texture_resolution;
		/// extent of 2d plot in texture
		vec2 extent_on_texture;
		/// pointer to plot instance
		cgv::plot::plot_base* plot_ptr;
		/// subplot information
		std::vector<subplot_info> subplot_infos;
		// texture for offline rendering of 2d plots
		cgv::render::texture tex;
		// depth buffer for offline rendering of 2d plots
		cgv::render::render_buffer depth;
		// framebuffer obect for offline rendering of 2d plots
		cgv::render::frame_buffer fbo;
	};

}
