#pragma once

#include "LuaPlus.h"

#include <string>
#include <map>
#include <typeindex>

#include <glm.hpp>
#include <gtx/quaternion.hpp>

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
	typedef std::map<std::string, std::type_index> NameTypeMap;

	static std::string GetStringByTypeIndex(std::type_index index)
	{
		TypeNameMap::iterator it = s_typeNameMap.find(index);
		if (it != s_typeNameMap.end())
		{
			return it->second;
		}
		return "";
	}

	static std::type_index GetTypeByString(std::string const&name)
	{
		NameTypeMap::iterator it = s_nameTypeMap.find(name);
		if (it != s_nameTypeMap.end())
		{
			return it->second;
		}
		std::type_index index(typeid(int));
		return index;
	}

	template<class T>
	static std::string GetStringByType()
	{
		std::type_index index(typeid(T));
		TypeConstructorMap::iterator it = s_classes.find(index);
		if (it != s_typeNameMap.end())
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
			s_typeNameMap.insert(std::make_pair(index, type_string));
			s_nameTypeMap.insert(std::make_pair(type_string,index));
		}

	
	};

	static void* Create_Class(std::string const& type_string)
	{
		TypeConstructorMap::iterator it = s_classes.find(type_string);
		if (it == s_classes.end())return 0;
		return it->second();
	}
	
private:
	static TypeNameMap s_typeNameMap;
	static TypeConstructorMap s_classes;
	static NameTypeMap s_nameTypeMap;
};



#define REGISTER_CLASS(n) SerializableFactory::Register_Class<n>(#n)

#define CREATE_CLASS(n) SerializableFactory::Create_Class(n)

class ISerializable {

public:

	virtual void Serialize(LuaPlus::LuaObject &luaObject) = 0;
	virtual void Deserialize(LuaPlus::LuaObject &luaObject) = 0;

	static glm::vec3 DeserilizeVec3(LuaPlus::LuaObject &luaObject)
	{
		float x = luaObject["x"].ToNumber();
		float y = luaObject["y"].ToNumber();
		float z = luaObject["z"].ToNumber();

		glm::vec3 result(x,y,z);
		return result;
	}

	static glm::quat DeserilizeQuat(LuaPlus::LuaObject &luaObject)
	{
		float x = luaObject["x"].ToNumber();
		float y = luaObject["y"].ToNumber();
		float z = luaObject["z"].ToNumber();
		float w = luaObject["w"].ToNumber();
		glm::quat result;
		result.x = x;
		result.y = y;
		result.z = z;
		result.w = w;


		return result;
	}


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