// Object: the empty engine base class (Game derives from it). Its constructor is 0x1013ed90 and its destructor the
// shared empty function 0x1013eda0 (engine_base.cpp).
#ifndef GURUMIN_OBJECT_H
#define GURUMIN_OBJECT_H

struct Object
{
    Object();
    ~Object();
};

// A class derived from Object that carries no data of its own. Its constructor (0x10007df0) calls Object::Object and
// its destructor (0x10007e00) tail-calls Object::~Object; nothing in the game constructs or destroys one, the linker
// kept both (probably the leftover of a removed feature).
struct UnusedEngineObject : Object
{
    UnusedEngineObject();
    ~UnusedEngineObject();
};

#endif
