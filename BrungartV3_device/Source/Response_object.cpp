/*
 *  Response_object.cpp
 *  BrungartV0_device
 *
 *  Created by David Kieras on 4/27/10.
 *  Copyright 2010 __MyCompanyName__. All rights reserved.
 *
 */

#include "Response_object.h"
#include "EPICLib/Standard_symbols.h"
	
Response_object::Response_object(int id_, GU::Point loc_, const Symbol& color_, const Symbol& label_) :
	id(id_), location(loc_), color(color_), label(label_)
{	
}

