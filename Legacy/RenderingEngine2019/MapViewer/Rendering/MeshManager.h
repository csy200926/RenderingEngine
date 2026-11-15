#pragma once
#include "IResourceManager.h"
#include "ISingleton.h"
#include "Mesh.h"

namespace Rendering
{
	class MeshManager : public IResourceManager, public Singleton < MeshManager >
	{
	public:
		virtual IResource *CreateImpl(const std::string& i_path)
		{
			Mesh *pMesh;
			pMesh = new Mesh(i_path);
			return pMesh;
		}

		MeshPtr Load(const std::string& i_name, const std::string& i_path)
		{
			return std::static_pointer_cast<Mesh>(IResourceManager::Load(i_name, i_path));
		}

		MeshPtr GetByName(const std::string & i_name)
		{
			return std::static_pointer_cast<Mesh>(IResourceManager::GetResourceByName(i_name));
		}


	};











}
