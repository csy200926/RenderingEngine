#pragma once
#include "Engine/Scene.h"
#include "Engine/IMessageCallBack.h"
#include "Engine/IActorController.h"
#include <memory>
class PlayerController;
namespace MyEngine
{
	struct EngineSettings;
}
class GameWorld : public MyEngine::Scene, public MyEngine::IMessageCallBack
{
  public:
	explicit GameWorld(const MyEngine::EngineSettings &i_settings);
	~GameWorld() override;
	void update(double i_delta) override;
	void handleMessage(MyEngine::Actor &i_actor, MyEngine::IIsA *i_pData) override;
	MyEngine::ActorPtr shootProjectileAt(const MyEngine::Vector3 &i_position);
	MyEngine::ActorPtr useSuperWeapon();

  private:
	struct HitCounter : MyEngine::IMessageCallBack
	{
		unsigned m_count = 0;
		void handleMessage(MyEngine::Actor &, MyEngine::IIsA *) override { ++m_count; }
	};
	bool m_rendering;
	float counter = 0, m_elapsed = 0;
	unsigned m_hits = 0;
	HitCounter m_hitCounter;
	std::unique_ptr<PlayerController> m_playerController;
	std::unique_ptr<MyEngine::IActorController> m_spinController;
	void CreateGame();
	void CreateRendering();
	void CreateModel(const MyEngine::EngineSettings &i_settings);
	void SpawnEnemy(float i_x);
};
