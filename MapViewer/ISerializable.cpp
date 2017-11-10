#pragma once

#include "ISerializable.h"


SerializableFactory::TypeConstructorMap SerializableFactory::s_classes;
SerializableFactory::TypeNameMap SerializableFactory::s_nameMap;
