#pragma once

#include "LuaPlus.h"
#include <string>
#include <map>

template <class T>
void* MyConstructor()
{
	return (void*)(new T());
}
class SerializableFactory
{
public:
	typedef void*(*mycontructor)();
	typedef std::map<std::string, mycontructor> TypeMap;

	template<class T>
	static void Register_Class(std::string const& type_string)
	{
		if (m_classes.find(type_string) == m_classes.end())
		{
			m_classes.insert(std::make_pair(type_string, &MyConstructor<T>));
		}
	};

	static void* Create_Class(std::string const& type_string)
	{
		TypeMap::iterator it = m_classes.find(type_string);
		if (it == m_classes.end())return 0;
		return it->second();
	}
private:
	
	static TypeMap m_classes;
};

#define REGISTER_CLASS(n) SerializableFactory::Register_Class<n>(#n)
#define CREATE_CLASS(n) SerializableFactory::Create_Class(n)




class ISerializable {

public:

	virtual void Serialize(LuaPlus::LuaObject luaObject) = 0;
	virtual void Deserialize(LuaPlus::LuaObject luaObject) = 0;

};