#pragma once
#include <cgv/render/context.h>
#include <cgv_gl/point_renderer.h>
#include <atomic>
#include <mutex>

#include "gl/lib_begin.h"

// [WIP] clod point cloud renderer

namespace cgv {
	namespace render {

		/** render style for sphere rendere */
		struct CGV_API clod_point_render_style : public point_render_style
		{
			/*@name clod rendering attributes*/
			//@{
			float CLOD = 1.f;
			float spacing = 1.f;
			float scale = 1.f;
			float min_millimeters = 1.f;
			float pointSize = 1.f;
			//@}

			/// construct with default values
			clod_point_render_style();
		};

		class CGV_API clod_point_renderer : public cgv::render::renderer {

			struct Vertex {
				vec3 position;
				rgb8 colors;
				uint8_t level = 0;
			};

			struct DrawParameters {
				GLuint  count = 0; //element count
				GLuint  primCount = 1;
				GLuint  first = 0;
				GLuint  baseInstance = 0;
			};

			struct State {
				std::atomic<int64_t> pointsTotal = 0;
				std::atomic<int64_t> pointsProcessed = 0;

				int numPasses = 3;
				int currentPass = 0; // starts with index 1! interval: [1,  numPasses]

				std::mutex mtx;

				double progress() {
					return double(pointsProcessed) / double(pointsTotal);
				}
			};


			//float CLOD = 1.f;
			//float spacing = 1.f; //root spacing
			//float scale = 1.f;
			
			shader_program reduce_prog;
			shader_program draw_prog;

			std::vector<vec3> positions;
			std::vector<Vertex> input_buffer_data;
			std::vector<rgba8> colors; //alpha channel is later used for storing the clod level
			GLuint vertex_array = 0;
			GLuint input_buffer = 0, render_buffer = 0, draw_parameter_buffer = 0;
			const int input_pos = 0, render_pos = 1, drawp_pos = 3;
			bool buffers_outofdate = true;

		protected:

			void lod_chunking();
			std::vector<std::atomic<unsigned>> lod_counting(std::vector<vec3> positions, vec3 min, vec3 max, int64_t gridSize, State& state);

			void draw_and_compute_impl(context& ctx, PrimitiveType type, size_t start, size_t count, bool use_strips, bool use_adjacency, uint32_t strip_restart_index);
		public:
			render_style* create_render_style() const;

			bool init(context& ctx);

			bool enable(context& ctx);
			
			bool disable(context& ctx);

			void draw(context& ctx, size_t start, size_t count,
				bool use_strips = false, bool use_adjacency = false, uint32_t strip_restart_index = -1);
			
			void generate_lods();

			void set_positions(context& ctx, std::vector<vec3> positions) {
				this->positions = positions;
				input_buffer_data.resize(positions.size());
				for (int i = 0; i < positions.size(); ++i) {
					input_buffer_data[i].position = positions[i];
				}
				buffers_outofdate = true;
			}
			template<typename T>
			void set_colors(const context& ctx, const std::vector<T>& colors) {
				//input_buffer_data.resize(colors.size());
				for (int i = 0; i < positions.size(); ++i) {
					input_buffer_data[i].colors = rgb(colors[i]);
				}
				buffers_outofdate = true;
			}

			template<typename T>
			void set_lods(const std::vector<T>& lod) {
				//input_buffer_data.resize(lod.size());
				for (int i = 0; i < lod.size(); ++i) {
					input_buffer_data[i].level = lod[i]; //set LOD level (lower levels should be more coarse than higher levels)
				}
				buffers_outofdate = true;
			}

		private:
			void add_shader(context& ctx, shader_program& prog, const std::string& sf, const cgv::render::ShaderType st);
			void fill_buffers(context& ctx);
			void clear_buffers(context& ctx);
		};
	}
}
#include <cgv/config/lib_end.h>