/* === S Y N F I G ========================================================= */
/*!	\file docks/dock_switchdrawings.h
**	\brief Header File of Switchdrawings Panel
**
**	\legal
**	Copyright (c) 2002-2005 Robert B. Quattlebaum Jr., Adrian Bentley
**	Copyright (c) 2026 Synfig contributors
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

#ifndef SYNFIG_STUDIO_DOCK_SWITCHDRAWINGS_H
#define SYNFIG_STUDIO_DOCK_SWITCHDRAWINGS_H

/* === H E A D E R S ======================================================= */

#include <gtkmm/actiongroup.h>
#include <gtkmm/flowbox.h>
#include <gtkmm/label.h>
#include <gtkmm/radiobutton.h>

#include <gui/docks/dock_canvasspecific.h>

#include <synfig/layers/layer_switch.h>

/* === M A C R O S ========================================================= */

/* === T Y P E D E F S ===================================================== */

/* === C L A S S E S & S T R U C T S ======================================= */

namespace studio {


class Dock_Switchdrawings : public Dock_CanvasSpecific
{
public:
	Dock_Switchdrawings();
	~Dock_Switchdrawings();

protected:
	void init_canvas_view_vfunc(etl::loose_handle<CanvasView> canvas_view) override;
	void changed_canvas_view_vfunc(etl::loose_handle<CanvasView> canvas_view) override;

private:
	Glib::RefPtr<Gtk::ActionGroup> action_group_;
	sigc::connection connection_selection_changed;
	sigc::connection connection_canvas_time_changed;
	sigc::connection connection_layer_switch_children_changed;
	sigc::connection connection_layer_switch_param_changed;
	sigc::connection connection_layer_switch_static_param_changed;
	sigc::connection connection_layer_switch_dynamic_param_changed;

	Gtk::Label* label_no_selection_;
	Gtk::Label* label_multiple_selection_;
	Gtk::FlowBox* fbox_;
	Gtk::RadioButtonGroup radio_group_;
	struct DrawingItem {
		Gtk::Widget* container;
		Gtk::RadioButton* button;
		Gtk::Image* image;
	};
	std::vector<DrawingItem> drawing_items_;
	DrawingItem empty_drawing_item_;
	/** Used to prevent issue with Gtk::manage()
	 *  in create_drawing_item() used to create empty_drawing_line_.
	 *  Here the FlowBoxChild is not created with Gtk::manage().
	 */
	Gtk::FlowBoxChild flowbox_child_empty_item_;
	std::vector<synfig::rendering::TaskEvent::Handle> task_events_;

	/** protects drawing_items_ and task_events_ */
	std::mutex mutex_drawings;

	etl::handle<synfig::Layer_Switch> selected_layer_switch;

	void refresh_toolbar();
	void refresh_drawings();
	void render_drawing(int idx);
	void cancel_pending_rendering();
	void switch_to_drawing(int idx);
	void update_number_of_drawings();
	void clear_layer_switch_widgets_and_connections();

	void on_layer_selection_changed();
	void on_layer_switch_param_changed(const std::string& param_name);
	void on_canvas_time_changed();
	void on_drawing_item_clicked(int i);
	void on_drawing_rendered(bool success, synfig::rendering::SurfaceResource::Handle s, int idx, bool flip_v);
	void on_next_pressed();
	void on_previous_pressed();
	void on_duplicate_pressed();
	void on_refresh_pressed();

	DrawingItem create_drawing_item(int idx);
	DrawingItem create_drawing_item(const std::string& title, int idx);

}; // END of Dock_Switchdrawings

}; // END of namespace studio

/* === E N D =============================================================== */

#endif
