/* === S Y N F I G ========================================================= */
/*!	\file widget_timeslider.cpp
**	\brief Time Slider Widget Implementation File
**
**	\legal
**	Copyright (c) 2004 Adrian Bentley
**	Copyright (c) 2007, 2008 Chris Moore
**	Copyright (c) 2012, Carlos López
**	......... ... 2018 Ivan Mahonin
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

#include <gui/widgets/widget_timeslider.h>

#include <cmath>

#include <gdkmm/general.h>
#include <gtkmm/icontheme.h>

#include <gui/app.h>
#include <gui/exception_guard.h>
#include <gui/timeplotdata.h>

#endif

/* === U S I N G =========================================================== */

using namespace synfig;
using namespace studio;

/* === M A C R O S ========================================================= */

/* === G L O B A L S ======================================================= */

const double zoominfactor = 1.25;
const double zoomoutfactor = 1/zoominfactor;
const int fullheight = 20;
const double handle_dimension = 8;

/* === P R O C E D U R E S ================================================= */

/* === M E T H O D S ======================================================= */

static void
calc_divisions(float fps, double range, double sub_range, double &out_step, int &out_subdivisions)
{
	int ifps = synfig::round_to_int(fps);
	if (ifps < 1) ifps = 1;

	// build a list of all the factors of the frame rate
	int pos = 0;
	std::vector<double> ranges;
	for(int i = 1; i*i <= ifps; i++)
		if (ifps % i == 0) {
			ranges.insert(ranges.begin() + pos, i/fps);
			if (i*i != ifps)
				ranges.insert(ranges.begin() + pos + 1, ifps/i/fps);
			pos++;
		}

	{ // fill in any gaps where one factor is more than 2 times the previous
		std::vector<double>::iterator iter, next;
		pos = 0;
		for(int pos = 0; pos < (int)ranges.size()-1; pos++) {
			next = ranges.begin() + pos;
			iter = next++;
			if (*iter*2 < *next)
				ranges.insert(next, *iter*2);
		}
	}

	double more_ranges[] = {
		2, 3, 5, 10, 20, 30, 60, 90, 120, 180,
		300, 600, 1200, 1800, 2700, 3600, 3600*2,
		3600*4, 3600*8, 3600*16, 3600*32, 3600*64,
		3600*128, 3600*256, 3600*512, 3600*1024 };
	ranges.insert(ranges.end(), more_ranges, more_ranges + sizeof(more_ranges)/sizeof(double));

	double mid_range = (range + sub_range)/2;

	// find most ideal scale
	double scale;
	{
		std::vector<double>::iterator next = synfig::binary_find(ranges.begin(), ranges.end(), mid_range);
		std::vector<double>::iterator iter = next++;
		if (iter == ranges.end())
			--iter;
		if (next == ranges.end())
			--next;
		if (fabs(*next - mid_range) < fabs(*iter - mid_range))
			iter = next;
		scale = *iter;
	}

	// subdivide into this many tick marks (8 or less)
	int subdiv = synfig::round_to_int(scale * ifps);
	if (subdiv > 8) {
		const int ideal = subdiv;

		// find a number of tick marks that nicely divides the scale
		// (5 minutes divided by 6 is 50s, but that's not 'nice' -
		//  5 ticks of 1m each is much simpler than 6 ticks of 50s)
		for (subdiv = 8; subdiv > 0; subdiv--)
			if ((ideal <= ifps*2       && (ideal % (subdiv           )) == 0) ||
				(ideal <= ifps*2*60    && (ideal % (subdiv*ifps      )) == 0) ||
				(ideal <= ifps*2*60*60 && (ideal % (subdiv*ifps*60   )) == 0) ||
				(true                  && (ideal % (subdiv*ifps*60*60)) == 0))
				break;

		// if we didn't find anything, use 4 ticks
		if (!subdiv)
			subdiv = 4;
	}

	out_step = scale;
	out_subdivisions = subdiv;
}

/* === E N T R Y P O I N T ================================================= */

Widget_Timeslider::Widget_Timeslider():
	layout(Pango::Layout::create(get_pango_context())),
	lastx()
{
	set_size_request(-1, fullheight);

	{ // prepare pattern for play bounds
		const int pattern_step = 32;
		Cairo::RefPtr<Cairo::ImageSurface> surface = Cairo::ImageSurface::create(Cairo::FORMAT_ARGB32, pattern_step, pattern_step);
		Cairo::RefPtr<Cairo::Context> cr = Cairo::Context::create(surface);
		cr->set_source_rgba(0.0, 0.0, 0.0, 0.25);
		cr->scale((double)pattern_step, (double)pattern_step);
		cr->set_line_width(0.375);
		cr->move_to(1.75, -1.0);
		cr->line_to(-1.0, 1.75);
		cr->stroke();
		cr->move_to(2.0, -0.25);
		cr->line_to(-0.25, 2.0);
		cr->stroke();
		surface->flush();

		play_bounds_pattern = Cairo::SurfacePattern::create(surface);
		play_bounds_pattern->set_filter(Cairo::FILTER_NEAREST);
		play_bounds_pattern->set_extend(Cairo::EXTEND_REPEAT);
	}

	time_plot_data = new TimePlotData(this);

	// click / scroll / zoom
	add_events( Gdk::BUTTON_PRESS_MASK
			  | Gdk::BUTTON_RELEASE_MASK
			  | Gdk::BUTTON_MOTION_MASK
			  | Gdk::SCROLL_MASK );

	auto icon_theme = Gtk::IconTheme::get_default();
	lower_bound_pixbuf = icon_theme->load_icon("lower_bound_handle_icon", 1);
	upper_bound_pixbuf = icon_theme->load_icon("upper_bound_handle_icon", 1);

	bounds_cursor = Gdk::Cursor::create(get_display(), "ew-resize");
	default_cursor = Gdk::Cursor::create(get_display(), "default");
}

Widget_Timeslider::~Widget_Timeslider()
{
	time_change.disconnect();
	time_bounds_change.disconnect();
	delete time_plot_data;
}

const etl::handle<TimeModel>&
Widget_Timeslider::get_time_model() const
{
	return time_plot_data->time_model;
}

void
Widget_Timeslider::set_time_model(const etl::handle<TimeModel> &x)
{
	time_plot_data->set_time_model(x);
}

void
Widget_Timeslider::draw_background(const Cairo::RefPtr<Cairo::Context> &cr)
{
	//draw grey rectangle
	cr->save();
	cr->set_source_rgb(0.5, 0.5, 0.5);
	cr->rectangle(0.0, 0.0, (double)get_width(), (double)get_height());
	cr->fill();
	cr->restore();
}

bool
Widget_Timeslider::on_draw(const Cairo::RefPtr<Cairo::Context> &cr)
{
	const double mark_height = 12.0;
	const double sub_mark_height = 4.0;

	draw_background(cr);

	if (!time_plot_data->time_model || get_width() <= 0 || get_height() <= 0) return true;

	const etl::handle<TimeModel> & time_model = time_plot_data->time_model;

	// Draw the time line...
	double tpx = time_plot_data->get_pixel_t_coord(time_plot_data->time) + 0.5;
	cr->save();
	cr->set_source_rgb(1.0, 175.0/255.0, 0.0);
	cr->set_line_width(1.0);
	cr->move_to(tpx, 0.0);
	cr->line_to(tpx, fullheight);
	cr->stroke();
	cr->restore();

	// draw marks

	// get divisions
	double big_step_value;
	int subdivisions;
	calc_divisions(
		time_model->get_frame_rate(),
		140.0/time_plot_data->k,
		280.0/time_plot_data->k,
		big_step_value,
		subdivisions );

	step = time_model->round_time(Time(big_step_value/(double)subdivisions));
	step = std::max(time_model->get_step_increment(), step);

	Time big_step = step * (double)subdivisions;
	Time current = big_step * floor((double)time_plot_data->lower_ex/(double)big_step);
	current = time_model->round_time(current);

	// draw
	cr->save();
	cr->set_source_rgb(51.0/255.0,51.0/255.0,51.0/255.0);
	cr->set_line_width(1.0);
	for(int i = 0; current <= time_plot_data->upper_ex; ++i, current = time_model->round_time(current + step)) {
		double x = time_plot_data->get_pixel_t_coord(current) + 0.5;
		if (i % subdivisions == 0) {
			// draw big
			cr->move_to(x, 0.0);
			cr->line_to(x, mark_height);
			cr->stroke();

			layout->set_text(
				current.get_string(
					time_model->get_frame_rate(),
					App::get_time_format() ));

			// Approximately a font size of 8 pixels.
			// Pango::SCALE = 1024
			// create_attr_size waits a number in 1000th of pixels.
			// Should be user customizable in the future. Now it is fixed to 10
			Pango::AttrList attr_list;
			Pango::AttrInt pango_size(Pango::Attribute::create_attr_size(Pango::SCALE*10));
			pango_size.set_start_index(0);
			pango_size.set_end_index(64);
			attr_list.change(pango_size);
			layout->set_attributes(attr_list);
			cr->move_to(x + 1.0, 0.0);
			layout->show_in_cairo_context(cr);
		} else {
			// draw small
			cr->move_to(x, 0.0);
			cr->line_to(x, sub_mark_height);
			cr->stroke();
		}
	}
	cr->restore();

	// Draw the time line
	Gdk::Cairo::set_source_rgba(cr, Gdk::RGBA("#ffaf00"));
	cr->set_line_width(3.0);
	double x = time_plot_data->get_pixel_t_coord(time_plot_data->time);
	cr->move_to(x, 0.0);
	cr->line_to(x, fullheight);
	cr->stroke();

	// Draw play bounds
	if (time_model->get_play_bounds_enabled()) {
		double offset = time_plot_data->get_double_pixel_t_coord(0);
		Time bounds[2][2] {
			{ time_plot_data->lower_ex, time_model->get_play_bounds_lower() },
			{ time_model->get_play_bounds_upper(), time_plot_data->upper_ex } };
		for(int i = 0; i < 2; ++i) {
			if (bounds[i][0] < bounds[i][1]) {
				double x0 = time_plot_data->get_double_pixel_t_coord(bounds[i][0]);
				double x1 = time_plot_data->get_double_pixel_t_coord(bounds[i][1]);
				double w = x1 - x0;

				double boundary_adjust = handle_dimension;
				double background_adjust = (i == 0) ? 0 : handle_dimension;

				cr->save();
				cr->rectangle(x0 + background_adjust, 0.0, w - handle_dimension, (double)get_height());
				cr->clip();
				cr->translate(offset, 0.0);
				cr->set_source(play_bounds_pattern);
				cr->paint();
				cr->restore();

				boundary_adjust = (i == 0) ? 0 : handle_dimension;

				cr->save();
				cr->set_line_width(1.0);
				cr->set_source_rgba(0.0, 0.0, 0.0, 0.25);//inner border
				cr->rectangle(x0 + 1.0 + boundary_adjust, 1.0, w - 2.0 - handle_dimension, (double)get_height() - 2.0);
				cr->stroke();
				cr->set_source_rgba(0.0, 0.0, 0.0, 0.3);//outer border
				cr->rectangle(x0 + 0.5 + boundary_adjust, 0.5, w - 1.0 - handle_dimension, (double)get_height() - 1.0);
				cr->stroke();
				cr->restore();

				Glib::RefPtr<Gdk::Pixbuf> icon;
				if (i == 0) {
					boundary_adjust = -handle_dimension;
					icon = lower_bound_pixbuf;
				} else {
					boundary_adjust = -w;
					icon = upper_bound_pixbuf;
				}

				cr->save();
				Gdk::Cairo::set_source_pixbuf(cr, icon, x0 + w + boundary_adjust, 0);
				cr->rectangle(x0 + w + boundary_adjust, 0.0, handle_dimension, (double)get_height());
				cr->fill();
				if ((moving_lower_bound_handle && i==0) || (moving_upper_bound_handle && i==1)) { //highlight/lighten bound on move
					cairo_set_operator(cr->cobj(), CAIRO_OPERATOR_LIGHTEN);
					cr->set_source_rgba(1.0, 1.0, 1.0, 0.4);
					cr->rectangle(x0 + w + boundary_adjust, 0.0, handle_dimension, (double)get_height());
					cr->fill();
				} else if ((hovering_on_lower_bound_handle && i==0) ||	(hovering_on_upper_bound_handle && i==1)) { //highlight/lighten bound on hover
					cairo_set_operator(cr->cobj(), CAIRO_OPERATOR_LIGHTEN);
					cr->set_source_rgba(1.0, 1.0, 1.0, 0.2);
					cr->rectangle(x0 + w + boundary_adjust, 0.0, handle_dimension, (double)get_height());
					cr->fill();
				}
				cr->restore();
			}
		}
	}

	return true;
}

bool
Widget_Timeslider::on_button_press_event(GdkEventButton *event) //for clicking
{
	SYNFIG_EXCEPTION_GUARD_BEGIN()
	lastx = (double)event->x;

	if (!time_plot_data->time_model || get_width() <= 0 || get_height() <= 0)
		return false;

	if (event->button == 1) {
		Time time = time_plot_data->get_t_from_pixel_coord(event->x);
		time_plot_data->time_model->set_time(time);
	}
	//if moving timeslider along with handle isnt wanted move these above prev if and add cond.
	bool bounds_enabled = time_plot_data->time_model->get_play_bounds_enabled();
	if (bounds_enabled) {
		synfig::Rect lower_bound = get_bounds_rectangle(true);
		synfig::Rect upper_bound = get_bounds_rectangle(false);
		synfig::Point cursor_pos(event->x, 0.0);

		if (lower_bound.is_inside(cursor_pos))
			moving_lower_bound_handle = true;
		else if (upper_bound.is_inside(cursor_pos))
			moving_upper_bound_handle = true;
	}
	queue_draw();
	return event->button == 1 || event->button == 2;
	SYNFIG_EXCEPTION_GUARD_END_BOOL(true)
}

bool
Widget_Timeslider::on_button_release_event(GdkEventButton *event){
	SYNFIG_EXCEPTION_GUARD_BEGIN()
	lastx = (double)event->x;
	moving_lower_bound_handle = false;
	moving_upper_bound_handle = false;
	queue_draw();
	return event->button == 1 || event->button == 2;
	SYNFIG_EXCEPTION_GUARD_END_BOOL(true)
}

bool
Widget_Timeslider::on_leave_notify_event(GdkEventCrossing*)
{
	SYNFIG_EXCEPTION_GUARD_BEGIN()
	hovering_on_lower_bound_handle = false;
	hovering_on_upper_bound_handle = false;
	queue_draw();
	return true;
	SYNFIG_EXCEPTION_GUARD_END_BOOL(true)
}

const synfig::Rect
Widget_Timeslider::get_bounds_rectangle(bool lower) const
{
	if (lower) {
		double x1 = time_plot_data->get_double_pixel_t_coord(time_plot_data->time_model->get_play_bounds_lower());
		synfig::Rect lower_bound(synfig::Point(x1 - handle_dimension, 0.0), synfig::Point(x1, 0.0));
		return lower_bound;
	} else {
		double x0 = time_plot_data->get_double_pixel_t_coord(time_plot_data->time_model->get_play_bounds_upper());
		synfig::Rect upper_bound(synfig::Point(x0, 0.0), synfig::Point(x0 + handle_dimension, 0.0));
		return upper_bound;
	}
}

bool
Widget_Timeslider::on_motion_notify_event(GdkEventMotion* event) //for dragging
{
	SYNFIG_EXCEPTION_GUARD_BEGIN()

	synfig::Rect lower_bound = get_bounds_rectangle(true);
	synfig::Rect upper_bound = get_bounds_rectangle(false);
	synfig::Point cursor_pos(event->x, 0.0);
	bool moving_handle = moving_lower_bound_handle || moving_upper_bound_handle;
	bool bounds_enabled = time_plot_data->time_model->get_play_bounds_enabled();
	hovering_on_lower_bound_handle = lower_bound.is_inside(cursor_pos);
	hovering_on_upper_bound_handle = upper_bound.is_inside(cursor_pos);
	bool hovering_on_handle = hovering_on_lower_bound_handle || hovering_on_upper_bound_handle;

	if ((moving_handle || hovering_on_handle) && (bounds_enabled))
		get_window()->set_cursor(bounds_cursor);
	else
		get_window()->set_cursor(default_cursor);

	queue_draw();

	double dx = (double)event->x - lastx;
	lastx = (double)event->x;

	if (!time_plot_data->time_model || get_width() <= 0 || get_height() <= 0)
		return false;

	Gdk::ModifierType mod = Gdk::ModifierType(event->state);

	if (moving_lower_bound_handle)
		time_plot_data->time_model->set_play_bounds_lower(time_plot_data->get_t_from_pixel_coord(event->x));
	else if (moving_upper_bound_handle)
		time_plot_data->time_model->set_play_bounds_upper(time_plot_data->get_t_from_pixel_coord(event->x));

	if ((mod & Gdk::BUTTON1_MASK) /*&& (!moving_lower_bound_handle) && (!moving_upper_bound_handle)*/) {//commented code is for disabling moving timeslider while moving bound
		// scrubbing
		Time time = time_plot_data->get_t_from_pixel_coord(event->x);
		time_plot_data->time_model->set_time(time);
		return true;
	} else
	if (mod & Gdk::BUTTON2_MASK) {
		// scrolling
		Time dt(-dx*time_plot_data->dt);
		time_plot_data->time_model->move_by(dt);
		return true;
	}

	return false;
	SYNFIG_EXCEPTION_GUARD_END_BOOL(true)
}

bool
Widget_Timeslider::on_scroll_event(GdkEventScroll* event) //for zooming
{
	SYNFIG_EXCEPTION_GUARD_BEGIN()
	etl::handle<TimeModel> &time_model = time_plot_data->time_model;

	if (!time_model || get_width() <= 0 || get_height() <= 0)
		return false;

	Time time = time_plot_data->get_t_from_pixel_coord(event->x);

	switch(event->direction) {
	case GDK_SCROLL_UP: //zoom in
		time_model->zoom(zoominfactor, time);
		return true;
	case GDK_SCROLL_DOWN: //zoom out
		time_model->zoom(zoomoutfactor, time);
		return true;
	case GDK_SCROLL_RIGHT:
		time_model->move_by(step);
		return true;
	case GDK_SCROLL_LEFT:
		time_model->move_by(-step);
		return true;
	default:
		break;
	}

	return false;
	SYNFIG_EXCEPTION_GUARD_END_BOOL(true)
}
