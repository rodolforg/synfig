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
#include <synfig/rendering/common/task/tasktransformation.h>
#include <synfig/target.h>


#include <synfig/general.h>
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


struct CoordConverter {
	CoordConverter(const rendering::Task& task)
		: ppu(task.get_pixels_per_unit()),
		upp(task.get_units_per_pixel())
	{
		const Rect& source = task.source_rect;
		const RectInt& target = task.target_rect;
		k[0] = source.get_min()[0] * target.get_max()[0] - source.get_max()[0] * target.get_min()[0];
		k[1] = source.get_min()[1] * target.get_max()[1] - source.get_max()[1] * target.get_min()[1];

		w_size = source.get_size();
		r_size = target.get_size();

		k_over_w_size = k.divide_coords(source.get_size());
		k_over_r_size[0] = k[0] / target.get_size()[0];
		k_over_r_size[1] = k[1] / target.get_size()[1];
	}

	PointInt to_raster(Point p)
	{
		Point q = p.multiply_coords(ppu) - k_over_w_size;
		return {round(q[0]), round(q[1])};
	}

	Point to_world(PointInt p)
	{
		return Point(p[0], p[1]).multiply_coords(upp) + k_over_w_size;
	}

	std::function<PointInt (PointInt)> from_subtask_raster_coord(const rendering::Task& subtask) const {
		CoordConverter sub_converter(subtask);
		const Point kk = sub_converter.k_over_r_size.multiply_coords(ppu) - k_over_w_size;
		auto conv = [kk](PointInt sr) -> PointInt {
			PointInt dr;
			dr[0] = sr[0] + kk[0];
			dr[1] = sr[1] + kk[1];
			return dr;
		};
		return conv;
	}

	std::function<PointInt (PointInt)> to_subtask_raster_coord(const rendering::Task& subtask) const {
		CoordConverter sub_converter(subtask);
		const Point kk = sub_converter.k_over_r_size.multiply_coords(ppu) - k_over_w_size;
		auto conv = [kk](PointInt dr) -> PointInt {
			PointInt sr;
			sr[0] = dr[0] - kk[0];
			sr[1] = dr[1] - kk[1];
			return sr;
		};
		return conv;
	}


private:
	Vector ppu, upp;
	Vector k;
	Vector k_over_w_size;
	Vector k_over_r_size;
	Vector w_size;
	VectorInt r_size;
};


class TaskCobraTest
	: public rendering::Task//, public rendering::TaskInterfaceTransformationPass
{
public:
	typedef etl::handle<TaskCobraTest> Handle;
	static Token token;
	Token::Handle get_token() const override { return token.handle(); }

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
		if (!sub_task())
			return true;

		Rect common_source_rect;
		rect_set_intersect(common_source_rect, source_rect, sub_task()->source_rect);
		if (!common_source_rect.is_valid())
			return false;

		CoordConverter conv(*this);
		auto convert_to_raster_subtask = conv.to_subtask_raster_coord(*sub_task());

		const PointInt target_min = conv.to_raster(common_source_rect.get_min());
		const PointInt target_max = conv.to_raster(common_source_rect.get_max());

		synfig::warning("\n"
						"Task World [%f, %f], [%f, %f]\n"
						"SubTask World [%f, %f], [%f, %f]\n"
						"Common World [%f, %f], [%f, %f]\n",
						 source_rect.get_min()[0], source_rect.get_min()[1], source_rect.get_max()[0], source_rect.get_max()[1],
						sub_task()->source_rect.get_min()[0], sub_task()->source_rect.get_min()[1], sub_task()->source_rect.get_max()[0], sub_task()->source_rect.get_max()[1],
						common_source_rect.get_min()[0], common_source_rect.get_min()[1], common_source_rect.get_max()[0], common_source_rect.get_max()[1]);

		synfig::warning("\n"
						"Task Raster [%i, %i], [%i, %i]\n"
						"SubTask Raster [%i, %i], [%i, %i]\n"
						"Common Raster [%i, %i], [%i, %i]\n",
						target_rect.get_min()[0], target_rect.get_min()[1], target_rect.get_max()[0], target_rect.get_max()[1],
						sub_task()->target_rect.get_min()[0], sub_task()->target_rect.get_min()[1], sub_task()->target_rect.get_max()[0], sub_task()->target_rect.get_max()[1],
						target_min[0], target_min[1], target_max[0], target_max[1]);

		LockWrite ldst(this);
		if (!ldst) return false;
		LockRead lsrc(sub_task());
		if (!lsrc) return false;

		const synfig::Surface& src = lsrc->get_surface();
		synfig::Surface& dst = ldst->get_surface();

		for(int y = target_min[1]; y < target_max[1]; ++y)
		{
			Color* cc = &dst[y][target_min[0]];
			for (int x = target_min[0]; x < target_max[0]; ++x, ++cc) {
				auto p = convert_to_raster_subtask(PointInt(x,y));
				*cc = src[p[1]][p[0]];
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
