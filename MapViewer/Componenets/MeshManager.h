#pragma once
#include "IResourceManager.h"
#include "ISingleton.h"
#include "Mesh.h"

namespace Rendering
{
	class MeshManager : public IResourceManager, public Singleton < MeshManager >
	{
	public:
		virtual IResource *CreateImpl(std::string& i_path)
		{
			Mesh *pMesh;
			pMesh = new Mesh(i_path);
			return pMesh;
		}





	};











}
