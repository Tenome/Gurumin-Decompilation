// UnusedEngineObject: the data-less class derived from Object (declared in object.h). It sits in its own file because
// Object's constructor and destructor live in another translation unit in the original (engine_base.cpp), so the
// compiler calls them here instead of expanding them.

#include "object.h"

// 0x10007df0: constructs the Object base and nothing else.
UnusedEngineObject::UnusedEngineObject()
{
}

// 0x10007e00: destroys the Object base and nothing else (tail call).
UnusedEngineObject::~UnusedEngineObject()
{
}
