
#line 1 "../source/if_truth_value.rl"
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


#line 33 "../source/if_truth_value.rl"


#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-const-variable"

#line 33 "source/if_truth_value.c"
static const int m_start = 1;
static const int m_first_final = 16;
static const int m_error = 0;

static const int m_en_main = 1;


#line 38 "../source/if_truth_value.rl"
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
	
#line 54 "source/if_truth_value.c"
	{
	cs = m_start;
	}

#line 54 "../source/if_truth_value.rl"
	
#line 57 "source/if_truth_value.c"
	{
	if ( p == pe )
		goto _test_eof;
	if ( cs == 0 )
		goto _out;
_resume:
	switch ( cs ) {
case 1:
	switch( (*p) ) {
		case 70: goto tr0;
		case 78: goto tr2;
		case 79: goto tr3;
		case 84: goto tr4;
		case 89: goto tr5;
		case 102: goto tr0;
		case 110: goto tr2;
		case 111: goto tr3;
		case 116: goto tr4;
		case 121: goto tr5;
	}
	goto tr1;
case 0:
	goto _out;
case 2:
	switch( (*p) ) {
		case 65: goto tr6;
		case 97: goto tr6;
	}
	goto tr1;
case 3:
	switch( (*p) ) {
		case 76: goto tr7;
		case 108: goto tr7;
	}
	goto tr1;
case 4:
	switch( (*p) ) {
		case 83: goto tr8;
		case 115: goto tr8;
	}
	goto tr1;
case 5:
	switch( (*p) ) {
		case 69: goto tr9;
		case 101: goto tr9;
	}
	goto tr1;
case 6:
	if ( (*p) == 0 )
		goto tr10;
	goto tr1;
case 16:
	goto tr1;
case 7:
	switch( (*p) ) {
		case 79: goto tr9;
		case 111: goto tr9;
	}
	goto tr1;
case 8:
	switch( (*p) ) {
		case 70: goto tr11;
		case 78: goto tr12;
		case 102: goto tr11;
		case 110: goto tr12;
	}
	goto tr1;
case 9:
	switch( (*p) ) {
		case 70: goto tr9;
		case 102: goto tr9;
	}
	goto tr1;
case 10:
	if ( (*p) == 0 )
		goto tr13;
	goto tr1;
case 11:
	switch( (*p) ) {
		case 82: goto tr14;
		case 114: goto tr14;
	}
	goto tr1;
case 12:
	switch( (*p) ) {
		case 85: goto tr15;
		case 117: goto tr15;
	}
	goto tr1;
case 13:
	switch( (*p) ) {
		case 69: goto tr12;
		case 101: goto tr12;
	}
	goto tr1;
case 14:
	switch( (*p) ) {
		case 69: goto tr16;
		case 101: goto tr16;
	}
	goto tr1;
case 15:
	switch( (*p) ) {
		case 83: goto tr12;
		case 115: goto tr12;
	}
	goto tr1;
	}

	tr1: cs = 0; goto _again;
	tr0: cs = 2; goto _again;
	tr6: cs = 3; goto _again;
	tr7: cs = 4; goto _again;
	tr8: cs = 5; goto _again;
	tr9: cs = 6; goto _again;
	tr2: cs = 7; goto _again;
	tr3: cs = 8; goto _again;
	tr11: cs = 9; goto _again;
	tr12: cs = 10; goto _again;
	tr4: cs = 11; goto _again;
	tr14: cs = 12; goto _again;
	tr15: cs = 13; goto _again;
	tr5: cs = 14; goto _again;
	tr16: cs = 15; goto _again;
	tr10: cs = 16; goto f0;
	tr13: cs = 16; goto f1;

f0:
#line 31 "../source/if_truth_value.rl"
	{ res = 0; }
	goto _again;
f1:
#line 32 "../source/if_truth_value.rl"
	{ res = 1; }
	goto _again;

_again:
	if ( cs == 0 )
		goto _out;
	if ( ++p != pe )
		goto _resume;
	_test_eof: {}
	_out: {}
	}

#line 55 "../source/if_truth_value.rl"
	return res;
}
