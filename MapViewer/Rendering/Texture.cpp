#include "Texture.h"
#include "../Utilities/stb_image.h"
namespace Rendering
{

	Texture::Texture()
	{
		m_type = RT_Texture;
		int debug = 0;
	}


	Texture::~Texture()
	{
		glDeleteTextures(1, &m_textureID);
		
	}

	bool Texture::LoadFromPath(const char *i_path)
	{

		int x, y, n;
		int force_channels = 4;
		unsigned char * image_data = stbi_load(i_path, &x, &y, &n, force_channels);

		int width_in_bytes = x * 4;
		unsigned char *top = NULL;
		unsigned char *bottom = NULL;
		unsigned char temp = 0;
		int half_height = y / 2;
		 
		for (int row = 0; row < half_height; row++) {
			top = image_data + row * width_in_bytes;
			bottom = image_data + (y - row - 1) * width_in_bytes;
			for (int col = 0; col < width_in_bytes; col++) {
				temp = *top;
				*top = *bottom;
				*bottom = temp;
				top++;
				bottom++;
			}
		}


		// Create one OpenGL texture
		glGenTextures(1, &m_textureID);

		// "Bind" the newly created texture : all future texture functions will modify this texture
		glBindTexture(GL_TEXTURE_2D, m_textureID);

		/* Note:
		You should be careful when specifying your textures in sRGB space as not all textures will actually be in sRGB space. 
		Textures used for coloring objects like diffuse textures are almost always in sRGB space. 
		Textures used for retrieving lighting parameters like specular maps and normal maps are almost always in linear space so if you were to configure these as sRGB textures as well, 
			the lighting will break down. 
		Be careful in which textures you specify as sRGB.
		*/

		// Monitor show things in SRGB.
		// Most textures are painted in SRGB space. So they look correct without Gamma correction.
		// Doing a renderer Gamma correction would make textures look way brighter than they ought to be.
		// So textures must be transfer back to linear space before renderer Gamma correction is applied.

		// Give the image to OpenGL use GL_SRGB_ALPHA if want it to be linear space
		glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB_ALPHA, x, y, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glGenerateMipmap(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, 0);

		delete[]image_data;

		return true;
	}
	bool Texture::ShowDown()
	{
		return true;
	}

	void Texture::Activate()
	{
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, m_textureID);
	}

	void Texture::Bind()
	{
		glBindTexture(GL_TEXTURE_2D, m_textureID);
	}



}
