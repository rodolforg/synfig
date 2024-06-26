/* === S Y N F I G ========================================================= */
/*!	\file timepointcollect.h
**	\brief Template Header
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

#ifndef __SYNFIG_TIMEPOINTCOLLECT_H
#define __SYNFIG_TIMEPOINTCOLLECT_H

/* === H E A D E R S ======================================================= */

#include <set>
#include "activepoint.h"
#include "waypoint.h"
#include "node.h"
#include "time.h"

/* === M A C R O S ========================================================= */

/* === T Y P E D E F S ===================================================== */

/* === C L A S S E S & S T R U C T S ======================================= */

namespace synfig {

//! \writeme
int waypoint_collect(std::set<Waypoint, std::less<UniqueID> >& waypoint_set, const Time& time, const etl::handle<Node>& node, bool ignore_dynamic_parameters = false);

//! Search for a specific waypoint (by its uid) in node.
bool waypoint_search(Waypoint& waypoint, const UniqueID& uid, const etl::handle<Node>& node);

//! \writeme
int activepoint_collect(std::set<Activepoint, std::less<UniqueID> >& activepoint_set,const Time& time, const etl::handle<Node>& node);

}; // END of namespace synfig

/* === E N D =============================================================== */

#endif
