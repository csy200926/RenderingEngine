#pragma once
#include <vector>
#include <cstddef>
namespace MyEngine
{
	class MemoryPool
	{
	  public:
		static MemoryPool *create(size_t i_itemSize, size_t i_itemCount);
		~MemoryPool();
		void *alloc();
		void free(void *i_pPtr);
		bool Contains(void *i_pPtr) const;
		size_t GetLiveCount() const { return m_liveCount; }

	  private:
		MemoryPool(size_t i_itemSize, size_t i_itemCount);
		void Grow();
		size_t m_itemSize, m_itemCount, m_liveCount = 0;
		std::vector<void *> m_blocks;
		std::vector<void *> m_free;
	};
} // namespace MyEngine
