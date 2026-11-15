#include "MemoryPool.h"
#include <new>
#include <cassert>
#include <cstdint>
namespace MyEngine
{
	MemoryPool::MemoryPool(size_t i_itemSize, size_t i_itemCount)
		: m_itemSize((i_itemSize + alignof(std::max_align_t) - 1) / alignof(std::max_align_t) *
					 alignof(std::max_align_t)),
		  m_itemCount(i_itemCount)
	{
	}
	MemoryPool *MemoryPool::create(size_t i_itemSize, size_t i_itemCount)
	{
		return new MemoryPool(i_itemSize, i_itemCount);
	}
	void MemoryPool::Grow()
	{
		auto *pBlock = static_cast<unsigned char *>(::operator new(m_itemSize * m_itemCount));
		m_blocks.push_back(pBlock);
		for (size_t i = 0; i < m_itemCount; ++i)
			m_free.push_back(pBlock + i * m_itemSize);
	}
	MemoryPool::~MemoryPool()
	{
		assert(m_liveCount == 0);
		for (auto pBlock : m_blocks)
			::operator delete(pBlock);
	}
	void *MemoryPool::alloc()
	{
		if (m_free.empty())
			Grow();
		auto pResult = m_free.back();
		m_free.pop_back();
		++m_liveCount;
		return pResult;
	}
	void MemoryPool::free(void *i_pPtr)
	{
		assert(Contains(i_pPtr) && m_liveCount);
		m_free.push_back(i_pPtr);
		--m_liveCount;
	}
	bool MemoryPool::Contains(void *i_pPtr) const
	{
		auto value = reinterpret_cast<uintptr_t>(i_pPtr);
		for (auto pBlock : m_blocks)
		{
			auto start = reinterpret_cast<uintptr_t>(pBlock);
			if (value >= start && value < start + m_itemSize * m_itemCount && (value - start) % m_itemSize == 0)
				return true;
		}
		return false;
	}
} // namespace MyEngine
