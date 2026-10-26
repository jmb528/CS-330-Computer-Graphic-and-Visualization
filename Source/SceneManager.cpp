///////////////////////////////////////////////////////////////////////////////
// scenemanager.cpp
// ============
// manage the preparing and rendering of 3D scenes - textures, materials, lighting
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/gtx/transform.hpp>

// declaration of global variables
namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";
}

/***********************************************************
 *  SceneManager()
 *
 *  The constructor for the class
 ***********************************************************/
SceneManager::SceneManager(ShaderManager *pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();
	m_loadedTextures = 0;
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;
	DestroyGLTextures();
}

/***********************************************************
 *  CreateGLTexture()
 *
 *  This method is used for loading textures from image files,
 *  configuring the texture mapping parameters in OpenGL,
 *  generating the mipmaps, and loading the read texture into
 *  the next available texture slot in memory.
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
	int width = 0;
	int height = 0;
	int colorChannels = 0;
	GLuint textureID = 0;

	// indicate to always flip images vertically when loaded
	stbi_set_flip_vertically_on_load(true);

	// try to parse the image data from the specified image file
	unsigned char* image = stbi_load(
		filename,
		&width,
		&height,
		&colorChannels,
		0);

	// if the image was successfully read from the image file
	if (image)
	{
		std::cout << "Successfully loaded image:" << filename << ", width:" << width << ", height:" << height << ", channels:" << colorChannels << std::endl;

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// if the loaded image is in RGB format
		if (colorChannels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		// if the loaded image is in RGBA format - it supports transparency
		else if (colorChannels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			std::cout << "Not implemented to handle image with " << colorChannels << " channels" << std::endl;
			return false;
		}

		// generate the texture mipmaps for mapping textures to lower resolutions
		glGenerateMipmap(GL_TEXTURE_2D);

		// free the image data from local memory
		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		// register the loaded texture and associate it with the special tag string
		m_textureIDs[m_loadedTextures].ID = textureID;
		m_textureIDs[m_loadedTextures].tag = tag;
		m_loadedTextures++;

		return true;
	}

	std::cout << "Could not load image:" << filename << std::endl;

	// Error loading the image
	return false;
}

/***********************************************************
 *  BindGLTextures()
 *
 *  This method is used for binding the loaded textures to
 *  OpenGL texture memory slots.  There are up to 16 slots.
 ***********************************************************/
void SceneManager::BindGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		// bind textures on corresponding texture units
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  DestroyGLTextures()
 *
 *  This method is used for freeing the memory in all the
 *  used texture memory slots.
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glDeleteTextures(1, &m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  FindTextureID()
 *
 *  This method is used for getting an ID for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureID(std::string tag)
{
	int textureID = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureID = m_textureIDs[index].ID;
			bFound = true;
		}
		else
			index++;
	}

	return(textureID);
}

/***********************************************************
 *  FindTextureSlot()
 *
 *  This method is used for getting a slot index for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureSlot(std::string tag)
{
	int textureSlot = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureSlot = index;
			bFound = true;
		}
		else
			index++;
	}

	return(textureSlot);
}

/***********************************************************
 *  FindMaterial()
 *
 *  This method is used for getting a material from the previously
 *  defined materials list that is associated with the passed in tag.
 ***********************************************************/
bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL& material)
{
	if (m_objectMaterials.size() == 0)
	{
		return(false);
	}

	int index = 0;
	bool bFound = false;
	while ((index < m_objectMaterials.size()) && (bFound == false))
	{
		if (m_objectMaterials[index].tag.compare(tag) == 0)
		{
			bFound = true;
			material.diffuseColor = m_objectMaterials[index].diffuseColor;
			material.specularColor = m_objectMaterials[index].specularColor;
			material.shininess = m_objectMaterials[index].shininess;
		}
		else
		{
			index++;
		}
	}

	return(bFound);
}

/***********************************************************
 *  SetTransformations()
 *
 *  This method is used for setting the transform buffer
 *  using the passed in transformation values.
 ***********************************************************/
void SceneManager::SetTransformations(
	glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ)
{
	// variables for this method
	glm::mat4 modelView;
	glm::mat4 scale;
	glm::mat4 rotationX;
	glm::mat4 rotationY;
	glm::mat4 rotationZ;
	glm::mat4 translation;

	// set the scale value in the transform buffer
	scale = glm::scale(scaleXYZ);
	// set the rotation values in the transform buffer
	rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
	rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
	// set the translation value in the transform buffer
	translation = glm::translate(positionXYZ);

	modelView = translation * rotationZ * rotationY * rotationX * scale;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
	}
}

/***********************************************************
 *  SetShaderColor()
 *
 *  This method is used for setting the passed in color
 *  into the shader for the next draw command
 ***********************************************************/
void SceneManager::SetShaderColor(
	float redColorValue,
	float greenColorValue,
	float blueColorValue,
	float alphaValue)
{
	// variables for this method
	glm::vec4 currentColor;

	currentColor.r = redColorValue;
	currentColor.g = greenColorValue;
	currentColor.b = blueColorValue;
	currentColor.a = alphaValue;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, false);
		m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
	}
}

/***********************************************************
 *  SetShaderTexture()
 *
 *  This method is used for setting the texture data
 *  associated with the passed in ID into the shader.
 ***********************************************************/
void SceneManager::SetShaderTexture(
	std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, true);

		int textureID = -1;
		textureID = FindTextureSlot(textureTag);
		m_pShaderManager->setSampler2DValue(g_TextureValueName, textureID);
	}
}

/***********************************************************
 *  SetTextureUVScale()
 *
 *  This method is used for setting the texture UV scale
 *  values into the shader.
 ***********************************************************/
void SceneManager::SetTextureUVScale(float u, float v)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
	}
}

/***********************************************************
 *  SetShaderMaterial()
 *
 *  This method is used for passing the material values
 *  into the shader.
 ***********************************************************/
void SceneManager::SetShaderMaterial(
	std::string materialTag)
{
	if (m_objectMaterials.size() > 0)
	{
		OBJECT_MATERIAL material;
		bool bReturn = false;

		bReturn = FindMaterial(materialTag, material);
		if (bReturn == true)
		{
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
}

/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/
void SceneManager::SetupSceneLights()
{

	// Enable lighting in the shader
	m_pShaderManager->setIntValue(g_UseLightingName, true);

		// ---------------------------------------------------------
		// Primary directional light
		// ---------------------------------------------------------
		m_pShaderManager->setIntValue(
			"directionalLight.bActive", true);

		m_pShaderManager->setVec3Value(
			"directionalLight.direction",
			glm::vec3(-0.5f, -1.0f, -0.3f));

		m_pShaderManager->setVec3Value(
			"directionalLight.ambient",
			glm::vec3(0.25f, 0.25f, 0.25f));

		m_pShaderManager->setVec3Value(
			"directionalLight.diffuse",
			glm::vec3(0.75f, 0.75f, 0.75f));

		m_pShaderManager->setVec3Value(
			"directionalLight.specular",
			glm::vec3(1.0f, 1.0f, 1.0f));

		// ---------------------------------------------------------
		// Secondary point light
		// ---------------------------------------------------------
		m_pShaderManager->setIntValue(
			"pointLights[0].bActive", true);

		m_pShaderManager->setVec3Value(
			"pointLights[0].position",
			glm::vec3(2.0f, 5.0f, 3.0f));

		m_pShaderManager->setVec3Value(
			"pointLights[0].ambient",
			glm::vec3(0.10f, 0.10f, 0.10f));

		m_pShaderManager->setVec3Value(
			"pointLights[0].diffuse",
			glm::vec3(0.45f, 0.45f, 0.40f));

		m_pShaderManager->setVec3Value(
			"pointLights[0].specular",
			glm::vec3(0.60f, 0.60f, 0.60f));

		// Spotlight is not needed for this scene
		m_pShaderManager->setBoolValue(
			"spotLight.bActive", false);

		// Default material values so lighting can reflect
		// from the textured objects
		m_pShaderManager->setVec3Value(
			"material.diffuseColor",
			glm::vec3(1.0f, 1.0f, 1.0f));

		m_pShaderManager->setVec3Value(
			"material.specularColor",
			glm::vec3(0.5f, 0.5f, 0.5f));

		m_pShaderManager->setFloatValue(
			"material.shininess",
			32.0f);
	}


/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene 
 *  rendering
 ***********************************************************/
void SceneManager::PrepareScene()
{
		SetupSceneLights();

		// Load texture images for the 3D scene
		CreateGLTexture("textures/wood.jpg", "wood");
		CreateGLTexture("textures/metal2.jpg", "metal");
		CreateGLTexture("textures/sand.jpg", "sand");
		CreateGLTexture("textures/fabric.jpg", "fabric");

		// Bind loaded textures to OpenGL texture slots
		BindGLTextures();

		// Load meshes used in the scene
		m_basicMeshes->LoadPlaneMesh();
		m_basicMeshes->LoadBoxMesh();
		m_basicMeshes->LoadCylinderMesh();
		m_basicMeshes->LoadConeMesh();
		m_basicMeshes->LoadSphereMesh();

		
}

/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by 
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{
    // Shared shape helpers keep scene layout readable and avoid repeated setup code.
    // Each shape is positioned in world coordinates on the desktop (y = 0).
    auto color = [&](float r, float g, float b) {
        SetShaderColor(r, g, b, 1.0f);
        SetTextureUVScale(1.0f, 1.0f);
    };
    auto textured = [&](const char* tag, float u = 1.0f, float v = 1.0f) {
        SetShaderTexture(tag);
        SetTextureUVScale(u, v);
    };
    auto box = [&](glm::vec3 size, glm::vec3 pos, float rx = 0, float ry = 0, float rz = 0) {
        SetTransformations(size, rx, ry, rz, pos);
        m_basicMeshes->DrawBoxMesh();
    };
    auto cylinder = [&](glm::vec3 size, glm::vec3 pos, float rx = 0, float ry = 0, float rz = 0) {
        SetTransformations(size, rx, ry, rz, pos);
        m_basicMeshes->DrawCylinderMesh();
    };
    auto sphere = [&](glm::vec3 size, glm::vec3 pos) {
        SetTransformations(size, 0, 0, 0, pos);
        m_basicMeshes->DrawSphereMesh();
    };

    // Wooden desktop. Two different object textures: wood and metal/fabric.
    textured("wood", 4.0f, 3.0f);
    SetTransformations(glm::vec3(12.0f, 1.0f, 7.0f), 0, 0, 0, glm::vec3(0));
    m_basicMeshes->DrawPlaneMesh();

    // LAPTOP, left side: body, inset keyboard, individual keys, hinged display.
    color(0.15f, 0.18f, 0.23f);
    box(glm::vec3(3.25f, 0.10f, 2.05f), glm::vec3(-2.4f, 0.10f, 0.70f));
    color(0.26f, 0.29f, 0.35f);
    box(glm::vec3(2.92f, 0.015f, 1.31f), glm::vec3(-2.4f, 0.16f, 0.47f));
    // One batch of small keys; keyboard sits toward the back of the laptop.
    color(0.07f, 0.08f, 0.10f);
    for (int row = 0; row < 4; ++row)
        for (int col = 0; col < 11; ++col)
            box(glm::vec3(0.20f, 0.016f, 0.19f),
                glm::vec3(-3.67f + col * 0.25f, 0.177f, -0.10f + row * 0.27f));
    color(0.30f, 0.34f, 0.40f);
    box(glm::vec3(0.76f, 0.008f, 0.39f), glm::vec3(-2.4f, 0.173f, 1.35f));
    // Open lid is slightly tilted backward, near rear of keyboard base.
    color(0.12f, 0.14f, 0.18f);
    box(glm::vec3(3.22f, 2.05f, 0.10f), glm::vec3(-2.4f, 1.14f, -0.45f), -12.0f);
    color(0.12f, 0.32f, 0.43f);
    box(glm::vec3(2.88f, 1.70f, 0.012f), glm::vec3(-2.4f, 1.15f, -0.374f), -12.0f);
    color(0.25f, 0.77f, 0.77f);
    box(glm::vec3(2.35f, 0.035f, 0.012f), glm::vec3(-2.4f, 1.10f, -0.353f), -12.0f);

    // DESK LAMP, at back right. Shade and articulated metal arm.
    textured("metal");
    cylinder(glm::vec3(0.62f, 0.12f, 0.62f), glm::vec3(2.95f, 0.06f, -1.80f));
    cylinder(glm::vec3(0.06f, 2.20f, 0.06f), glm::vec3(2.95f, 0.12f, -1.80f));
    sphere(glm::vec3(0.12f), glm::vec3(2.95f, 2.30f, -1.80f));
    // Cylinder rotation tilts arm toward the front-left.
    cylinder(glm::vec3(0.06f, 1.62f, 0.06f), glm::vec3(2.95f, 2.30f, -1.80f), 0, 0, 60.0f);
    sphere(glm::vec3(0.13f), glm::vec3(1.55f, 3.11f, -1.80f));
    textured("fabric");
    SetTransformations(glm::vec3(0.72f, 0.85f, 0.72f), 0, 0, -20.0f,
        glm::vec3(1.55f, 2.55f, -1.80f));
    m_basicMeshes->DrawConeMesh();

    // COFFEE MUG, in front-center. Recessed dark top suggests open cup.
    color(0.88f, 0.84f, 0.74f);
    cylinder(glm::vec3(0.44f, 0.76f, 0.44f), glm::vec3(0.60f, 0.02f, 1.25f));
    color(0.20f, 0.12f, 0.08f);
    cylinder(glm::vec3(0.37f, 0.012f, 0.37f), glm::vec3(0.60f, 0.785f, 1.25f));
    // Box handle approximates a sturdy open rectangular loop.
    color(0.88f, 0.84f, 0.74f);
    box(glm::vec3(0.38f, 0.10f, 0.13f), glm::vec3(1.18f, 0.62f, 1.25f));
    box(glm::vec3(0.13f, 0.41f, 0.13f), glm::vec3(1.32f, 0.41f, 1.25f));
    box(glm::vec3(0.38f, 0.10f, 0.13f), glm::vec3(1.18f, 0.23f, 1.25f));

    // COMPUTER MOUSE, right of mug. Flattened sphere and scroll wheel.
    color(0.19f, 0.21f, 0.24f);
    sphere(glm::vec3(0.36f, 0.17f, 0.55f), glm::vec3(2.34f, 0.18f, 1.30f));
    color(0.04f, 0.05f, 0.06f);
    box(glm::vec3(0.025f, 0.008f, 0.40f), glm::vec3(2.34f, 0.329f, 1.09f));
    color(0.52f, 0.54f, 0.58f);
    box(glm::vec3(0.08f, 0.025f, 0.12f), glm::vec3(2.34f, 0.35f, 1.02f));

    // NOTEBOOK on right: pages, fabric cover, contrasting binding strip.
    color(0.93f, 0.89f, 0.78f);
    box(glm::vec3(1.70f, 0.10f, 2.10f), glm::vec3(4.08f, 0.09f, 0.72f));
    textured("fabric");
    box(glm::vec3(1.88f, 0.06f, 2.25f), glm::vec3(4.08f, 0.15f, 0.72f));
    color(0.12f, 0.15f, 0.18f);
    box(glm::vec3(0.12f, 0.075f, 2.25f), glm::vec3(3.19f, 0.16f, 0.72f));
}
