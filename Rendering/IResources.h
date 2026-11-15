#pragma once
#include <memory>
#include <string>
#include <atomic>
#include <cstdint>
namespace Rendering
{
	enum ResourceType
	{
		RT_Texture,
		RT_Mesh,
		RT_Shader,
		RT_Material
	};
	class IResource
	{
	  public:
		virtual ~IResource() = default;
		ResourceType GetType() const { return m_type; }
		const std::string &GetFileName() const { return m_fileName; }
		uint64_t GetHandle() const { return m_handle; }

	  protected:
		explicit IResource(ResourceType i_type) : m_type(i_type), m_handle(++s_nextHandle) {}
		ResourceType m_type;
		std::string m_name, m_fileName;

	  private:
		uint64_t m_handle;
		inline static std::atomic<uint64_t> s_nextHandle = 0;
	};
	using ResourcePtr = std::shared_ptr<IResource>;
} // namespace Rendering
