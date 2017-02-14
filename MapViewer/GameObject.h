#pragma once
#include <string>
#include <vector>
namespace Rendering
{
	class GameObject
	{
	public:
		GameObject();
		~GameObject();

		std::string name;

		GameObject *parent;
		std::vector<GameObject *> children;

		void AddChild(GameObject *pChildObj)
		{
			children.push_back(pChildObj);
		};
		void RemoveChild(std::string childObjName)
		{
			for (size_t index = 0; index < children.size(); index++)
			{
				if (children[index]->name == childObjName)
				{
					children.erase(children.begin() + index);
					break;
				}

			}
		}

		virtual void Draw(){};
	};



}
