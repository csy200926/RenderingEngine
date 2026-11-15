#pragma once
#include <vector>
#include <algorithm>
namespace MyEngine
{
	template <class T> class TemplateArray
	{
	  public:
		int len = 0;
		void push(const T &i_value)
		{
			m_data.push_back(i_value);
			len = static_cast<int>(m_data.size());
		}
		void remove(const T &i_value)
		{
			auto it = std::find(m_data.begin(), m_data.end(), i_value);
			if (it != m_data.end())
				m_data.erase(it);
			len = static_cast<int>(m_data.size());
		}
		void clear()
		{
			m_data.clear();
			len = 0;
		}
		T &operator[](int i_index) { return m_data.at(i_index); }
		const T &operator[](int i_index) const { return m_data.at(i_index); }
		auto begin() { return m_data.begin(); }
		auto end() { return m_data.end(); }
		auto begin() const { return m_data.begin(); }
		auto end() const { return m_data.end(); }

	  private:
		std::vector<T> m_data;
	};
} // namespace MyEngine
