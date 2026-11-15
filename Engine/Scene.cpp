#include "Scene.h"
#include "Collision.h"
#include "Platform/Paths.h"
#include <lua.hpp>
#include <cstring>
#include <memory>
namespace MyEngine
{
	Scene::~Scene()
	{
		for (auto &actor : Actors)
			actor->m_pScene = nullptr;
		for (auto &actor : m_pendingActors)
			actor->m_pScene = nullptr;
	}
	void Scene::addActor(const ActorPtr &i_actor)
	{
		if (!i_actor || i_actor->GetParent())
			throw std::logic_error("Scene requires an unattached Actor");
		if (i_actor->m_pScene == this)
			return;
		if (i_actor->m_pScene)
			throw std::logic_error("Actor already belongs to another Scene");
		i_actor->m_pScene = this;
		if (m_isUpdating)
			m_pendingActors.push(i_actor);
		else
			Actors.push(i_actor);
	}
	bool Scene::RemoveActor(ActorPtr i_actor)
	{
		// 绘制和游戏更新期间不改变所有权；编辑器在两帧之间调用这里。
		if (m_isUpdating || !i_actor || i_actor->m_pScene != this)
			return false;
		Actors.remove(i_actor);
		m_pendingActors.remove(i_actor);
		i_actor->m_pScene = nullptr;
		return true;
	}
	ActorPtr Scene::addActor(const char *i_texture, float i_width, float i_height, bool i_is3DObj)
	{
		ActorPtr actor(new Actor(i_texture, i_width, i_height, i_is3DObj));
		addActor(actor);
		return actor;
	}
	ActorPtr Scene::addTriggerBox(float i_x, float i_y, float i_z, bool i_is3DObj)
	{
		ActorPtr actor(new Actor(i_x, i_y, i_z, i_is3DObj));
		addActor(actor);
		return actor;
	}
	void Scene::FlushPendingActors()
	{
		for (auto &actor : m_pendingActors)
			Actors.push(actor);
		m_pendingActors.clear();
	}
	void Scene::InternalUpdate(double i_delta)
	{
		FlushPendingActors();
		m_isUpdating = true;
		update(i_delta);
		auto snapshot = Actors;
		for (auto &actor : snapshot)
		{
			if (actor->getState() != ActorState_Dead)
				actor->InternalUpdate(i_delta);
			actor->projectMove(i_delta);
		}
		Collision::GetInstance()->update(snapshot, i_delta);
		for (auto &actor : snapshot)
			actor->finalizeMove();
		m_isUpdating = false;
		releaseDeadActors();
		FlushPendingActors();
	}
	void Scene::releaseDeadActors()
	{
		if (m_isUpdating)
			return;
		for (int i = Actors.len - 1; i >= 0; --i)
		{
			ActorPtr actor = Actors[i];
			if (actor->getState() == ActorState_Dead)
			{
				actor->m_pScene = nullptr;
				Actors.remove(actor);
			}
		}
	}
	void Scene::Draw(Rendering::RenderingEngine &i_engine)
	{
		m_isUpdating = true;
		auto snapshot = Actors;
		for (auto &actor : snapshot)
			if (actor->getState() != ActorState_Dead)
				actor->Draw(i_engine);
		m_isUpdating = false;
		releaseDeadActors();
		FlushPendingActors();
	}
	ActorPtr Scene::FindActorByName(const char *i_name) const
	{
		auto hash = HashedString::Hash(i_name);
		for (auto &actor : Actors)
			if (actor->getName() == hash)
				return actor;
		return {};
	}
	TemplateArray<ActorPtr> Scene::FindAllActorsByClass(const char *i_type) const
	{
		TemplateArray<ActorPtr> result;
		for (auto &actor : Actors)
			if (std::strcmp(actor->getClassType(), i_type) == 0)
				result.push(actor);
		return result;
	}
	bool Scene::loadActorsFromConfig(const std::string &i_path)
	{
		std::unique_ptr<lua_State, decltype(&lua_close)> state(luaL_newstate(), lua_close);
		auto *pState = state.get();
		if (!pState)
			throw std::bad_alloc();
		auto text = Platform::ReadText(Platform::GetAssetPath(i_path));
		if (luaL_loadbuffer(pState, text.data(), text.size(), i_path.c_str()) || lua_pcall(pState, 0, 0, 0))
			throw std::runtime_error(lua_tostring(pState, -1));
		lua_getglobal(pState, "Actors");
		if (!lua_istable(pState, -1))
			return false;
		lua_pushnil(pState);
		while (lua_next(pState, -2))
		{
			if (lua_istable(pState, -1))
			{
				lua_getfield(pState, -1, "name");
				std::string name = lua_isstring(pState, -1) ? lua_tostring(pState, -1) : "Actor";
				lua_pop(pState, 1);
				lua_getfield(pState, -1, "class");
				std::string type = lua_isstring(pState, -1) ? lua_tostring(pState, -1) : "Actor";
				lua_pop(pState, 1);
				ActorPtr actor(new Actor(name));
				actor->setClassType(type.c_str());
				addActor(actor);
			}
			lua_pop(pState, 1);
		}
		return true;
	}
} // namespace MyEngine
