/* === S Y N F I G ========================================================= */
/*!	\file layerupgrade.cpp
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

/* === H E A D E R S ======================================================= */

#ifdef USING_PCH
#	include "pch.h"
#else
#ifdef HAVE_CONFIG_H
#	include <config.h>
#endif

#include <algorithm>
#include <utility>

#include <synfig/general.h>

#include "layerupgrade.h"

#include <synfigapp/canvasinterface.h>
#include <synfigapp/localization.h>

#endif

using namespace synfig;
using namespace synfigapp;
using namespace Action;

/* === M A C R O S ========================================================= */

ACTION_INIT(Action::LayerUpgrade);
ACTION_SET_NAME(Action::LayerUpgrade,"LayerUpgrade");
ACTION_SET_LOCAL_NAME(Action::LayerUpgrade,N_("Upgrade Layer"));
ACTION_SET_TASK(Action::LayerUpgrade,"upgrade");
ACTION_SET_CATEGORY(Action::LayerUpgrade,Action::CATEGORY_LAYER);
ACTION_SET_PRIORITY(Action::LayerUpgrade,0);
ACTION_SET_VERSION(Action::LayerUpgrade,"0.0");

/* === G L O B A L S ======================================================= */

/* === P R O C E D U R E S ================================================= */

/* === M E T H O D S ======================================================= */

synfig::String
Action::LayerUpgrade::get_layer_param(const Layer::LooseHandle& layer)
{
	struct BrokenInfo
	{
		const char* layer_name;
		const char* param_name;
		bool param_value;
	};
	static const BrokenInfo table[] = {
									   { "bevel", "broken_rendering_0_3", true },
									   };
	for (const auto& entry : table)
		if (layer->get_name() == entry.layer_name)
			if (layer->get_param(entry.param_name).get(bool()) == entry.param_value)
				return entry.param_name;
	return synfig::String{};
}

Action::ParamVocab
Action::LayerUpgrade::get_param_vocab()
{
	ParamVocab ret(Action::CanvasSpecific::get_param_vocab());

	ret.push_back(ParamDesc("layer",Param::TYPE_LAYER)
		.set_local_name(_("Layer"))
		.set_desc(_("Deprecated layer to upgrade"))
		.set_supports_multiple()
	);

	return ret;
}

bool
Action::LayerUpgrade::is_candidate(const ParamList& x)
{
	if(!candidate_check(get_param_vocab(),x))
		return false;

	// Candidate only when EVERY selected layer has a known upgrade target.
	std::pair<ParamList::const_iterator, ParamList::const_iterator> range(x.equal_range("layer"));
	if (range.first == range.second)
		return false;
	for (ParamList::const_iterator it(range.first); it != range.second; ++it) {
		if (it->second.get_type()!=Param::TYPE_LAYER)
			return false;
		const Layer::Handle layer(it->second.get_layer());
		if (!layer || get_layer_param(layer).empty())
			return false;
	}
	return true;
}

bool
Action::LayerUpgrade::set_param(const synfig::String& name, const Action::Param& param)
{
	if (name=="layer" && param.get_type() == Param::TYPE_LAYER) {
		layers.push_back(param.get_layer());
		return true;
	}

	return Action::CanvasSpecific::set_param(name, param);
}

bool
Action::LayerUpgrade::is_ready() const
{
	if (layers.empty())
		return false;
	return Action::CanvasSpecific::is_ready();
}

void
Action::LayerUpgrade::prepare()
{
	if (!first_time())
		return;

	if (layers.empty())
		throw Error(_("No layers to upgrade"));

	for (const Layer::Handle& layer : layers) {
		const String param_name(get_layer_param(layer));
		if (param_name.empty())
			continue; // shouldn't happen -- is_candidate already filtered

		Action::Handle action(Action::create("ValueDescSet"));
		action->set_param("canvas", get_canvas());
		action->set_param("canvas_interface", get_canvas_interface());

		action->set_param("value_desc", synfigapp::ValueDesc(layer, param_name));
		action->set_param("new_value", ValueBase(false));
		add_action(action);
	}
}
