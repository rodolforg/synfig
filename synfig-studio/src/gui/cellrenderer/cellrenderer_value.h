/* === S Y N F I G ========================================================= */
/*!	\file cellrenderer_value.h
**	\brief Template File
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

/* === S T A R T =========================================================== */

#ifndef __SYNFIG_GTKMM_CELLRENDERER_VALUE_H
#define __SYNFIG_GTKMM_CELLRENDERER_VALUE_H

/* === H E A D E R S ======================================================= */

#include <glibmm/property.h>

#include <gtkmm/cellrenderertext.h>

#include <synfig/paramdesc.h>
#include <synfig/value.h>

#include <synfigapp/canvasinterface.h>
#include <synfigapp/value_desc.h>


/* === M A C R O S ========================================================= */

/* === T Y P E D E F S ===================================================== */

/* === C L A S S E S & S T R U C T S ======================================= */

namespace studio {

class ValueBase_Entry;

class CellRenderer_ValueBase : public Gtk::CellRendererText
{
	sigc::signal<void, const Glib::ustring&>                    signal_secondary_click_;
	sigc::signal<void, const Glib::ustring&, synfig::ValueBase> signal_edited_;

	Glib::Property<synfig::ValueBase>            property_value_;
	Glib::Property<synfig::Canvas::Handle>       property_canvas_;
	Glib::Property<synfig::ParamDesc>            property_param_desc_;
	Glib::Property<synfigapp::ValueDesc>         property_value_desc_;
	Glib::Property<synfig::ParamDesc>            property_child_param_desc_;

	void string_edited_ (const Glib::ustring&, const Glib::ustring&);

	void gradient_edited(synfig::Gradient gradient,  Glib::ustring path);
	void color_edited   (synfig::Color    color,     Glib::ustring path);

	bool edit_value_done_called;

	synfig::ValueBase saved_data; //Issues 659, 526, 520

	etl::loose_handle<synfigapp::CanvasInterface> canvas_interface;

public:
	sigc::signal<void, const Glib::ustring&> &signal_secondary_click()
	{return signal_secondary_click_; }

	sigc::signal<void, const Glib::ustring&, synfig::ValueBase> &signal_edited()
	{return signal_edited_; }

	Glib::PropertyProxy<synfig::ValueBase>            property_value()            { return property_value_.get_proxy();}
	Glib::PropertyProxy<synfig::Canvas::Handle>       property_canvas()           { return property_canvas_.get_proxy();}
	Glib::PropertyProxy<synfig::ParamDesc>            property_param_desc()       { return property_param_desc_.get_proxy(); }
	Glib::PropertyProxy<synfigapp::ValueDesc>         property_value_desc()       { return property_value_desc_.get_proxy(); }
	Glib::PropertyProxy<synfig::ParamDesc>            property_child_param_desc() { return property_child_param_desc_.get_proxy(); }
	Glib::PropertyProxy<bool>                         property_inconsistent()     { return property_foreground_set(); }

	synfig::Canvas::Handle      get_canvas()const           { return property_canvas_; }
	synfig::ParamDesc           get_param_desc()const       { return property_param_desc_; }
	synfigapp::ValueDesc        get_value_desc()const       { return property_value_desc_; }
	synfig::ParamDesc           get_child_param_desc()const { return property_child_param_desc_; }

	CellRenderer_ValueBase();
	~CellRenderer_ValueBase();

	void set_canvas_interface(const etl::loose_handle<synfigapp::CanvasInterface>& x);

protected:
	ValueBase_Entry *value_entry;

	void on_value_editing_done();

	virtual void
	render_vfunc(
		const ::Cairo::RefPtr< ::Cairo::Context>& cr,
		Gtk::Widget& widget,
		const Gdk::Rectangle& background_area,
		const Gdk::Rectangle&  cell_area,
		Gtk::CellRendererState flags);

	virtual Gtk::CellEditable*
	start_editing_vfunc(
		GdkEvent* event,
		Gtk::Widget& widget,
		const Glib::ustring& path,
		const Gdk::Rectangle& background_area,
		const Gdk::Rectangle& cell_area,
		Gtk::CellRendererState flags);

}; // END of class CellRenderer_ValueBase

}; // END of namespace studio

/* === E N D =============================================================== */

#endif
