/* === S Y N F I G ========================================================= */
/*!	\file dock_switchdrawings.cpp
**	\brief Implementation file for Switchdrawings Panel
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

/* === H E A D E R S ======================================================= */

#ifdef USING_PCH
#	include "pch.h"
#else
#ifdef HAVE_CONFIG_H
#	include <config.h>
#endif

#include "docks/dock_switchdrawings.h"

#include <synfig/general.h>
#include <synfig/layers/layer_group.h>
#include <synfig/rendering/renderer.h>
#include <synfig/rendering/common/task/tasktransformation.h>
#include <synfig/threadpool.h>

#include <gui/app.h>
#include <gui/canvasview.h>
#include <gui/localization.h>

#endif

/* === U S I N G =========================================================== */

using namespace synfig;
using namespace studio;

/* === M A C R O S ========================================================= */

/* === G L O B A L S ======================================================= */

static const VectorInt default_drawing_image_size{100, 100};

/* === P R O C E D U R E S ================================================= */

static int
get_layer_switch_current_depth(etl::handle<Layer_Switch> selected_layer_switch)
{
	if (!selected_layer_switch)
		return -1;

	const std::string layer_name = selected_layer_switch->get_param("layer_name").get(std::string());
	if (!layer_name.empty()) {
		if (!selected_layer_switch->get_sub_canvas())
			return -1;
		int layer_depth = 0;
		for (IndependentContext i = selected_layer_switch->get_sub_canvas()->get_independent_context(); *i; ++i, ++layer_depth) {
			if ((*i)->get_description() == layer_name)
				return layer_depth;
		}
	}
	const int layer_depth = selected_layer_switch->get_param("layer_depth").get(int());
	return layer_depth;
}

static void
create_transparent_image(Gtk::Image* image, const VectorInt& size)
{
	if (!image || size[0] <= 0 || size[1] <= 0)
		return;
	const int square_size = 10;
	const Color color1(Color::alpha());
	const Color color2(Color::gray());
	auto pixbuf = Gdk::Pixbuf::create(Gdk::Colorspace::COLORSPACE_RGB, true, 8, size[0], size[1]);
	pixbuf->fill(0);
	image->set(pixbuf);
}

/* === M E T H O D S ======================================================= */

Dock_Switchdrawings::Dock_Switchdrawings()
	: Dock_CanvasSpecific("switchdrawings", _("Switch Drawings"), "switchdrawings_icon")
	, action_group_(Gtk::ActionGroup::create("action_group_dock_switchdrawings"))
	, empty_drawing_item_(create_drawing_item(_("None"), -1))
{
	// Make toolbar small for space efficiency
	get_style_context()->add_class("synfigstudio-efficient-workspace");

	create_transparent_image(empty_drawing_item_.image, default_drawing_image_size);
	flowbox_child_empty_item_.add(*empty_drawing_item_.container);

	action_group_->add(
		Gtk::Action::create_with_icon_name(
			"action-SwitchdrawingsNext",
			"go-next",
			_("Next drawing"),
			_("Switch to next drawing")
		),
		sigc::mem_fun(*this, &Dock_Switchdrawings::on_next_pressed)
	);

	action_group_->add(
		Gtk::Action::create_with_icon_name(
			"action-SwitchdrawingsPrevious",
			"go-previous",
			_("Previous drawing"),
			_("Switch to previous drawing")
		),
		sigc::mem_fun(*this, &Dock_Switchdrawings::on_previous_pressed)
	);

	action_group_->add(
		Gtk::Action::create_with_icon_name(
			"action-SwitchdrawingsDuplicate",
			"duplicate_icon",
			_("Duplicate"),
			_("Duplicate current drawing")
		),
		sigc::mem_fun(*this, &Dock_Switchdrawings::on_duplicate_pressed)
	);

	action_group_->add(
		Gtk::Action::create_with_icon_name(
			"action-SwitchdrawingsRefresh",
			"view-refresh",
			_("Refresh"),
			_("Update the possible drawings for selected layer")
		),
		sigc::mem_fun(*this, &Dock_Switchdrawings::on_refresh_pressed)
	);

	action_group_->add(Gtk::Action::create("toolbar-switchdrawings", _("Switch Drawings")) );
	App::ui_manager()->insert_action_group(action_group_);

	Glib::ustring ui_info =
		"<ui>"
		" <toolbar action='toolbar-switchdrawings'>"
		"  <toolitem action='action-SwitchdrawingsPrevious' />"
		"  <toolitem action='action-SwitchdrawingsNext' />"
		"  <separator />"
		"  <toolitem action='action-SwitchdrawingsDuplicate' />"
		"  <separator />"
		"  <toolitem action='action-SwitchdrawingsRefresh' />"
		" </toolbar>"
		"</ui>";

	App::ui_manager()->add_ui_from_string(ui_info);

	action_group_->set_sensitive(false);

	if (Gtk::Toolbar* toolbar = dynamic_cast<Gtk::Toolbar*>(App::ui_manager()->get_widget("/toolbar-switchdrawings"))) {
		set_toolbar(*toolbar);
	}

	Gtk::Box* vbox = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL));
	vbox->show();
	add(*vbox);

	fbox_ = Gtk::manage(new Gtk::FlowBox());
	fbox_->set_selection_mode(Gtk::SELECTION_NONE);
	fbox_->show();
	vbox->pack_end(*fbox_);

	label_no_selection_ = Gtk::manage(new Gtk::Label(_("No Switch Layer is selected")));
	label_multiple_selection_ = Gtk::manage(new Gtk::Label(_("Multiple layers are selected")));
	vbox->pack_start(*label_no_selection_);
	vbox->pack_start(*label_multiple_selection_);

	label_no_selection_->show();
	refresh_toolbar();
}

Dock_Switchdrawings::~Dock_Switchdrawings()
{
}

void
Dock_Switchdrawings::init_canvas_view_vfunc(CanvasView::LooseHandle canvas_view)
{
}

void
Dock_Switchdrawings::changed_canvas_view_vfunc(CanvasView::LooseHandle canvas_view)
{
	connection_selection_changed.disconnect();
	selected_layer_switch = nullptr;
	clear_layer_switch_widgets_and_connections();

	if (canvas_view) {
		action_group_->set_sensitive(true);
		Gtk::TreeView* layer_tree_view = dynamic_cast<Gtk::TreeView*>(canvas_view->get_ext_widget("layers"));
		connection_selection_changed = layer_tree_view->get_selection()->signal_changed().connect(sigc::mem_fun(*this, &Dock_Switchdrawings::on_layer_selection_changed));
		on_layer_selection_changed();
	} else {
		action_group_->set_sensitive(false);
	}
}

void
Dock_Switchdrawings::on_next_pressed()
{
	if (!selected_layer_switch)
		return;
	const int idx = get_layer_switch_current_depth(selected_layer_switch);
	if (!selected_layer_switch->get_sub_canvas() || idx >= selected_layer_switch->get_sub_canvas()->size() - 1)
		return;
	switch_to_drawing(idx + 1);
}

void
Dock_Switchdrawings::on_previous_pressed()
{
	if (!selected_layer_switch)
		return;
	const int idx = get_layer_switch_current_depth(selected_layer_switch);
	if (idx <= 0)
		return;
	switch_to_drawing(idx - 1);
}

void
Dock_Switchdrawings::on_duplicate_pressed()
{
	if (!selected_layer_switch)
		return;
	Layer::Handle layer = selected_layer_switch->get_current_layer();
	if (!layer)
		return;

	Canvas::Handle canvas = get_canvas_view()->get_canvas();
	etl::handle<synfigapp::CanvasInterface> canvas_interface = get_canvas_view()->canvas_interface();
	etl::handle<synfigapp::Instance> instance = canvas_interface->get_instance();

	synfigapp::Action::PassiveGrouper group(instance.get(), _("Duplicate Drawing"));

	synfigapp::Action::Handle action = synfigapp::Action::create("LayerDuplicate");
	action->set_param("layer", layer);
	action->set_param("canvas", canvas);
	action->set_param("canvas_interface", canvas_interface);
	instance->perform_action(action);

	action = synfigapp::Action::create("LayerMove");
	action->set_param("layer", layer);
	action->set_param("new_index", -1);
	action->set_param("canvas", canvas);
	action->set_param("canvas_interface", canvas_interface);
	instance->perform_action(action);
}

void
Dock_Switchdrawings::on_refresh_pressed()
{
	// cancel_pending_rendering();
	update_number_of_drawings();
	// refresh_drawings();
}

void
Dock_Switchdrawings::on_layer_selection_changed()
{
	if (!get_canvas_view())
		return;

	const int layer_count = get_canvas_view()->get_selection_manager()->get_selected_layer_count();
	if (layer_count != 1) {
		if (!selected_layer_switch)
			return;
		selected_layer_switch = nullptr;
	} else {
		Layer::Handle layer = get_canvas_view()->get_selection_manager()->get_selected_layer();
		auto layer_switch = etl::handle<Layer_Switch>::cast_dynamic(layer);
		if (!layer_switch) {
			// Accept Group with multiple sync'ed Switch as children too
			auto layer_group = etl::handle<Layer_Group>::cast_dynamic(layer);
			if (layer_group) {
				if (layer_group->get_sub_canvas()->size() > 0) {
					const auto lsw1 = etl::handle<Layer_Switch>::cast_dynamic(layer_group->get_sub_canvas()->front());
					if (lsw1) {
						const auto lsw1_depth_vn_it = lsw1->dynamic_param_list().find("layer_depth");;
						const auto lsw1_depth_vn = lsw1_depth_vn_it != lsw1->dynamic_param_list().end() ? lsw1_depth_vn_it->second : nullptr;
						const auto lsw1_name_vn_it = lsw1->dynamic_param_list().find("layer_name");
						const auto lsw1_name_vn = lsw1_name_vn_it != lsw1->dynamic_param_list().end() ? lsw1_name_vn_it->second : nullptr;
						enum class CommonParameter {
							UNKNOWN,
							NAME,
							DEPTH,
						} used_parameter = CommonParameter::UNKNOWN;

						for (const auto& item : *layer_group->get_sub_canvas()) {
							auto lsw = etl::handle<Layer_Switch>::cast_dynamic(layer);
							if (!lsw)
								break;
							const auto lsw_depth_vn_it = lsw->dynamic_param_list().find("layer_depth");;
							const auto lsw_depth_vn = lsw_depth_vn_it != lsw->dynamic_param_list().end() ? lsw_depth_vn_it->second : nullptr;
							const auto lsw_name_vn_it = lsw->dynamic_param_list().find("layer_name");
							const auto lsw_name_vn = lsw_name_vn_it != lsw->dynamic_param_list().end() ? lsw_name_vn_it->second : nullptr;
							if (used_parameter == CommonParameter::UNKNOWN) {
								if (lsw_depth_vn == lsw1_depth_vn) {
									used_parameter = CommonParameter::DEPTH;
									continue;
								}
								if (lsw_name_vn == lsw1_name_vn) {
									used_parameter = CommonParameter::NAME;
									continue;
								}
								break;
							} else if (used_parameter == CommonParameter::NAME) {
								if (lsw_name_vn != lsw1_name_vn)
									break;
							} else if (used_parameter == CommonParameter::DEPTH) {
								if (lsw_depth_vn != lsw1_depth_vn)
									break;
							}
						}

						layer_switch = lsw1;
					}
				}
			}
		}
		fprintf(stderr, "selection changed! %s -> %s\n", selected_layer_switch ? selected_layer_switch->get_name().c_str() : "null", layer ? layer->get_name().c_str() : "null");
		if (layer_switch != selected_layer_switch) {
			selected_layer_switch = layer_switch;
		} else {
			return;
		}
	}
	fprintf(stderr, "selection changed!\n");
	// clear stuff first
	clear_layer_switch_widgets_and_connections();

	// Update stuff according to selection
	if (layer_count > 1) {
		label_multiple_selection_->show();
	} else if (layer_count == 0 || !selected_layer_switch) {
		label_no_selection_->show();
	} else {
		fbox_->add(flowbox_child_empty_item_);
		const int num_drawings = selected_layer_switch->get_sub_canvas()->size();
		for (int i = 0; i < num_drawings; ++i) {
			auto item = create_drawing_item(i);
			{
				std::lock_guard<std::mutex> lock(mutex_drawings);
				if (i >= drawing_items_.size())
					drawing_items_.push_back(item);
				else
					drawing_items_[i] = item;
				fbox_->add(*item.container);
			}
		}
		const int layer_depth = get_layer_switch_current_depth(selected_layer_switch);
		if (layer_depth >= 0 && layer_depth < drawing_items_.size()) {
			drawing_items_[layer_depth].button->set_active(true);
		}
		refresh_drawings();
		fbox_->show_all();

		// missing way to detect when a child layer changes to refresh that specific drawing image
		// missing way to detect when children count changes to add/remove/refresh drawing images
		connection_canvas_time_changed = get_canvas_interface()->signal_time_changed().connect(sigc::mem_fun(*this, &Dock_Switchdrawings::on_canvas_time_changed));
		connection_layer_switch_children_changed = selected_layer_switch->signal_possible_layers_changed().connect(sigc::mem_fun(*this, &Dock_Switchdrawings::update_number_of_drawings));
		connection_layer_switch_param_changed = selected_layer_switch->signal_param_changed().connect(sigc::mem_fun(*this, &Dock_Switchdrawings::on_layer_switch_param_changed));
		connection_layer_switch_static_param_changed = selected_layer_switch->signal_static_param_changed().connect(sigc::mem_fun(*this, &Dock_Switchdrawings::on_layer_switch_param_changed));
		connection_layer_switch_dynamic_param_changed = selected_layer_switch->signal_dynamic_param_changed().connect(sigc::mem_fun(*this, &Dock_Switchdrawings::on_layer_switch_param_changed));
	}

	refresh_toolbar();
}

void
Dock_Switchdrawings::refresh_toolbar()
{
	int idx = -1;
	int n_items = -1;
	if (selected_layer_switch) {
		idx = get_layer_switch_current_depth(selected_layer_switch);
		n_items = selected_layer_switch->get_sub_canvas()->size();
	}
	action_group_->get_action("action-SwitchdrawingsPrevious")->set_sensitive(idx > 0);
	action_group_->get_action("action-SwitchdrawingsNext")->set_sensitive(idx < n_items - 1);
	action_group_->get_action("action-SwitchdrawingsDuplicate")->set_sensitive(idx >= 0 && idx < n_items);
}

void
Dock_Switchdrawings::refresh_drawings()
{
	std::lock_guard<std::mutex> lock(mutex_drawings);

	for (auto& t : task_events_) {
		t->finish(false);
	}
	task_events_.clear();

	const int size = drawing_items_.size();
	for (int idx = 0; idx < size; ++idx)
		render_drawing(idx);
}

void
Dock_Switchdrawings::render_drawing(int idx)
{
	// mutex_drawing must be locked
	// bool must_unlock = mutex_drawings.try_lock();
	// class LockIfNot{}

	const Layer::Handle layer_to_render = *selected_layer_switch->get_sub_canvas()->byindex(idx);
	if (!layer_to_render) {
		synfig::error(_("Internal error: no sublayer at index #%i"), idx);
		return;
	}
	const Layer_PasteCanvas::Handle pastecanvas_to_render = Layer_PasteCanvas::Handle::cast_dynamic(layer_to_render);
	Rect layer_rect = !pastecanvas_to_render ? layer_to_render->get_bounding_rect() : pastecanvas_to_render->get_bounding_rect_context_dependent(ContextParams());
	if (layer_rect.is_nan_or_inf())
		layer_rect = get_canvas_view()->get_canvas()->rend_desc().get_rect();

	ContextParams context_params;
	context_params.z_range = true;
	context_params.z_range_depth = 0;
	context_params.z_range_position = idx;
	rendering::Task::Handle task = selected_layer_switch->get_sub_canvas()->build_rendering_task(context_params);
	if (!task)
		task = new rendering::TaskSurface();

	bool flip_v = false;
	bool flip_h = false;
	{
		const RendDesc& rend_desc = selected_layer_switch->get_canvas()->get_root()->rend_desc();
		const Vector p0 = rend_desc.get_tl();
		const Vector p1 = rend_desc.get_br();
		flip_h = p0[0] > p1[0];
		flip_v = p0[1] > p1[1];
	}

	VectorInt raster_size{default_drawing_image_size};
	// Fix aspect ratio if needed
	// FIXME if default_drawing_image_size isn't square
	if (layer_rect.get_width() > layer_rect.get_height()) {
		raster_size[1] = layer_rect.get_height() / layer_rect.get_width() * raster_size[0];
	} else if (layer_rect.get_width() < layer_rect.get_height()) {
		raster_size[0] = layer_rect.get_width() / layer_rect.get_height() * raster_size[1];
	}

	task->target_surface = new rendering::SurfaceResource();
	task->target_surface->create(raster_size[0], raster_size[1]);
	task->target_rect = RectInt(VectorInt(), task->target_surface->get_size());
	task->source_rect = Rect(layer_rect.get_min(), layer_rect.get_max());

	rendering::TaskEvent::Handle task_event = new rendering::TaskEvent();
	task_event->signal_finished.connect(
		sigc::bind(
			sigc::bind(
				sigc::bind(
					sigc::mem_fun(*this, &Dock_Switchdrawings::on_drawing_rendered),
					flip_v
					),
				idx
				),
			task->target_surface
			)
		);

	task_events_.push_back(task_event);

	rendering::Renderer::Handle renderer = rendering::Renderer::get_renderer(App::workarea_renderer);
	if (!renderer) {
		synfig::error(_("Internal error: there is no renderer called %s"), App::workarea_renderer.c_str());
		return;
	}

	// Renderer::enqueue contains the expensive 'optimization' stage, so call it async
	ThreadPool::instance().enqueue( sigc::bind(
		sigc::ptr_fun(&rendering::Renderer::enqueue_task_func),
		renderer, task, task_event, false ));
}

void
Dock_Switchdrawings::cancel_pending_rendering()
{
	std::lock_guard<std::mutex> lock(mutex_drawings);

	for (auto& t : task_events_) {
		t->finish(false);
	}
	task_events_.clear();
}

void
Dock_Switchdrawings::switch_to_drawing(int idx)
{
	synfigapp::Action::PassiveGrouper action_group(get_canvas_view()->get_instance().get(), _("Change Switch Drawing"));
	get_canvas_interface()->change_value(synfigapp::ValueDesc(Layer::Handle(selected_layer_switch.get()), "layer_depth"), ValueBase(idx));
	get_canvas_interface()->change_value(synfigapp::ValueDesc(Layer::Handle(selected_layer_switch.get()), "layer_name"), ValueBase(""));
	refresh_toolbar();
}

void Dock_Switchdrawings::update_number_of_drawings()
{
	fprintf(stderr, "%s\n", __PRETTY_FUNCTION__);
	{
		// {
		std::lock_guard<std::mutex> lock(mutex_drawings);

		for (auto& t : task_events_) {
			t->finish(false);
		}
		task_events_.clear();
		//}

		const int num_drawings = selected_layer_switch ? selected_layer_switch->get_sub_canvas()->size() : 0;
		const int num_existent_drawings = drawing_items_.size();

		if (num_drawings < num_existent_drawings) {
			for (int i = num_drawings; i < num_existent_drawings; ++i) {
				fbox_->get_child_at_index(i)->remove();
				drawing_items_.pop_back();
			}
		} else {
			for (int i = num_existent_drawings; i < num_drawings; ++i) {
				auto item = create_drawing_item(i);
				{
					// std::lock_guard<std::mutex> lock(mutex_drawings);
					if (i >= drawing_items_.size())
						drawing_items_.push_back(item);
					else
						drawing_items_[i] = item;
					fbox_->add(*item.container);
				}
			}
		}
		if (selected_layer_switch) {
			const int layer_depth = get_layer_switch_current_depth(selected_layer_switch);
			if (layer_depth >= 0 && layer_depth < drawing_items_.size()) {
				drawing_items_[layer_depth].button->set_active(true);
			}
		}
		fbox_->show_all();
	}

	refresh_drawings();
}

void
Dock_Switchdrawings::clear_layer_switch_widgets_and_connections()
{
	connection_canvas_time_changed.disconnect();
	connection_layer_switch_children_changed.disconnect();
	connection_layer_switch_param_changed.disconnect();
	connection_layer_switch_static_param_changed.disconnect();
	connection_layer_switch_dynamic_param_changed.disconnect();
	label_no_selection_->hide();
	label_multiple_selection_->hide();

	{
		std::lock_guard<std::mutex> lock(mutex_drawings);

		for (auto& t : task_events_) {
			t->finish(false);
		}
		task_events_.clear();

		drawing_items_.clear();
	}

	if (fbox_) {
		while (auto child = fbox_->get_child_at_index(0))
			fbox_->remove(*child);
	}
}

void
Dock_Switchdrawings::on_layer_switch_param_changed(const std::string& param_name)
{
	fprintf(stderr, "%s\n", __PRETTY_FUNCTION__);
	if (param_name != "layer_name" && param_name != "layer_depth")
		return;
	const int layer_depth = get_layer_switch_current_depth(selected_layer_switch);
	fprintf(stderr, "changed to depth: %i\n", layer_depth);
	if (layer_depth >= 0 && layer_depth < drawing_items_.size()) {
		drawing_items_[layer_depth].button->set_active(true);
	} else {
		// invalid layer_depth!
		empty_drawing_item_.button->set_active(true);
	}
	refresh_toolbar();
}

void
Dock_Switchdrawings::on_canvas_time_changed()
{
	if (selected_layer_switch) {
		const int new_depth = get_layer_switch_current_depth(selected_layer_switch);
		std::lock_guard<std::mutex> lock(mutex_drawings);
		if (new_depth >= 0 && new_depth < drawing_items_.size()) {
			auto& button = drawing_items_[new_depth].button;
			if (!button->get_active())
				button->set_active(true);
		} else {
			auto& button = empty_drawing_item_.button;
			if (!button->get_active())
				button->set_active(true);
		}
	}
}

void
Dock_Switchdrawings::on_drawing_item_clicked(int i)
{
	if (i >= 0 && i < drawing_items_.size()) {
		const bool active = drawing_items_[i].button->get_active();
		if (active) {
			switch_to_drawing(i);
		}
	} else {
		const bool active = empty_drawing_item_.button->get_active();
		if (active) {
			switch_to_drawing(-1);
		}
	}
}

void
Dock_Switchdrawings::on_drawing_rendered(bool success, synfig::rendering::SurfaceResource::Handle s, int idx, bool flip_v)
{
	fprintf(stderr, "acabou #%i : %b\n", idx, success);
	if (!success) {
		fprintf(stderr, "\tcancelado\n");
		return;
	}

	auto pixbuf = Gdk::Pixbuf::create(Gdk::Colorspace::COLORSPACE_RGB, true, 8, default_drawing_image_size[0], default_drawing_image_size[1]);
	if (!pixbuf)
		return;
	pixbuf->fill(0);

	rendering::SurfaceResource::LockReadBase surface_lock(s);
	if (surface_lock.get_resource() && surface_lock.get_resource()->is_blank()) {
	} else {
		if (surface_lock.convert(rendering::Surface::Token::Handle(), false, true)) {
			const rendering::Surface& s = *surface_lock.get_surface();
			const Color* src = s.get_pixels_pointer();
			const int pad_y = int((pixbuf->get_height() - s.get_height()) / 2);
			const int pad_x = int((pixbuf->get_width() - s.get_width()) / 2);
			if (pad_x < 0 || pad_y < 0) {
				synfig::error(
					_("Internal error: invalid size of rendered Drawing: %i x %i; should be smaller than or equals to %i x %i"),
					s.get_width(), s.get_height(),
					pixbuf->get_width(), pixbuf->get_height()
				);
				return;
			}
			auto dst = pixbuf->get_pixels() + pixbuf->get_rowstride() * pad_y;
			if (pad_x == 0) {
				color_to_pixelformat(dst, src, PF_RGB | PF_A, nullptr, s.get_width(), s.get_height(), pixbuf->get_rowstride(), 0);
			} else {
				dst += pad_x * pixbuf->get_bits_per_sample() * pixbuf->get_n_channels() / 8;
				for (int i = 2*pad_y; i < pixbuf->get_height(); ++i) {
					color_to_pixelformat(dst, src, PF_RGB | PF_A, nullptr, s.get_width(), 1, pixbuf->get_rowstride());
					dst += pixbuf->get_rowstride();
					src += s.get_width();
				}
			}
			if (flip_v)
				pixbuf = pixbuf->flip(false);
		}
	}

	{
		std::lock_guard<std::mutex> lock(mutex_drawings);
		if (idx < 0 || idx > drawing_items_.size()) {
			synfig::warning(_("Trying to draw rendered image of invalid index: %i\n"), idx);
			return;
		}
		drawing_items_[idx].image->set(pixbuf);
	}
}

Dock_Switchdrawings::DrawingItem
Dock_Switchdrawings::create_drawing_item(int idx)
{
	return create_drawing_item(strprintf(_("#%d"), idx), idx);
}

Dock_Switchdrawings::DrawingItem
Dock_Switchdrawings::create_drawing_item(const std::string& title, int idx)
{
	Gtk::Box* box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL));

	auto image = Gtk::manage(new Gtk::Image());
	image->property_icon_name() = "image-missing";
	box->pack_start(*image);

	Gtk::Label* label = Gtk::manage(new Gtk::Label(title));
	label->set_vexpand(false);
	box->pack_start(*label, Gtk::PACK_SHRINK);

	box->show_all();

	Gtk::RadioButton* bt = Gtk::manage(new Gtk::RadioButton(radio_group_));
	bt->property_draw_indicator() = false;
	bt->signal_clicked().connect(sigc::bind(sigc::mem_fun(*this, &Dock_Switchdrawings::on_drawing_item_clicked), idx));
	bt->set_vexpand(true);
	bt->add(*box);

	return DrawingItem{bt, bt, image};
}
