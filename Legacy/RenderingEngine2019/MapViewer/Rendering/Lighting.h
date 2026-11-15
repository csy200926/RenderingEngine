#pragma once
#include "INodeComponent.h"
#include <glm.hpp>


namespace Rendering
{
	enum LightType
	{
		DIRECTIONAL = 0,
		POINT,
		SPOT,
		NONE
	};

	class LightBase : public INodeComponent 
	{
	public:

		LightBase();


		virtual ~LightBase(){};
		
		glm::vec3 GetPosition()
		{ 
			return m_pNode->m_position;
		};

		void SetColor(glm::vec3 &i_color){ m_diffuse = i_color; }

		LightType GetType(){ return m_type; }
		bool IsActivated(){ return m_isActivated; }

		void Serialize(LuaPlus::LuaObject &luaObject)
		{
			using namespace LuaPlus;
			luaObject.SetInteger("Type", (int)m_type);
			luaObject.SetBoolean("Enable", m_isActivated);

			LuaObject diffuse_lua = luaObject.CreateTable("Diffuse");
			ISerializable::SerilizeVec3(diffuse_lua, m_diffuse);

			LuaObject specular_lua = luaObject.CreateTable("Specular");
			ISerializable::SerilizeVec3(specular_lua, m_specular);

			luaObject.SetNumber("Intensity", m_intensity);
			luaObject.SetNumber("Constant", m_constant);
			luaObject.SetNumber("Linear", m_linear);
			luaObject.SetNumber("Quadratic", m_quadratic);


		}

		void Deserialize(LuaPlus::LuaObject &luaObject)
		{
			m_constant = luaObject["Constant"].ToNumber();
			m_diffuse = ISerializable::DeserilizeVec3(luaObject["Diffuse"]);
			m_isActivated = luaObject["Enable"].ToString() == "true" ? true : false;
			m_intensity = luaObject["Intensity"].ToNumber();
			m_linear = luaObject["Linear"].ToNumber();
			m_quadratic = luaObject["Quadratic"].ToNumber();
			m_specular = ISerializable::DeserilizeVec3(luaObject["Specular"]);
			m_type = (LightType)luaObject["Type"].ToInteger();

		}
	protected:

		void AddToEngine();

		LightType m_type;

		//glm::vec3 m_ambient;
		glm::vec3 m_diffuse;
		glm::vec3 m_specular;

		float m_intensity;
		float m_constant;
		float m_linear;
		float m_quadratic;

	private:
		bool m_isActivated;

		friend class RenderingEngine;
		friend class RenderingEngineDeferred;
		friend class RenderingEnginePbr;
	};
	class DirectionalLight : public LightBase
	{
	public:
		DirectionalLight()
		{
			m_type = DIRECTIONAL;
			AddToEngine(); 
		}

		// TODO: Hack
		glm::vec3 GetDirection(){ return glm::vec3(-1.0f, -1.0f, -1.0f); }


	};
	class PointLight :public LightBase
	{
	public:
		PointLight()
		{
			m_type = POINT;
			AddToEngine();
		}

	};
	class SpotLight :public LightBase
	{
	public:
		SpotLight()
		{
			m_type = SPOT;
			AddToEngine();
		}
		float m_cutoff;

		void Serialize(LuaPlus::LuaObject &luaObject)
		{
			using namespace LuaPlus;
			LightBase::Serialize(luaObject);
			luaObject.SetNumber("Cutoff", m_cutoff);
		}

		void Deserialize(LuaPlus::LuaObject &luaObject)
		{
			using namespace LuaPlus;

		}
	};

}

