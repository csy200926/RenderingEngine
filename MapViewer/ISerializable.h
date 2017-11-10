#pragma once

#include "LuaPlus.h"

#include <string>
#include <map>
#include <typeindex>

#include "glm.hpp"

template <class T>
void* MyConstructor()
{
	return (void*)(new T());
}
class SerializableFactory
{
public:
	typedef void*(*mycontructor)();
	typedef std::map<std::string, mycontructor> TypeConstructorMap;
	typedef std::map<std::type_index, std::string> TypeNameMap;

	static std::string GetStringByTypeIndex(std::type_index index)
	{
		TypeNameMap::iterator it = s_nameMap.find(index);
		if (it != s_nameMap.end())
		{
			return it->second;
		}
		return "";
	}

	template<class T>
	static std::string GetStringByType()
	{
		std::type_index index(typeid(T));
		TypeConstructorMap::iterator it = s_classes.find(index);
		if (it != s_nameMap.end())
		{
			return it->second;
		}
		return "";
	}

	template<class T>
	static void Register_Class(std::string const& type_string)
	{
		if (s_classes.find(type_string) == s_classes.end())
		{
			s_classes.insert(std::make_pair(type_string, &MyConstructor<T>));

			std::type_index index(typeid(T));
			s_nameMap.insert(std::make_pair(index, type_string));
		}
	};

	static void* Create_Class(std::string const& type_string)
	{
		TypeConstructorMap::iterator it = s_classes.find(type_string);
		if (it == s_classes.end())return 0;
		return it->second();
	}
	
private:
	static TypeNameMap s_nameMap;
	static TypeConstructorMap s_classes;
};



#define REGISTER_CLASS(n) SerializableFactory::Register_Class<n>(#n)

#define CREATE_CLASS(n) SerializableFactory::Create_Class(n)

class ISerializable {

public:

	virtual void Serialize(LuaPlus::LuaObject &luaObject) = 0;
	virtual void Deserialize(LuaPlus::LuaObject &luaObject) = 0;

	static void SerilizeVec3(LuaPlus::LuaObject &luaObject, glm::vec3 &vec3)
	{
		luaObject.SetNumber("x", vec3.x);
		luaObject.SetNumber("y", vec3.y);
		luaObject.SetNumber("z", vec3.z);
	}

	static void SerilizeQuat(LuaPlus::LuaObject &luaObject, glm::quat &quat)
	{
		luaObject.SetNumber("x", quat.x);
		luaObject.SetNumber("y", quat.y);
		luaObject.SetNumber("z", quat.z);
		luaObject.SetNumber("w", quat.w);
	}
};