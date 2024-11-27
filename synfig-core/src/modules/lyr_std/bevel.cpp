/* === S Y N F I G ========================================================= */
/*!	\file bevel.cpp
**	\brief Implementation of the "Bevel" layer
**
**	\legal
**	Copyright (c) 2002-2005 Robert B. Quattlebaum Jr., Adrian Bentley
**	Copyright (c) 2012-2013 Carlos López
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

#include "bevel.h"

#include <synfig/localization.h>
#include <synfig/general.h>

#include <synfig/blur.h>
#include <synfig/context.h>

#include "synfig/debug/debugsurface.h"

#endif

/* === U S I N G =========================================================== */

using namespace synfig;
using namespace modules;
using namespace lyr_std;

/* === G L O B A L S ======================================================= */

SYNFIG_LAYER_INIT(Layer_Bevel);
SYNFIG_LAYER_SET_NAME(Layer_Bevel,"bevel");
SYNFIG_LAYER_SET_LOCAL_NAME(Layer_Bevel,N_("Bevel"));
SYNFIG_LAYER_SET_CATEGORY(Layer_Bevel,N_("Stylize"));
SYNFIG_LAYER_SET_VERSION(Layer_Bevel,"0.2");

/* === P R O C E D U R E S ================================================= */

static void
save_float_surface(const surface<float>& float_surface, const filesystem::Path& filename, bool overwrite)
{
	synfig::Surface	color_surface(float_surface.get_w(), float_surface.get_h());
	for(int j=0;j<color_surface.get_h();j++)
		for(int i=0;i<color_surface.get_w();i++)
			color_surface[j][i] = Color(1,1,1,float_surface[j][i]);
	debug::DebugSurface::save_to_file(color_surface, filename, overwrite);
}

/* === M E T H O D S ======================================================= */

Layer_Bevel::Layer_Bevel():
	Layer_CompositeFork(0.75,Color::BLEND_ONTO),
	param_type(ValueBase(int(Blur::FASTGAUSSIAN))),
	param_softness (ValueBase(Real(0.1))),
	param_color1(ValueBase(Color::white())),
	param_color2(ValueBase(Color::black())),
	param_depth(ValueBase(Real(0.2)))
{
	param_angle=ValueBase(Angle::deg(135));
	calc_offset();
	param_use_luma=ValueBase(false);
	param_solid=ValueBase(false);
	param_cobra=ValueBase(true);
	param_overlay_only=ValueBase(false);

	SET_INTERPOLATION_DEFAULTS();
	SET_STATIC_DEFAULTS();
}

void
Layer_Bevel::calc_offset()
{
	Angle angle=param_angle.get(Angle());
	Real depth=param_depth.get(Real());
	
	offset[0]=Angle::cos(angle).get()*depth;
	offset[1]=Angle::sin(angle).get()*depth;

	offset45[0]=Angle::cos(angle-Angle::deg(45)).get()*depth*0.707106781;
	offset45[1]=Angle::sin(angle-Angle::deg(45)).get()*depth*0.707106781;
}

bool
Layer_Bevel::set_param(const String &param, const ValueBase &value)
{
	IMPORT_VALUE_PLUS(param_softness,
		{
			Real softness=param_softness.get(Real());
			softness=softness>0?softness:0;
			param_softness.set(softness);
		}
		);
	IMPORT_VALUE(param_color1);
	IMPORT_VALUE(param_color2);
	IMPORT_VALUE_PLUS(param_depth,calc_offset());
	IMPORT_VALUE_PLUS(param_angle,calc_offset());
	IMPORT_VALUE(param_type);
	IMPORT_VALUE(param_use_luma);
	IMPORT_VALUE(param_solid);
	IMPORT_VALUE(param_cobra);
	IMPORT_VALUE(param_overlay_only);
	if (param == "fake_origin")
		return true;

	return Layer_Composite::set_param(param,value);
}

ValueBase
Layer_Bevel::get_param(const String &param)const
{
	EXPORT_VALUE(param_type);
	EXPORT_VALUE(param_softness);
	EXPORT_VALUE(param_color1);
	EXPORT_VALUE(param_color2);
	EXPORT_VALUE(param_depth);
	EXPORT_VALUE(param_angle);
	EXPORT_VALUE(param_use_luma);
	EXPORT_VALUE(param_solid);
	EXPORT_VALUE(param_cobra);
	EXPORT_VALUE(param_overlay_only);
	if (param == "fake_origin")
	{
		return Vector();
	}

	EXPORT_NAME();
	EXPORT_VERSION();

	return Layer_Composite::get_param(param);
}

Color
Layer_Bevel::get_color(Context context, const Point &pos)const
{
	Real softness=param_softness.get(Real());
	int type=param_type.get(int());
	Color color1=param_color1.get(Color());
	Color color2=param_color2.get(Color());
	
	const Vector size(softness,softness);
	Point blurpos = Blur(size,type)(pos);

	if(get_amount()==0.0)
		return context.get_color(pos);

	Color shade;

	Real hi_alpha(1.0f-context.get_color(blurpos+offset).get_a());
	Real lo_alpha(1.0f-context.get_color(blurpos-offset).get_a());

	Real shade_alpha(hi_alpha-lo_alpha);
	if(shade_alpha>0)
		shade=color1,shade.set_a(shade_alpha);
	else
		shade=color2,shade.set_a(-shade_alpha);

	return Color::blend(shade,context.get_color(pos),get_amount(),get_blend_method());
}

RendDesc
Layer_Bevel::get_sub_renddesc_vfunc(const RendDesc &renddesc) const
{
	Real softness=param_softness.get(Real());
	int type=param_type.get(int());

	const int	w = renddesc.get_w(),
				h = renddesc.get_h();
	const Real	pw = renddesc.get_pw(),
				ph = renddesc.get_ph();
	const Vector size(softness,softness);

	RendDesc workdesc(renddesc);

	//expand the working surface to accommodate the blur

	//the expanded size = 1/2 the size in each direction rounded up
	int	halfsizex = (int) (std::fabs(size[0]*.5/pw) + 3),
		halfsizey = (int) (std::fabs(size[1]*.5/ph) + 3);

	int offset_u(round_to_int(offset[0]/pw)),offset_v(round_to_int(offset[1]/ph));
	int offset_w(w+std::abs(offset_u)*2),offset_h(h+std::abs(offset_v)*2);

	workdesc.set_subwindow(
		-std::abs(offset_u),
		-std::abs(offset_v),
		w+std::abs(offset_u),
		h+std::abs(offset_v)
	);

	//expand by 1/2 size in each direction on either side
	switch(type)
	{
		case Blur::DISC:
		case Blur::BOX:
		case Blur::CROSS:
		case Blur::FASTGAUSSIAN:
		{
			halfsizex = std::max(1, halfsizex);
			halfsizey = std::max(1, halfsizey);
			break;
		}
		case Blur::GAUSSIAN:
		{
		#define GAUSSIAN_ADJUSTMENT		(0.05)
			Real pw = workdesc.get_pw();
			Real ph = workdesc.get_ph();

			Real pw2 = pw * pw;
			Real ph2 = ph * ph;

			halfsizex = (int)(size[0]*GAUSSIAN_ADJUSTMENT/std::fabs(pw2) + 0.5);
			halfsizey = (int)(size[1]*GAUSSIAN_ADJUSTMENT/std::fabs(ph2) + 0.5);

			halfsizex = (halfsizex + 1)/2;
			halfsizey = (halfsizey + 1)/2;
			break;
		}
	}

	workdesc.set_subwindow( -halfsizex, -halfsizey, offset_w + 2*halfsizex, offset_h + 2*halfsizey );
	synfig::warning("Original RendDesc:\nx %f y %f\nw %i h %i", renddesc.get_tl()[0], renddesc.get_tl()[1], renddesc.get_w(), renddesc.get_h());
	synfig::warning("Changed RendDesc:\nx %f y %f\nw %i h %i", workdesc.get_tl()[0], workdesc.get_tl()[1], workdesc.get_w(), workdesc.get_h());
	return workdesc;
}


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

class TaskBevel: public rendering::Task
{
public:
	typedef etl::handle<TaskBevel> Handle;
	SYNFIG_EXPORT static Token token;
	Token::Handle get_token() const override { return token.handle(); }

	Real softness;
	int type;
	Color color1;
	Color color2;
	bool use_luma;
	bool solid;

	Vector offset, offset45;

	void set_coords_sub_tasks() override
	{
		synfig::error(__PRETTY_FUNCTION__);
		if (!sub_task(0)) {
			trunc_to_zero();
			return;
		}
		if (!is_valid_coords()) {
			sub_task(0)->set_coords_zero();
			return;
		}

		const int w = target_rect.get_width();
		const int h = target_rect.get_height();
		const Real pw = get_units_per_pixel()[0];
		const Real ph = get_units_per_pixel()[1];

		synfig::error("source rect:\nx %f y %f\nw %f %f", source_rect.minx, source_rect.miny, source_rect.get_width(), source_rect.get_height());
		synfig::error("target rect:\nx %i y %i\nw %i %i", target_rect.minx, target_rect.miny, target_rect.get_width(), target_rect.get_height());

		const Vector size(softness,softness);

		//expand the working surface to accommodate the blur

		//the expanded size = 1/2 the size in each direction rounded up
		int	halfsizex = (int) (std::fabs(size[0]*.5/pw) + 3),
			halfsizey = (int) (std::fabs(size[1]*.5/ph) + 3);

		const int offset_u(round_to_int(offset[0]/pw));
		const int offset_v(round_to_int(offset[1]/ph));
		const int offset_w(w+std::abs(offset_u)*2);
		const int offset_h(h+std::abs(offset_v)*2);

		//expand by 1/2 size in each direction on either side
		switch(type)
		{
			case Blur::DISC:
			case Blur::BOX:
			case Blur::CROSS:
			case Blur::FASTGAUSSIAN:
			{
				halfsizex = std::max(1, halfsizex);
				halfsizey = std::max(1, halfsizey);
				break;
			}
			case Blur::GAUSSIAN:
			{
			#define GAUSSIAN_ADJUSTMENT		(0.05)

				Real pw2 = pw * pw;
				Real ph2 = ph * ph;

				halfsizex = (int)(size[0]*GAUSSIAN_ADJUSTMENT/std::fabs(pw2) + 0.5);
				halfsizey = (int)(size[1]*GAUSSIAN_ADJUSTMENT/std::fabs(ph2) + 0.5);

				halfsizex = (halfsizex + 1)/2;
				halfsizey = (halfsizey + 1)/2;
				break;
			}
		}

		Real delta_x = -std::abs(offset_u) - halfsizex;
		Real delta_y = -std::abs(offset_v) - halfsizey;
		Real new_w = std::abs(offset_u) + offset_w + 2*halfsizex;
		Real new_h = std::abs(offset_v) + offset_h + 2*halfsizey;

		Rect new_sub_source_rect(source_rect.minx + pw*delta_x, source_rect.miny + ph*delta_y, source_rect.minx + pw*delta_x + pw*new_w, source_rect.miny + ph*delta_y + ph*new_h);
		sub_task(0)->set_coords(new_sub_source_rect, VectorInt(new_w, new_h));
		synfig::error("Delta:\ndx %f dy %f\nw %f %f", delta_x, delta_y, new_w, new_h);
		synfig::error("New subtask source rect:\nx %f y %f\nw %f %f", new_sub_source_rect.minx, new_sub_source_rect.miny, new_sub_source_rect.get_width(), new_sub_source_rect.get_height());

		synfig::warning("subtask source rect:\nx %f y %f\nw %f %f", sub_task(0)->source_rect.minx, sub_task(0)->source_rect.miny, sub_task(0)->source_rect.get_width(), sub_task(0)->source_rect.get_height());
		synfig::warning("subtask target rect:\nx %i y %i\nw %i %i", sub_task(0)->target_rect.minx, sub_task(0)->target_rect.miny, sub_task(0)->target_rect.get_width(), sub_task(0)->target_rect.get_height());
	}
};

SYNFIG_EXPORT rendering::Task::Token TaskBevel::token(
	DescAbstract<TaskBevel>("Bevel") );

#include "synfig/rendering/software/task/tasksw.h"


class TaskBevelSW : public TaskBevel, public synfig::rendering::TaskSW
{
public:
	typedef etl::handle<TaskBevel> Handle;
	SYNFIG_EXPORT static Token token;
	Token::Handle get_token() const override { return token.handle(); }

	// Rect calc_bounds() const override
	// {
	// 		synfig::warning("Calc bounds %s", sub_task(0) ? "y" : "n");
	// 	return sub_task(0) ? sub_task(0)->get_bounds() : Rect::zero(); // FIXME
	// }

	bool run(RunParams&) const override {
		if (!is_valid())
			return true;
		if (!sub_task(0))
			return false;


		Rect common_source_rect;
		rect_set_intersect(common_source_rect, source_rect, sub_task(0)->source_rect);
		if (!common_source_rect.is_valid())
			return false;

		CoordConverter conv(*this);
		auto convert_to_raster_subtask = conv.to_subtask_raster_coord(*sub_task(0));

		const PointInt target_min = conv.to_raster(common_source_rect.get_min());
		const PointInt target_max = conv.to_raster(common_source_rect.get_max());

		// LockWrite ldst(this);
		// if (!ldst) return false;
		// LockRead lsrc(sub_task(0));
		// if (!lsrc) return false;

		// const synfig::Surface& src = lsrc->get_surface();
		// synfig::Surface& dst = ldst->get_surface();

		// for(int y = target_min[1]; y < target_max[1]; ++y)
		// {
		// 	Color* cc = &dst[y][target_min[0]];
		// 	for (int x = target_min[0]; x < target_max[0]; ++x, ++cc) {
		// 		auto p = convert_to_raster_subtask(PointInt(x,y));
		// 		*cc = src[p[1]][p[0]];
		// 	}
		// }
		// return true;




		Vector ppu = get_pixels_per_unit();

synfig::error("Target: %i, %i  ->  %i, %i", target_rect.get_min()[0], target_rect.get_min()[1], target_rect.get_max()[0], target_rect.get_max()[1]);
synfig::error("Sub Target: %i, %i  ->  %i, %i", sub_tasks[0]->target_rect.get_min()[0], sub_tasks[0]->target_rect.get_min()[1], sub_tasks[0]->target_rect.get_max()[0], sub_tasks[0]->target_rect.get_max()[1]);

synfig::error("Source: %f, %f  ->  %f, %f", source_rect.get_min()[0], source_rect.get_min()[1], source_rect.get_max()[0], source_rect.get_max()[1]);
synfig::error("Sub source: %f, %f  ->  %f, %f", sub_tasks[0]->source_rect.get_min()[0], sub_tasks[0]->source_rect.get_min()[1], sub_tasks[0]->source_rect.get_max()[0], sub_tasks[0]->source_rect.get_max()[1]);

synfig::error("ppu: %f, %f", get_pixels_per_unit()[0], get_pixels_per_unit()[1]);
synfig::error("Sub ppu: %f, %f", sub_tasks[0]->get_pixels_per_unit()[0], sub_tasks[0]->get_pixels_per_unit()[1]);

		int tw = target_rect.get_width();
		LockWrite la(this);
		if (!la)
			return false;

		synfig::surface<float> blurred;

		const Vector size(softness,softness);

		{
			synfig::surface<float> alpha_surface;
			get_alpha_surface(alpha_surface);
			//blur the image
			Blur(size, type)(alpha_surface, sub_task(0)->source_rect.get_size(), blurred); //source_rect??
			synfig::error("Alpha size: %i, %i", alpha_surface.get_w(), alpha_surface.get_h());
			synfig::error("Blurred size: %i, %i", blurred.get_w(), blurred.get_h());
		}

		save_float_surface(blurred, filesystem::Path("blurred-cobra.tga"), true);

		const Real pw = 1/ppu[0];
		const Real ph = 1/ppu[1];
		const int halfsizex = (int) (std::fabs(size[0]*.5/pw) + 3);
		const int halfsizey = (int) (std::fabs(size[1]*.5/ph) + 3);

		const int offset_u(round_to_int(offset[0]/pw)),offset_v(round_to_int(offset[1]/ph));

		const float u0(offset[0]/pw),   v0(offset[1]/ph);
		const float u1(offset45[0]/pw), v1(offset45[1]/ph);

		synfig::Surface::pen apen(la->get_surface().get_pen(target_min[0], target_min[1]));

		int v = halfsizey+std::abs(offset_v) + target_min[1];
		for(int iy = target_min[1]; iy < target_max[1]; ++iy, apen.inc_y(), apen.dec_x(target_max[0]-target_min[0]), ++v) {
			int u = halfsizex+std::abs(offset_u) + target_min[0];
			for(int ix =target_min[0]; ix < target_max[0]; ++ix, apen.inc_x(), ++u) {

				Real alpha(0);
				Color shade;

				alpha += -blurred.linear_sample(u+u0, v+v0);
				alpha -= -blurred.linear_sample(u-u0, v-v0);
				alpha += -blurred.linear_sample(u+u1, v+v1)*0.5f;
				alpha += -blurred.linear_sample(u+v1, v-u1)*0.5f;
				alpha -= -blurred.linear_sample(u-u1, v-v1)*0.5f;
				alpha -= -blurred.linear_sample(u-v1, v+u1)*0.5f;

				if(solid)
				{
					alpha/=4.0f;
					alpha+=0.5f;
					shade=Color::blend(color1,color2,alpha,Color::BLEND_STRAIGHT);
				}
				else
				{
					alpha/=2;
					if(alpha>0)
						shade=color1,shade.set_a(shade.get_a()*alpha);
					else
						shade=color2,shade.set_a(shade.get_a()*-alpha);
				}

				if (shade.get_a())
					la->get_surface()[iy][ix] = shade;	//apen.put_value(shade);
				else
					la->get_surface()[iy][ix] = Color::alpha(); //apen.put_value(Color::alpha());
			}
		}
		debug::DebugSurface::save_to_file(*la.get_surface(), filesystem::Path("cobra.tga"), true);
		return true;
	}

private:
	bool get_alpha_surface(synfig::surface<float>& output) const
	{
		LockRead lb(sub_tasks[0]);
		if (!lb)
			return false;

		// Vector ppu = get_pixels_per_unit();
		// Matrix transformation_matrix;
		// transformation_matrix.m00 = ppu[0];
		// transformation_matrix.m11 = ppu[1];
		// transformation_matrix.m20 = target_rect.minx - source_rect.minx*ppu[0];
		// transformation_matrix.m21 = target_rect.miny - source_rect.miny*ppu[1];
		// Matrix inv_transformation_matrix = transformation_matrix.get_inverted();

		// Vector sub_ppu = sub_task(0)->get_pixels_per_unit();
		// Matrix sub_transformation_matrix;
		// sub_transformation_matrix.m00 = sub_ppu[0];
		// sub_transformation_matrix.m11 = sub_ppu[1];
		// sub_transformation_matrix.m20 = sub_task(0)->target_rect.minx - sub_task(0)->source_rect.minx*sub_ppu[0];
		// sub_transformation_matrix.m21 = sub_task(0)->target_rect.miny - sub_task(0)->source_rect.miny*sub_ppu[1];

		// sub_transformation_matrix *= inv_transformation_matrix;

		// Rect common_source_rect;
		// rect_set_intersect(common_source_rect, source_rect, sub_task(0)->source_rect);
		// if (!common_source_rect.is_valid())
		// 	return false;
		// const Rect rectf(transformation_matrix.get_transformed(common_source_rect.get_min()), transformation_matrix.get_transformed(common_source_rect.get_max()));
		// const RectInt rect(rectf.minx, rectf.miny, rectf.maxx, rectf.maxy);
		// const int rect_w = rect.get_width();

		// const Vector sub_rect_min = sub_transformation_matrix.get_transformed({rect.get_min()[0], rect.get_min()[1]});
		// synfig::Surface::const_alpha_pen bpen(lb->get_surface().get_pen(sub_rect_min[0], sub_rect_min[1]));
		const Surface& context = lb->get_surface();
		synfig::surface<float>& alpha_surface = output;
		alpha_surface.set_wh(context.get_w(), context.get_h());
		if (!use_luma) {
			for (int y = 0; y < context.get_h(); ++y) {
				for (int x = 0; x < context.get_w(); ++x) {
					alpha_surface[y][x] = context[y][x].get_a();
				}
			}
		} else {
			for (int y = 0; y < context.get_h(); ++y) {
				for (int x = 0; x < context.get_w(); ++x) {
					const auto& value = context[y][x];
					alpha_surface[y][x] = value.get_a() * value.get_y();
				}
			}
		}
		// alpha_surface.set_wh(rect.get_width(), rect.get_height());
		// if (!use_luma) {
		// 	for (int iy = 0, sy = sub_rect_min[1]; iy < rect.get_height(); ++iy, ++sy, bpen.inc_y(), bpen.dec_x(rect_w)) {
		// 		for (int ix = 0, sx = sub_rect_min[0]; ix < rect.get_width(); ++ix, ++sx, bpen.inc_x()) {
		// 			alpha_surface[iy][ix] = context[sy][sx].get_a();
		// 		}
		// 	}
		// } else {
		// 	for (int iy = 0, sy = sub_rect_min[1]; iy < rect.get_height(); ++iy, ++sy, bpen.inc_y(), bpen.dec_x(rect_w)) {
		// 		for (int ix = 0, sx = sub_rect_min[0]; ix < rect.get_width(); ++ix, ++sx, bpen.inc_x()) {
		// 			const auto& value = context[sy][sx];
		// 			alpha_surface[iy][ix] = value.get_a() * value.get_y();
		// 		}
		// 	}
		// }
		save_float_surface(alpha_surface, filesystem::Path("alpha-cobra.tga"), true);
		return true;
	}
};

SYNFIG_EXPORT rendering::Task::Token TaskBevelSW::token(
	DescReal<TaskBevelSW, TaskBevel>("BevelSW") );

bool
Layer_Bevel::accelerated_render(Context context,Surface *surface,int quality, const RendDesc &renddesc, ProgressCallback *cb)const
{
	RENDER_TRANSFORMED_IF_NEED(__FILE__, __LINE__)

	Real softness=param_softness.get(Real());
	const int type=param_type.get(int());
	Color color1=param_color1.get(Color());
	Color color2=param_color2.get(Color());
	bool use_luma=param_use_luma.get(bool());
	bool solid=param_solid.get(bool());
	bool overlay_only = param_overlay_only.get(bool());

	int x,y;
	SuperCallback stageone(cb,0,5000,10000);
	SuperCallback stagetwo(cb,5000,10000,10000);

	RendDesc	workdesc = get_sub_renddesc(renddesc);
	Surface		worksurface;
	synfig::surface<float> alpha_surface, blurred;

	const Real	pw = renddesc.get_pw(),
				ph = renddesc.get_ph();
	const Vector size(softness,softness);

	int	halfsizex = (int) (std::fabs(size[0]*.5/pw) + 3),
		halfsizey = (int) (std::fabs(size[1]*.5/ph) + 3);

	int offset_u(round_to_int(offset[0]/pw)),offset_v(round_to_int(offset[1]/ph));

	//callbacks depend on how long the blur takes
	if(size[0] || size[1])
	{
		if(type == Blur::DISC)
		{
			stageone = SuperCallback(cb,0,5000,10000);
			stagetwo = SuperCallback(cb,5000,10000,10000);
		}
		else
		{
			stageone = SuperCallback(cb,0,9000,10000);
			stagetwo = SuperCallback(cb,9000,10000,10000);
		}
	}
	else
	{
		stageone = SuperCallback(cb,0,9999,10000);
		stagetwo = SuperCallback(cb,9999,10000,10000);
	}

	switch(type)
	{
		case Blur::GAUSSIAN:
		{
			Real pw = workdesc.get_pw();
			Real ph = workdesc.get_ph();

			Real pw2 = pw * pw;
			Real ph2 = ph * ph;

			halfsizex = (int)(size[0]*GAUSSIAN_ADJUSTMENT/std::fabs(pw2) + 0.5);
			halfsizey = (int)(size[1]*GAUSSIAN_ADJUSTMENT/std::fabs(ph2) + 0.5);

			halfsizex = (halfsizex + 1)/2;
			halfsizey = (halfsizey + 1)/2;

			break;
		}
	}

	//render the background onto the expanded surface
	if(!context.accelerated_render(&worksurface,quality,workdesc,&stageone))
		return false;

	// Copy over the alpha
	alpha_surface.set_wh(worksurface.get_w(), worksurface.get_h());
	if(!use_luma)
	{
		for(int j=0;j<worksurface.get_h();j++)
			for(int i=0;i<worksurface.get_w();i++)
			{
				alpha_surface[j][i] = worksurface[j][i].get_a();
			}
	}
	else
	{
		for(int j=0;j<worksurface.get_h();j++)
			for(int i=0;i<worksurface.get_w();i++)
			{
				alpha_surface[j][i] = worksurface[j][i].get_a() * worksurface[j][i].get_y();
			}
	}
	save_float_surface(alpha_surface, filesystem::Path("alpha-accel.tga"), true);

	//blur the image
	Blur(size, type, &stagetwo)(alpha_surface, workdesc.get_br()-workdesc.get_tl(), blurred);

	save_float_surface(blurred, filesystem::Path("blurred-accel.tga"), true);
	//be sure the surface is of the correct size
	surface->set_wh(renddesc.get_w(),renddesc.get_h());

	const float u0(offset[0]/pw),   v0(offset[1]/ph);
	const float u1(offset45[0]/pw), v1(offset45[1]/ph);

	const float amount = get_amount();
	const Color::BlendMethod blend_method = get_blend_method();

	int v = halfsizey+std::abs(offset_v);
	for(y=0;y<renddesc.get_h();y++,v++)
	{
		int u = halfsizex+std::abs(offset_u);
		for(x=0;x<renddesc.get_w();x++,u++)
		{
			Real alpha(0);
			Color shade;

			alpha += -blurred.linear_sample(u+u0, v+v0);
			alpha -= -blurred.linear_sample(u-u0, v-v0);
			alpha += -blurred.linear_sample(u+u1, v+v1)*0.5f;
			alpha += -blurred.linear_sample(u+v1, v-u1)*0.5f;
			alpha -= -blurred.linear_sample(u-u1, v-v1)*0.5f;
			alpha -= -blurred.linear_sample(u-v1, v+u1)*0.5f;

			if(solid)
			{
				alpha/=4.0f;
				alpha+=0.5f;
				shade=Color::blend(color1,color2,alpha,Color::BLEND_STRAIGHT);
			}
			else
			{
				alpha/=2;
				if(alpha>0)
					shade=color1,shade.set_a(shade.get_a()*alpha);
				else
					shade=color2,shade.set_a(shade.get_a()*-alpha);
			}



			if(shade.get_a())
			{
				(*surface)[y][x] = overlay_only ? shade : Color::blend(shade, worksurface[v][u], amount, blend_method);
			}
			else (*surface)[y][x] = overlay_only ? Color::alpha() : worksurface[v][u];
		}
	}
	debug::DebugSurface::save_to_file(*surface, filesystem::Path("accel.tga"), true);

	if(cb && !cb->amount_complete(10000,10000))
	{
		//if(cb)cb->error(strprintf(__FILE__"%d: Accelerated Renderer Failure",__LINE__));
		return false;
	}

	return true;
}


////

Layer::Vocab
Layer_Bevel::get_param_vocab(void)const
{
	Layer::Vocab ret(Layer_Composite::get_param_vocab());

	ret.push_back(ParamDesc("type")
		.set_local_name(_("Type"))
		.set_description(_("Type of blur to use"))
		.set_hint("enum")
		.set_static(true)
		.add_enum_value(Blur::BOX,"box",_("Box Blur"))
		.add_enum_value(Blur::FASTGAUSSIAN,"fastgaussian",_("Fast Gaussian Blur"))
		.add_enum_value(Blur::CROSS,"cross",_("Cross-Hatch Blur"))
		.add_enum_value(Blur::GAUSSIAN,"gaussian",_("Gaussian Blur"))
		.add_enum_value(Blur::DISC,"disc",_("Disc Blur"))
	);

	ret.push_back(ParamDesc("color1")
		.set_local_name(_("Hi-Color"))
	);
	ret.push_back(ParamDesc("color2")
		.set_local_name(_("Lo-Color"))
	);
	ret.push_back(ParamDesc("angle")
		.set_local_name(_("Light Angle"))
		.set_origin("fake_origin")
	);
	ret.push_back(ParamDesc("depth")
		.set_is_distance()
		.set_local_name(_("Depth of Bevel"))
		.set_origin("fake_origin")
	);
	ret.push_back(ParamDesc("softness")
		.set_is_distance()
		.set_local_name(_("Softness"))
		.set_origin("fake_origin")
	);
	ret.push_back(ParamDesc("use_luma")
		.set_local_name(_("Use Luma"))
	);
	ret.push_back(ParamDesc("solid")
		.set_local_name(_("Solid"))
	);

	ret.push_back(ParamDesc("cobra")
		.set_local_name(_("Cobra Engine"))
	);

	ret.push_back(ParamDesc("overlay_only")
		.set_local_name(_("Overlay only"))
		.set_description(_("Do not render this layer context"))
	);

	ret.push_back(ParamDesc("fake_origin")
		.hidden()
	);

	return ret;
}

Rect
Layer_Bevel::get_full_bounding_rect(Context context)const
{
	Real softness=param_softness.get(Real());
	Real depth=param_depth.get(Real());

	if(is_disabled())
		return context.get_full_bounding_rect();

	Rect under(context.get_full_bounding_rect());

	if(Color::is_onto(get_blend_method()))
		return under;

	Rect bounds(under.expand(softness));
	bounds.expand_x(std::fabs(depth));
	bounds.expand_y(std::fabs(depth));

	return bounds;
}

rendering::Task::Handle
Layer_Bevel::build_composite_fork_task_vfunc(ContextParams /*context_params*/, rendering::Task::Handle sub_task) const
{
	TaskBevel::Handle task_bevel(new TaskBevel());
	task_bevel->softness = param_softness.get(Real());
	task_bevel->type = param_type.get(int());
	task_bevel->color1 = param_color1.get(Color());
	task_bevel->color2 = param_color2.get(Color());
	task_bevel->use_luma = param_use_luma.get(bool());
	task_bevel->solid = param_solid.get(bool());

	task_bevel->offset = offset;
	task_bevel->offset45 = offset45;

	task_bevel->sub_task(0) = sub_task;

	return task_bevel;
}

rendering::Task::Handle
Layer_Bevel::build_rendering_task_vfunc(Context context) const
{
	if (!param_cobra.get(bool()))
		return Layer::build_rendering_task_vfunc(context);

	if (!param_overlay_only.get(bool()))
		return Layer_CompositeFork::build_rendering_task_vfunc(context);

	return build_composite_fork_task_vfunc(context.get_params(), context.build_rendering_task());
}
