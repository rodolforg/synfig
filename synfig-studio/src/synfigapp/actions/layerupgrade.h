/* === S Y N F I G ========================================================= */
/*!	\file layerupgrade.h
**	\brief Upgrade a deprecated layer to its current replacement
**
**	\legal
**	Copyright (C) 2026 Synfig Contributors
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

#ifndef SYNFIG_APP_ACTION_LAYERUPGRADE_H
#define SYNFIG_APP_ACTION_LAYERUPGRADE_H

/* === H E A D E R S ======================================================= */

#include <list>
#include <synfig/layer.h>
#include <synfigapp/action.h>

/* === M A C R O S ========================================================= */

/* === T Y P E D E F S ===================================================== */

/* === C L A S S E S & S T R U C T S ======================================= */

namespace synfigapp {

namespace Action {

class LayerUpgrade :
	public Super
{
private:
	std::list<synfig::Layer::Handle> layers;

	// Returns the parameter name that flags layer is deprecated, or empty if unknown.
	static synfig::String get_layer_param(const synfig::Layer::LooseHandle& layer);

public:

	static ParamVocab get_param_vocab();
	static bool is_candidate(const ParamList& x);

	virtual bool set_param(const synfig::String& name, const Param&);
	virtual bool is_ready() const;

	virtual void prepare();

	ACTION_MODULE_EXT
};

}; // END of namespace Action
}; // END of namespace synfigapp

/* === E N D =============================================================== */

#endif
