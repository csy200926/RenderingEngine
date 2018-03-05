Scene = 
{
	MaterialManager = 
	{
		Default = 
		{
			Path = "Materials/Diffuse.material",
		},
	},
	MeshManager = 
	{
		Table = 
		{
			Path = "Models/sponza.obj",
		},
	},
	RootNode = 
	{
		Children = 
		{
			DirLight = 
			{
				Components = 
				{
					DirectionalLight = 
					{
						Constant = 0.20000000298023,
						Diffuse = 
						{
							x = 0,
							y = 1,
							z = 0,
						},
						Enable = true,
						Intensity = 1,
						Linear = 0.029999999329448,
						Quadratic = 0.0099999997764826,
						Specular = 
						{
							x = 1,
							y = 1,
							z = 1,
						},
						Type = 0,
					},
				},
				Orientation = 
				{
					w = 1,
					x = 0,
					y = 0,
					z = 0,
				},
				Position = 
				{
					x = 1,
					y = 0,
					z = 0,
				},
				Scale = 
				{
					x = 1,
					y = 1,
					z = 1,
				},
			},
		},
		Components = 
		{
			MeshRenderer = 
			{
				Materials = 
				{
					[0] = "Default",
				},
				Mesh = "Table",
			},
		},
		Orientation = 
		{
			w = 1,
			x = 0,
			y = 0,
			z = 0,
		},
		Position = 
		{
			x = 0,
			y = 0,
			z = 0,
		},
		Scale = 
		{
			x = 1,
			y = 1,
			z = 1,
		},
	},
	TextureManager = 
	{
		Wood = 
		{
			Path = "Images/defaultTex.jpg",
		},
		main = 
		{
			Path = "Images/terrain.png",
		},
		terrain = 
		{
			Path = "Images/terrain.png",
		},
	},
}



