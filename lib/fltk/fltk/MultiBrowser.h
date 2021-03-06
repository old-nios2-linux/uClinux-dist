//
// "$Id: MultiBrowser.h 4886 2006-03-30 09:55:32Z fabien $"
//
// Browser that lets the user select more than one item at a time.
// Most of the implementation is in the base Browser class.
//
// Copyright 1998-2006 by Bill Spitzak and others.
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU Library General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
// USA.
//
// Please report all bugs and problems to "fltk-bugs@fltk.org".
//

#ifndef fltk_MultiBrowser_h
#define fltk_MultiBrowser_h

#include "Browser.h"

namespace fltk {

// This class is entirely inline.  If that changes, add FL_API to its declaration
class MultiBrowser : public Browser {
public:
    MultiBrowser(int x,int y,int w,int h,const char *l=0)
	: Browser(x,y,w,h,l) {type(MULTI);}
};

}

#endif

//
// End of "$Id: MultiBrowser.h 4886 2006-03-30 09:55:32Z fabien $".
//
