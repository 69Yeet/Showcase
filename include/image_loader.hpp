#ifndef IMAGE_LOADER_HPP
#define IMAGE_LOADER_HPP

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <ktx.h>
#include <string_view>
#include <vulkan/vulkan.h>

namespace iml
{
	ktxTexture* FetchImage(std::string_view imgPath)
	{
		int texWidth, texHeight, texChannels;
    	stbi_uc* pixels = stbi_load(imgPath.data(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

		ktxTexture2* texture;
		ktxTextureCreateInfo createInfo = 
		{
		    .vkFormat = VK_FORMAT_R8G8B8A8_SRGB,
		    .baseWidth = texWidth,
		    .baseHeight = texHeight,
		    .baseDepth = 1,
		    .numDimensions = 2,
		    .numLevels = 1,
		    .numLayers = 1,
		    .numFaces = 1,
		    .isArray = KTX_FALSE,
		    .generateMipmaps = KTX_FALSE
		};

		
	}
}

#endif
