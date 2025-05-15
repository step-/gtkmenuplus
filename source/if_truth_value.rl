/*
gtkmenuplus - Generate a GTK popup menu from text directives.

Copyright (C) 2024-2025 step https://github.com/step-/gtkmenuplus

Licensed under the GNU General Public License Version 2

-------------------------------------------------------------------------

This program is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License, version 2, as published
by the Free Software Foundation.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc., 59
Temple Place, Suite 330, Boston, MA 02111-1307 USA
-------------------------------------------------------------------------
*/
/* *INDENT-OFF* */

#include <string.h>

%%{
	machine m;
	main :=
		( 'false'i | 'no'i | 'off'i ) 0 @{ res = 0; } |
		( 'true'i  | 'yes'i | 'on'i )   0 @{ res = 1; };
}%%

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-const-variable"
%% write data;
#pragma GCC diagnostic pop

/**
if_match_truth_value:
Rigel machine to match (false|off) or (true|on) case independently.

@p: the match subject; `p' is a reserved Rigel name.

Return: (1)false (0)true (-1)invalid
*/
int
if_match_truth_value (char *p)
{
	int cs, res = -1;
	char *pe = strchr(p, 0) + 1;
	%% write init;
	%% write exec;
	return res;
}
