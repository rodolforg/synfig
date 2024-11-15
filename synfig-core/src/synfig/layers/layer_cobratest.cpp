/* === S Y N F I G ========================================================= */
/*!	\file layer_cobratest.cpp
**	\brief A layer to test Cobra renderer stuff
**
**	\legal
**	Copyright (c) 2002-2005 Robert B. Quattlebaum Jr., Adrian Bentley
**
**	This file is part of Synfig.
**
**	Synfig is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 2 of the License, or
**	(at your option) any later version.
**
**	Synfig is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with Synfig.  If not, see <https://www.gnu.org/licenses/>.
**	\endlegal
*/
/* ========================================================================= */

/* === H E A D E R S ======================================================= */

#ifdef USING_PCH
#	include "pch.h"
#else
#ifdef HAVE_CONFIG_H
#	include <config.h>
#endif

#include "layer_cobratest.h"

#include <synfig/context.h>
#include <synfig/surface.h>
#include <synfig/renddesc.h>
#include <synfig/rendering/task.h>
#include <synfig/rendering/software/task/tasksw.h>
#include <synfig/target.h>


#include <synfig/localization.h>
#include <synfig/paramdesc.h>

#endif

/* === U S I N G =========================================================== */

using namespace synfig;

/* === M A C R O S ========================================================= */

/* === G L O B A L S ======================================================= */

SYNFIG_LAYER_INIT(Layer_CobraTest);
SYNFIG_LAYER_SET_NAME(Layer_CobraTest,"cobra_test");
SYNFIG_LAYER_SET_LOCAL_NAME(Layer_CobraTest,N_("Cobra test"));
SYNFIG_LAYER_SET_CATEGORY(Layer_CobraTest,N_("Example"));
SYNFIG_LAYER_SET_VERSION(Layer_CobraTest,"0.1");

/* === P R O C E D U R E S ================================================= */


class TaskCobraTest
	: public rendering::Task
{
public:
	typedef etl::handle<TaskCobraTest> Handle;
	static Token token;
	Token::Handle get_token() const override { return token.handle(); }

	// Rect
	// compute_required_source_rect(const Rect& source_rect, const Matrix& inv_matrix) const override
	// {
	// 	//return Rect(-10,-10,10,10);
	// 	if (!internal.distort_outside)
	// 		return source_rect;

	// 	const int tw = target_rect.get_width();
	// 	const int th = target_rect.get_height();
	// 	Vector dx = inv_matrix.axis_x();
	// 	Vector dy = inv_matrix.axis_y() - dx*(Real)tw;
	// 	Vector p = inv_matrix.get_transformed( Vector((Real)target_rect.minx, (Real)target_rect.miny) );

	// 	Rect sub_source_rect = source_rect;

	// 	// Check from where the boundary pixels come in source context (before transform)
	// 	// vertical borders
	// 	for (int iy = target_rect.miny; iy < target_rect.maxy; ++iy, p[1] += dy[1]) {
	// 		Point tmp = internal.transform(p);
	// 		sub_source_rect.expand(tmp);
	// 		tmp = internal.transform(Point(p[0] + dx[0]*(Real)tw, p[1]));
	// 		sub_source_rect.expand(tmp);
	// 	}

	// 	// horizontal borders
	// 	for (int ix = target_rect.minx; ix < target_rect.maxx; ++ix, p[0] += dx[0]) {
	// 		Point tmp = internal.transform(p);
	// 		sub_source_rect.expand(tmp);
	// 		tmp = internal.transform(Point(p[0], p[1] - dy[1]*(Real)th));
	// 		sub_source_rect.expand(tmp);
	// 	}

	// 	return sub_source_rect;
	// }


	VectorInt
	get_offset() const
	{
		if (!sub_tasks[0]) return VectorInt::zero();
		Vector offset = (sub_task()->source_rect.get_min() - source_rect.get_min()).multiply_coords(get_pixels_per_unit());
		return VectorInt((int)round(offset[0]), (int)round(offset[1])) - sub_task()->target_rect.get_min();
	}

	Task::Handle&
	sub_task()
	{
		return Task::sub_task(0);
	}

	const Task::Handle&
	sub_task() const
	{
		return Task::sub_task(0);
	}

};

class TaskCobraTestSW
	: public TaskCobraTest, public rendering::TaskSW
{
public:
	typedef etl::handle<TaskCobraTest> Handle;
	static Token token;
	Token::Handle get_token() const override { return token.handle(); }

	Point
	point_vfunc(const Point& point) const
	{
		return point;
	}

	bool run(Task::RunParams& /*params*/) const override
	{
		//return run_task(*this);
		if (!sub_task())
			return true;

		RectInt r = target_rect;
		if (r.valid())
		{
			VectorInt offset = get_offset();

			RectInt ra = sub_task()->target_rect + r.get_min() + offset;
			if (ra.valid())
			{
				rect_set_intersect(ra, ra, r);
				if (ra.valid())
				{
					LockWrite ldst(this);
					if (!ldst) return false;
					LockRead lsrc(sub_task());
					if (!lsrc) return false;

					const synfig::Surface &a = lsrc->get_surface();
					synfig::Surface &c = ldst->get_surface();

					const Vector upp = get_units_per_pixel();
					const Vector sub_ppu = sub_tasks[0]->get_pixels_per_unit();

					// from raster r to 'world' w conversion: w = upp * r + (w0 * r1 + w1 * r0) / r_size
					Vector constant;
					constant[0] = (source_rect.get_min()[0] * target_rect.get_max()[0] - source_rect.get_max()[0] * target_rect.get_min()[0]) / target_rect.get_size()[0];
					constant[1] = (source_rect.get_min()[1] * target_rect.get_max()[1] - source_rect.get_max()[1] * target_rect.get_min()[1]) / target_rect.get_size()[1];
					Vector constant_;
					constant_[0] = (source_rect.get_min()[0] * target_rect.get_max()[0] - source_rect.get_max()[0] * target_rect.get_min()[0]);
					constant_[1] = (source_rect.get_min()[1] * target_rect.get_max()[1] - source_rect.get_max()[1] * target_rect.get_min()[1]);

					Vector sub_constant_w_r;
					sub_constant_w_r[0] = (sub_tasks[0]->source_rect.get_min()[0] * sub_tasks[0]->target_rect.get_max()[0] - sub_tasks[0]->source_rect.get_max()[0] * sub_tasks[0]->target_rect.get_min()[0]) / sub_tasks[0]->source_rect.get_size()[0];
					sub_constant_w_r[1] = (sub_tasks[0]->source_rect.get_min()[1] * sub_tasks[0]->target_rect.get_max()[1] - sub_tasks[0]->source_rect.get_max()[1] * sub_tasks[0]->target_rect.get_min()[1]) / sub_tasks[0]->source_rect.get_size()[1];
					Vector sub_constant_w_r_;
					sub_constant_w_r_[0] = (sub_tasks[0]->source_rect.get_min()[0] * sub_tasks[0]->target_rect.get_max()[0] - sub_tasks[0]->source_rect.get_max()[0] * sub_tasks[0]->target_rect.get_min()[0]);
					sub_constant_w_r_[1] = (sub_tasks[0]->source_rect.get_min()[1] * sub_tasks[0]->target_rect.get_max()[1] - sub_tasks[0]->source_rect.get_max()[1] * sub_tasks[0]->target_rect.get_min()[1]);

					// synfig::warning("source_rect: (%f, %f) , (%f, %f) [%f, %f]", source_rect.get_min()[0], source_rect.get_min()[1], source_rect.get_max()[0], source_rect.get_max()[1], source_rect.get_size()[0], source_rect.get_size()[1]);
					// synfig::warning("target_rect: (%i, %i) , (%i, %i) [%i, %i]", target_rect.get_min()[0], target_rect.get_min()[1], target_rect.get_max()[0], target_rect.get_max()[1], target_rect.get_size()[0], target_rect.get_size()[1]);
					// synfig::warning("a_rect: (%i, %i) , (%i, %i) [%i, %i]", ra.get_min()[0], ra.get_min()[1], ra.get_max()[0], ra.get_max()[1], ra.get_size()[0], ra.get_size()[1]);
					// synfig::warning("sub_source_rect: (%f, %f) , (%f, %f) [%f, %f]", sub_task()->source_rect.get_min()[0], sub_task()->source_rect.get_min()[1], sub_task()->source_rect.get_max()[0], sub_task()->source_rect.get_max()[1], sub_task()->source_rect.get_size()[0], sub_task()->source_rect.get_size()[1]);
					// synfig::warning("sub_target_rect: (%i, %i) , (%i, %i) [%i, %i]", sub_task()->target_rect.get_min()[0], sub_task()->target_rect.get_min()[1], sub_task()->target_rect.get_max()[0], sub_task()->target_rect.get_max()[1], sub_task()->target_rect.get_size()[0], sub_task()->target_rect.get_size()[1]);
					// synfig::warning("sub_ppu: %f, %f\tupp: %f, %f", sub_ppu[0], sub_ppu[1], upp[0], upp[1]);
					// // synfig::warning("sub ppu: %f, %f\tupp: %f, %f", sub_tasks[0]->get_pixels_per_unit()[0], sub_tasks[0]->get_pixels_per_unit()[1], upp[0], upp[1]);
					// synfig::warning("offset: %f, %f", offset[0], offset[1]);
					// synfig::warning("r->w: %f, %f\tw->r: %f, %f", constant[0], constant[1], sub_constant_w_r[0], sub_constant_w_r[1]);
					// synfig::warning("min: %f, %f\tmax: %f, %f", ra.minx - r.minx + offset[0], ra.miny - r.miny + offset[1], ra.maxx - r.minx + offset[0], ra.maxy - r.miny + offset[1]);
					// synfig::warning("uv: min: %f, %f\tmax: %f, %f", ra.minx * upp[0] + constant[0], ra.miny * upp[1] + constant[1], ra.maxx * upp[0] + constant[0], ra.maxy * upp[1] + constant[1]);
					// synfig::warning("XY: min: %f, %f\tmax: %f, %f", (ra.minx * upp[0] + constant[0])* sub_ppu[0] - sub_constant_w_r[0], (ra.miny * upp[1] + constant[1]) * sub_ppu[1] - sub_constant_w_r[1], (ra.maxx * upp[0] + constant[0])* sub_ppu[0] - sub_constant_w_r[0], (ra.maxy * upp[1] + constant[1]) * sub_ppu[1] - sub_constant_w_r[1]);

					bool first_run = true;
					// SmartFILE f(filesystem::Path("twirl-log-pr.csv"), "w");
					for(int y = ra.miny; y < ra.maxy; ++y)
					{
						Color *cc = &c[y][ra.minx];
						auto pen = c.get_pen(ra.minx, y);
						for (int x = ra.minx; x < ra.maxx; ++x, ++cc, pen.inc_x()) {
							Real u = upp[0] * x + constant[0];
							Real v = upp[1] * y + constant[1];
							//							Real u = (source_rect.get_size()[0] * x + constant_[0]) / target_rect.get_size()[0];
							//							Real v = (source_rect.get_size()[1] * y + constant_[1]) / target_rect.get_size()[1];
							if (first_run) {
								first_run = false;
								fprintf(stderr, "-p- %f, %f\n", u, v);
								// synfig::error("\t\tsx: %f, %f", (u * sub_tasks[0]->target_rect.get_size()[0] - sub_constant_w_r_[0]) / sub_tasks[0]->source_rect.get_size()[0], (v * sub_tasks[0]->target_rect.get_size()[1] - sub_constant_w_r_[1]) / sub_tasks[0]->source_rect.get_size()[1]);
								// synfig::error("\tx: %i y: %i", x, y);
							}
							//Point q = /*internal.transform*/(Vector(u, v));
							Point q = point_vfunc(Vector(u, v));
							// synfig::warning("\tp: %f, %f\tq: %f, %f", u, v, q[0], q[1]);
							// from 'world' to raster conversion: r = (w - constant) * ppu
							int sx = q[0] * sub_ppu[0] - sub_constant_w_r[0];
							int sy = q[1] * sub_ppu[1] - sub_constant_w_r[1];
							//							int sx = (q[0] * sub_tasks[0]->target_rect.get_size()[0] - sub_constant_w_r_[0]) / sub_tasks[0]->source_rect.get_size()[0];
							//							int sy = (q[1] * sub_tasks[0]->target_rect.get_size()[1] - sub_constant_w_r_[1]) / sub_tasks[0]->source_rect.get_size()[1];
							if (sx != x || sy != y) {
								//								synfig::error("\t\tsx: %i sy: %i   |   %f, %f", sx, sy, (q[0] * sub_tasks[0]->target_rect.get_size()[0] - sub_constant_w_r_[0]) / sub_tasks[0]->source_rect.get_size()[0], (q[1] * sub_tasks[0]->target_rect.get_size()[1] - sub_constant_w_r_[1]) / sub_tasks[0]->source_rect.get_size()[1]);
								//							synfig::error("\tx: %i y: %i", x, y);
							}
							// synfig::error("sx: %i sy: %i", sx, sy);
							// synfig::error("\tsx: %i sy: %i", sx- r.minx + offset[0], sy- r.miny + offset[1]);
							const Color *ca = &a[sy][sx];//&a[sy - r.miny + offset[1]][sx - r.minx + offset[0]];
							*cc = *ca;
							//*cc = a.cubic_sample(sx,sy);///*Color::red();//**/*ca;//Color::red();//*ca;
							//							pen.put_value(a.cubic_sample(sx, sy));
							// fprintf(f.get(), "%i;%i;%i;%i;%f;%f;%f;%f\n", x, y, sx, sy, u, v, q[0], q[1]);
						}
					}
				}
			}
		}

		return true;
	}
};

rendering::Task::Token TaskCobraTest::token(
	DescAbstract<TaskCobraTest>("CobraTest") );
rendering::Task::Token TaskCobraTestSW::token(
	DescReal<TaskCobraTestSW, TaskCobraTest>("CobraTestSW") );

/* === M E T H O D S ======================================================= */

Layer_CobraTest::Layer_CobraTest()
{
	SET_INTERPOLATION_DEFAULTS();
	SET_STATIC_DEFAULTS();
}

bool
Layer_CobraTest::set_param(const String& param, const ValueBase& value)
{
	return Layer::set_param(param, value);
}

ValueBase
Layer_CobraTest::get_param(const String& param) const
{
	EXPORT_NAME();
	EXPORT_VERSION();

	return Layer::get_param(param);
}

Layer::Vocab
Layer_CobraTest::get_param_vocab() const
{
	Layer::Vocab ret(Layer::get_param_vocab());

	return ret;
}

rendering::Task::Handle
Layer_CobraTest::build_rendering_task_vfunc(Context context) const
{
	rendering::Task::Handle subtask = context.build_rendering_task();
	TaskCobraTest::Handle task(new TaskCobraTest());
	task->sub_task() = subtask;
	return task;
}
