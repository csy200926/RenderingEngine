#include "MessageManager.h"
#include <algorithm>
namespace MyEngine
{
	bool MessageManager::RegisterMessageHandler(const HashedString &i_message, IMessageCallBack *i_pHandler)
	{
		if (!i_pHandler)
			return false;
		auto &handlers = m_handlers[i_message];
		for (auto &handler : handlers)
			if (handler.m_pHandler == i_pHandler)
				return false;
		handlers.push_back({i_pHandler, ++m_generation});
		return true;
	}
	bool MessageManager::UnregisterMessageHandler(const HashedString &i_message, IMessageCallBack *i_pHandler)
	{
		auto it = m_handlers.find(i_message);
		if (it == m_handlers.end())
			return false;
		auto &handlers = it->second;
		auto size = handlers.size();
		handlers.erase(std::remove_if(handlers.begin(), handlers.end(),
									  [&](const auto &handler) { return handler.m_pHandler == i_pHandler; }),
					   handlers.end());
		return size != handlers.size();
	}
	bool MessageManager::SendAMessage(const HashedString &i_message, Actor &i_actor, IIsA *i_pData)
	{
		auto it = m_handlers.find(i_message);
		if (it == m_handlers.end())
			return false;
		auto snapshot = it->second;
		bool delivered = false;
		for (auto handler : snapshot)
		{
			auto current = m_handlers.find(i_message);
			if (current == m_handlers.end())
				break;
			bool stillRegistered = std::any_of(current->second.begin(), current->second.end(), [&](const auto &value)
											   { return value.m_generation == handler.m_generation; });
			if (stillRegistered)
			{
				handler.m_pHandler->handleMessage(i_actor, i_pData);
				++m_deliveries;
				delivered = true;
			}
		}
		return delivered;
	}
} // namespace MyEngine
