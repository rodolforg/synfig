/* === S Y N F I G ========================================================= */
/*!	\file layer_cobratest.h
**	\brief Header for example layer to test cobra render stuff
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

#ifndef SYNFIG_LAYER_COBRA_TEST_H
#define SYNFIG_LAYER_COBRA_TEST_H

/* === H E A D E R S ======================================================= */

#include <synfig/layer.h>

/* === M A C R O S ========================================================= */

/* === T Y P E D E F S ===================================================== */

/* === C L A S S E S & S T R U C T S ======================================= */

namespace synfig {

/**	A layer to play with Cobra renderer */
class Layer_CobraTest : public Layer
{
	SYNFIG_LAYER_MODULE_EXT

public:
	Layer_CobraTest();

	bool set_param(const String& param, const ValueBase& value) override;

	ValueBase get_param(const String& param) const override;

	Vocab get_param_vocab() const override;

	rendering::Task::Handle build_rendering_task_vfunc(Context context) const override;
};

}; // END of namespace synfig

/* === E N D =============================================================== */

#endif
