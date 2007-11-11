/* --------------------------------------------------------------------------
 * haserl luascript library
 * $Id: haserl.c,v 1.32 2005/11/22 15:56:42 nangel Exp $
 * Copyright (c) 2003,2004    Nathan Angelacos (nangel@users.sourceforge.net)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 * --------------------------------------------------------------------------*/

const char *haserl_lualib =
    "-- The haserl table is reserved for haserl functions.  You are free to use\n"
    " haserl = {} \n"
    " FORM = {} \n"
    " ENV = {} \n"
    " function haserl.setfield ( f, v)\n"
    "    -- From programming in Lua 1st Ed.\n"
    "      local t = _G    -- start with the table of globals\n"
    "      for w, d in string.gfind(f, '([%w_]+)(.?)') do\n"
    "	   if ( tonumber(w) ) then w = tonumber(w) end\n"
    "        if d == '.' then      -- not last field?\n"
    "          t[w] = t[w] or {}   -- create table if absent\n"
    "          t = t[w]            -- get the table\n"
    "        else                  -- last field\n"
    "          t[w] = v            -- do the assignment\n"
    "        end\n"
    "      end\n"
    "    end\n"
    " function haserl.getfield (f)\n"
    "      local v = _G    -- start with the table of globals\n"
    "	for w in string.gfind(f, '[%w_]+') do\n"
    "        v = v[w]\n"
    "      end\n"
    "      return v\n"
    "    end\n"
    " function haserl.myputenv( key, value) \n"
    "	-- convert key to dotted form\n"
    "	key = string.gsub( key, '[\\]\\[]', '.' )\n"
    "	key = string.gsub( key, '[\\.]+', '.' )\n"
    "	-- and create a table if necessary\n"
    "	haserl.setfield (key, value)\n" "	end \n";
