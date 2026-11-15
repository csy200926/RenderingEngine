#pragma once

#include "ISerializable.h"


SerializableFactory::TypeConstructorMap SerializableFactory::s_classes;
SerializableFactory::TypeNameMap SerializableFactory::s_typeNameMap;
SerializableFactory::NameTypeMap SerializableFactory::s_nameTypeMap;
