/* === S Y N F I G ========================================================= */
/*!	\file statemanager.cpp
**	\brief Template File
**
**	\legal
**	Copyright (c) 2002-2005 Robert B. Quattlebaum Jr., Adrian Bentley
**	Copyright (c) 2007 Chris Moore
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

#include "statemanager.h"

#include <gui/actionmanagers/actionmanager.h>
#include <gui/app.h>
#include <gui/docks/dock_toolbox.h>
#include "gui/localization.h"
#endif

/* === U S I N G =========================================================== */

using namespace synfig;
using namespace studio;

/* === M A C R O S ========================================================= */

/* === G L O B A L S ======================================================= */

static const ActionManager::EntryList known_states_db =
{
	{"app.set-tool-normal",      N_("Transform Tool"),  {"s"}, "tool_normal_icon"},
	{"app.set-tool-smooth_move", N_("SmoothMove Tool"), {"m"}, "tool_smooth_move_icon"},
	{"app.set-tool-scale",       N_("Scale Tool"),      {"l"}, "tool_scale_icon"},
	{"app.set-tool-rotate",      N_("Rotate Tool"),     {"a"}, "tool_rotate_icon"},
	{"app.set-tool-mirror",      N_("Mirror Tool"),     {"i"}, "tool_mirror_icon"},
	{"app.set-tool-circle",      N_("Circle Tool"),     {"e"}, "tool_circle_icon"},
	{"app.set-tool-rectangle",   N_("Rectangle Tool"),  {"r"}, "tool_rectangle_icon"},
	{"app.set-tool-star",        N_("Star Tool"),       {"asterisk"}, "tool_star_icon"},
	{"app.set-tool-gradient",    N_("Gradient Tool"),   {"g"}, "tool_gradient_icon"},
	{"app.set-tool-polygon",     N_("Polygon Tool"),    {"o"}, "tool_polyline_icon"}, // icon name does not match state name
	{"app.set-tool-bline",       N_("Spline Tool"),     {"b"}, "tool_spline_icon"},   // icon name does not match state name
	{"app.set-tool-bone",        N_("Skeleton Tool"),   {"n"}, "tool_skeleton_icon"}, // icon name does not match state name
	{"app.set-tool-text",        N_("Text Tool"),       {"t"}, "tool_text_icon"},
	{"app.set-tool-fill",        N_("Fill Tool"),       {"u"}, "tool_fill_icon"},
	{"app.set-tool-eyedrop",     N_("Eyedrop Tool"),    {"d"}, "tool_eyedrop_icon"},
	{"app.set-tool-lasso",       N_("Cutout Tool"),     {"c"}, "tool_cutout_icon"},   // icon name does not match state name
	{"app.set-tool-zoom",        N_("Zoom Tool"),       {"z"}, "tool_zoom_icon"},
	{"app.set-tool-draw",        N_("Draw Tool"),       {"p"}, "tool_draw_icon"},
	{"app.set-tool-sketch",      N_("Sketch Tool"),     {"k"}, "tool_sketch_icon"},
	{"app.set-tool-width",       N_("Width Tool"),      {"w"}, "tool_width_icon"},
	{"app.set-tool-brush",       N_("Brush Tool"),      {}, "tool_brush_icon"},
};

/* === P R O C E D U R E S ================================================= */

/* === M E T H O D S ======================================================= */

StateManager::StateManager()
{
}

StateManager::~StateManager()
{
}

void
StateManager::change_state_(const Smach::state_base *state)
{
	App::dock_toolbox->change_state_(state);
}

void
StateManager::change_state_to(const std::string& state)
{
	App::dock_toolbox->change_state(state, true);
}

void
StateManager::add_state(const Smach::state_base* state)
{
	String name(state->get_name());

	for (const auto& entry : known_states_db)
		App::get_action_manager()->add(entry);

	App::instance()->add_action("set-tool-" + name, sigc::bind(
												sigc::mem_fun(*this, &studio::StateManager::change_state_),
												state
											));
	App::dock_toolbox->add_state(state);
}
