/* === S Y N F I G ========================================================= */
/*! \file actionmanager.h
**  \brief Header for a visual info database for GActions
**
**	\legal
**	Copyright (c) 2002-2005 Robert B. Quattlebaum Jr., Adrian Bentley
**	Copyright (c) 2022 Synfig Contributors
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
# include "pch.h"
#else
# ifdef HAVE_CONFIG_H
#  include <config.h>
# endif

# include "actionmanager.h"

# include <algorithm>
# include <set>

# include <glibmm/markup.h>

# include <gui/localization.h>
#endif

/* === U S I N G =========================================================== */

using namespace studio;

/* === M A C R O S ========================================================= */
/* === G L O B A L S ======================================================= */
/* === C L A S S E S ======================================================= */
/* === P R O C E D U R E S ================================================= */

/* === M E T H O D S ======================================================= */

ActionManager::Entry::Entry(const std::string& action_name, const std::string& label, const std::vector<Glib::ustring>& accelerators)
	: name_(action_name), label_(label), accelerators_(accelerators)
{
}

ActionManager::Entry::Entry(const std::string& action_name, const std::string& label, const std::string& accelerator)
	: name_(action_name), label_(label), accelerators_({accelerator})
{
}

ActionManager::Entry::Entry(const std::string& action_name, const std::string& label, const std::string& accelerator, const std::string& icon)
	: name_(action_name), label_(label), accelerators_({accelerator}), icon_(icon)
{
}

std::string
ActionManager::Entry::get_menu_item_string(Glib::RefPtr<Gtk::Application> app) const
{
	const bool has_accel = accelerators_.empty();
	const std::string accel = has_accel ? accelerators_.front() : "";

	std::string res;
	res += "<attribute name='label' translatable='yes'>" + label_ + "</attribute>";
	res += "<attribute name='action'>" + name_ + "</attribute>";

	if (has_accel)
		res += "<attribute name='accel'>" + Glib::Markup::escape_text(accel) + "</attribute>";

	if (!icon_.empty())
		res += "<attribute name='icon'>" + icon_ + "</attribute>";

	std::string tooltip = get_tooltip(app);
	if (!tooltip.empty())
		res += "<attribute name='tooltip'>" + tooltip + "</attribute>";
	return res;
}

std::string
ActionManager::Entry::get_tooltip(Glib::RefPtr<Gtk::Application> app) const
{
	std::string l10n_domain = l10n_domain_.empty() ? GETTEXT_PACKAGE : l10n_domain_;

	const auto app_accels = app ? app->get_accels_for_action(name_) : std::vector<Glib::ustring>();
	const bool has_accel = app ? !app_accels.empty() : !accelerators_.empty();
	const std::string accel = !has_accel ? "" : (app ? app_accels.front() : accelerators_.front());

	std::string tooltip = dgettext(l10n_domain.c_str(), (tooltip_.empty() ? label_ : tooltip_).c_str());

	if (has_accel && !accel.empty()) {
		Gtk::AccelKey accel_key(accel);
		tooltip += "  " + Gtk::AccelGroup::get_label(accel_key.get_key(), accel_key.get_mod());
	}
	return tooltip;
}

std::string
ActionManager::Entry::get_actual_group() const
{
	return !group_.empty() ? group_ : name_.substr(0, name_.find('.'));
}

ActionManager::ActionManager(Glib::RefPtr<Gtk::Application> app)
	: app_(app)
{
}

const ActionManager::Entry&
ActionManager::get(const std::string& name) const
{
	return book_.at(name);
}

void
ActionManager::add(const Entry& entry)
{
	book_[entry.name_] = entry;
}

std::vector<std::string>
ActionManager::get_groups() const
{
	std::vector<std::string> groups;
	std::set<std::string> group_set;
	for (const auto& item : book_) {
		const Entry& entry = item.second;
		group_set.insert(entry.get_actual_group());
	}
	groups.assign(group_set.begin(), group_set.end());
	return groups;
}

ActionManager::EntryList
ActionManager::get_entries() const
{
	EntryList v;
	std::transform(book_.begin(), book_.end(),
					std::back_inserter(v),
					[](const std::pair<std::string, Entry> &p) {
						return p.second;
					});
	return v;
}

ActionManager::EntryList
ActionManager::get_entries_for_group(const std::string& group) const
{
	EntryList entries;
	for (auto item : book_) {
		if (item.second.get_actual_group() == group)
			entries.push_back(item.second);
	}
	return entries;
}
