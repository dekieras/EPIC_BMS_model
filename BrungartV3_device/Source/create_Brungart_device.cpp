#include "EPICLib/Output_tee_globals.h"
#include "Brungart_device.h"


// for use in non-dynamically loaded models
Device_base * create_Brungart_device()
{
	return new Brungart_device("Brungart_device", Normal_out);
}

// the class factory functions to be accessed with dlsym
extern "C" Device_base * create_device() 
{
    return create_Brungart_device();
}

extern "C" void destroy_device(Device_base * p) 
{
    delete p;
}
