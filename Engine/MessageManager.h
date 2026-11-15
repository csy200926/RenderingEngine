#pragma once
#include "HashedString.h"
#include "IMessageCallBack.h"
#include "Rendering/ISingleton.h"
#include <map>
#include <vector>
namespace MyEngine
{
	class MessageManager : public Singleton<MessageManager>
	{
	  public:
		static MessageManager *SharedMessageManager() { return GetInstance(); }
		bool RegisterMessageHandler(const HashedString &i_message, IMessageCallBack *i_pHandler);
		bool UnregisterMessageHandler(const HashedString &i_message, IMessageCallBack *i_pHandler);
		bool SendAMessage(const HashedString &i_message, Actor &i_actor, IIsA *i_pData);
		unsigned GetDeliveryCount() const { return m_deliveries; }

	  private:
		struct Handler
		{
			IMessageCallBack *m_pHandler;
			size_t m_generation;
		};
		std::map<HashedString, std::vector<Handler>> m_handlers;
		size_t m_generation = 0;
		unsigned m_deliveries = 0;
	};
} // namespace MyEngine
