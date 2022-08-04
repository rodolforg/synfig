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

#include <gui/app.h>
#include <gui/docks/dock_toolbox.h>

#endif

/* === U S I N G =========================================================== */

using namespace synfig;
using namespace studio;

/* === M A C R O S ========================================================= */

/* === G L O B A L S ======================================================= */

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
StateManager::add_state(const Smach::state_base* state, const String& local_name)
{
	String name(state->get_name());

	App::instance()->add_action("set-tool-" + name, sigc::bind(
												sigc::mem_fun(*this, &studio::StateManager::change_state_),
												state
											));
	App::dock_toolbox->add_state(state, local_name);
}
