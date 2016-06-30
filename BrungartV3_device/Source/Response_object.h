/*
 *  Response_object.h
 *  BrungartV0_device
 *
 *  Created by David Kieras on 4/27/10.
 *  Copyright 2010 __MyCompanyName__. All rights reserved.
 *
 */
#ifndef RESPONSE_OBJECT_H
#define RESPONSE_OBJECT_H

#include "EPICLib/Symbol.h"
#include "EPICLib/Geometry.h"
namespace GU = Geometry_Utilities;

namespace GU = Geometry_Utilities;
struct Response_object {
	// default ctor needed
	Response_object() : id(-1)
		{}
	// 
	Response_object(int id_, GU::Point loc_, const Symbol& color_, const Symbol& label_);

	Symbol name;
	int id;	// an ID number
	GU::Point location;
	Symbol color;
	Symbol label;
};

#endif
