///////////////////////////////////////////////////////////////////////////////
// shadermanager.cpp
// ============
// manage the loading and rendering of 3D scenes
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
SceneManager::SceneManager(ShaderManager* pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();

	// initialize the texture collection
	for (int i = 0; i < 16; i++)
	{
		m_textureIDs[i].tag = "/0";
		m_textureIDs[i].ID = -1;
	}
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

	// destroy the created OpenGL textures
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

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// ensure image rows are read correctly for textures whose widths
		// are not naturally aligned to OpenGL's default 4-byte boundary
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

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
		glGenTextures(1, &m_textureIDs[i].ID);
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
			material.ambientColor = m_objectMaterials[index].ambientColor;
			material.ambientStrength = m_objectMaterials[index].ambientStrength;
			material.diffuseColor = m_objectMaterials[index].diffuseColor;
			material.specularColor = m_objectMaterials[index].specularColor;
			material.shininess = m_objectMaterials[index].shininess;
		}
		else
		{
			index++;
		}
	}

	return(true);
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

	modelView = translation * rotationX * rotationY * rotationZ * scale;

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
			m_pShaderManager->setVec3Value("material.ambientColor", material.ambientColor);
			m_pShaderManager->setFloatValue("material.ambientStrength", material.ambientStrength);
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

/***********************************************************
 *  LoadSceneTextures()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene
 *  rendering
 ***********************************************************/
void SceneManager::LoadSceneTextures()
{
	// load the ground texture for the grass plane
	CreateGLTexture("C:/CS330Content/Projects/7-1_FinalProjectMilestones/textures/grass.jpg", "groundTex");

	// load the bark texture for the firewood logs
	CreateGLTexture("C:/CS330Content/Projects/7-1_FinalProjectMilestones/textures/log-firepit.jpg", "logTex");

	// load the stone texture for the fire ring
	CreateGLTexture("C:/CS330Content/Projects/7-1_FinalProjectMilestones/textures/stone.jpg", "stoneTex");

	//load the sand texture for shoreline
	CreateGLTexture("C:/CS330Content/Projects/7-1_FinalProjectMilestones/textures/sand.jpg", "sand");

	// load the fabric texture for the chair cushions
	CreateGLTexture("C:/CS330Content/Projects/7-1_FinalProjectMilestones/textures/fabric.jpg", "fabricTex");

	// load the water texture for the river
	CreateGLTexture("C:/CS330Content/Projects/7-1_FinalProjectMilestones/textures/water.jpg", "waterTex");

	// load the plastic texture for the tent
	CreateGLTexture("C:/CS330Content/Projects/7-1_FinalProjectMilestones/textures/plastic.jpg", "plasticTex");

	// load the forest texture for the background
	CreateGLTexture("C:/CS330Content/Projects/7-1_FinalProjectMilestones/textures/forest.jpg", "forestTex");

	// after the texture image data is loaded into memory, the
	// loaded textures need to be bound to texture slots - there
	// are a total of 16 available slots for scene textures
	BindGLTextures();
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
	// load the textures for the 3D scene
	LoadSceneTextures();
	DefineObjectMaterials();
	SetupSceneLights();

	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene
	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadConeMesh();
	m_basicMeshes->LoadSphereMesh();
	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadTorusMesh();
}

/***********************************************************
 *  DefineObjectMaterials()
 *
 *  Define materials for lighting interaction with objects in scene
 ***********************************************************/
void SceneManager::DefineObjectMaterials()
{
	// ===============================
	// GROUND MATERIAL
	// darker values help the scene read more like night and keep the
	// grass from looking too bright far away from the fire
	// ===============================
	OBJECT_MATERIAL ground;
	ground.ambientStrength = 0.08f;
	ground.ambientColor = glm::vec3(0.24f, 0.22f, 0.16f);
	ground.diffuseColor = glm::vec3(0.36f, 0.32f, 0.22f);
	ground.specularColor = glm::vec3(0.05f, 0.05f, 0.05f);
	ground.shininess = 4.0f;
	ground.tag = "ground";
	m_objectMaterials.push_back(ground);

	// ===============================
	// LOG MATERIAL
	// kept darker and less reflective so the logs feel naturally lit
	// by the fire instead of glossy
	// ===============================
	OBJECT_MATERIAL log;
	log.ambientStrength = 0.12f;
	log.ambientColor = glm::vec3(0.34f, 0.22f, 0.10f);
	log.diffuseColor = glm::vec3(0.46f, 0.30f, 0.14f);
	log.specularColor = glm::vec3(0.04f, 0.04f, 0.04f);
	log.shininess = 3.0f;
	log.tag = "log";
	m_objectMaterials.push_back(log);

	// ===============================
	// STONE MATERIAL
	// slightly reflective so the stones can catch some fire highlight
	// without appearing overly shiny
	// ===============================
	OBJECT_MATERIAL stone;
	stone.ambientStrength = 0.12f;
	stone.ambientColor = glm::vec3(0.38f, 0.34f, 0.30f);
	stone.diffuseColor = glm::vec3(0.52f, 0.48f, 0.42f);
	stone.specularColor = glm::vec3(0.15f, 0.15f, 0.15f);
	stone.shininess = 10.0f;
	stone.tag = "stone";
	m_objectMaterials.push_back(stone);

	// ===============================
	// RIVER MATERIAL
	// slightly reflective so it reads as water in the background
	// ===============================
	OBJECT_MATERIAL river;
	river.ambientStrength = 0.10f;
	river.ambientColor = glm::vec3(0.08f, 0.12f, 0.16f);
	river.diffuseColor = glm::vec3(0.12f, 0.20f, 0.26f);
	river.specularColor = glm::vec3(0.30f, 0.30f, 0.30f);
	river.shininess = 18.0f;
	river.tag = "river";
	m_objectMaterials.push_back(river);

	// ===============================
	// FABRIC MATERIAL
	// soft and low-specular so it looks more like cloth instead of plastic in the light of the fire
	// ===============================
	OBJECT_MATERIAL fabric;
	fabric.ambientStrength = 0.14f;
	fabric.ambientColor = glm::vec3(0.22f, 0.18f, 0.12f);
	fabric.diffuseColor = glm::vec3(0.36f, 0.28f, 0.18f);
	fabric.specularColor = glm::vec3(0.03f, 0.03f, 0.03f);
	fabric.shininess = 2.0f;
	fabric.tag = "fabric";
	m_objectMaterials.push_back(fabric);

	// ===============================
	// PLASTIC MATERIAL
	// slightly shinier than fabric for tarp-like tent material
	// ===============================
	OBJECT_MATERIAL plastic;
	plastic.ambientStrength = 0.16f;
	plastic.ambientColor = glm::vec3(0.35f, 0.35f, 0.35f);
	plastic.diffuseColor = glm::vec3(0.75f, 0.75f, 0.75f);
	plastic.specularColor = glm::vec3(0.18f, 0.18f, 0.18f);
	plastic.shininess = 12.0f;
	plastic.tag = "plastic";
	m_objectMaterials.push_back(plastic);
}

/***********************************************************
 *  SetupSceneLights()
 *
 *  Configure lighting for campfire scene
 ***********************************************************/
void SceneManager::SetupSceneLights()
{
	// enable lighting in shader
	m_pShaderManager->setBoolValue(g_UseLightingName, true);

	// ============================================
	// LIGHT 0: FIRELIGHT (primary warm orange light)
	// ============================================
	m_pShaderManager->setVec3Value("lightSources[0].position", glm::vec3(0.0f, 0.3f, 0.0f));

	m_pShaderManager->setVec3Value("lightSources[0].ambientColor",
		glm::vec3(0.06f, 0.025f, 0.008f));

	m_pShaderManager->setVec3Value("lightSources[0].diffuseColor",
		glm::vec3(0.95f, 0.45f, 0.12f));

	m_pShaderManager->setVec3Value("lightSources[0].specularColor",
		glm::vec3(0.85f, 0.40f, 0.10f));

	m_pShaderManager->setFloatValue("lightSources[0].focalStrength", 10.0f);
	m_pShaderManager->setFloatValue("lightSources[0].specularIntensity", 0.22f);

	// ============================================
	// LIGHT 1: SOFT WARM FILL LIGHT
	// ============================================
	m_pShaderManager->setVec3Value("lightSources[1].position", glm::vec3(3.0f, 4.0f, 2.0f));

	m_pShaderManager->setVec3Value("lightSources[1].ambientColor",
		glm::vec3(0.015f, 0.010f, 0.006f));

	m_pShaderManager->setVec3Value("lightSources[1].diffuseColor",
		glm::vec3(0.12f, 0.08f, 0.04f));

	m_pShaderManager->setVec3Value("lightSources[1].specularColor",
		glm::vec3(0.10f, 0.07f, 0.04f));

	m_pShaderManager->setFloatValue("lightSources[1].focalStrength", 6.0f);
	m_pShaderManager->setFloatValue("lightSources[1].specularIntensity", 0.08f);
}

/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{
	// transformation variables
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	// turn lighting back ON for the rest of the scene
	m_pShaderManager->setIntValue("bUseLighting", true);

	// =========================================================
	// GROUND PLANE
	// - scaled and offset to serve as the base of the scene
	// - Extended backward to prevent visible edges in orthographic view
	// =========================================================
	scaleXYZ = glm::vec3(18.0f, 1.0f, 24.0f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(0.0f, -0.05f, -4.0f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

	// tile the grass texture across the ground plane to avoid stretching
	SetShaderTexture("groundTex");
	SetTextureUVScale(6.0f, 8.0f);
	SetShaderMaterial("ground");
	m_basicMeshes->DrawPlaneMesh();

	// =========================================================
	// FOREST BACKDROP - REAR
	// =========================================================
	scaleXYZ = glm::vec3(20.0f, 1.0f, 8.0f);
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(0.0f, 6.0f, -26.0f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("forestTex");
	SetTextureUVScale(2.0f, 1.0f);
	SetShaderMaterial("plastic");
	m_basicMeshes->DrawPlaneMesh();

	// =========================================================
	// FOREST BACKDROP - LEFT SIDE
	// =========================================================
	scaleXYZ = glm::vec3(20.0f, 1.0f, 8.0f);
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 90.0f;
	positionXYZ = glm::vec3(-18.0f, 6.0f, -4.0f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("forestTex");
	SetTextureUVScale(3.0f, 1.0f);
	SetShaderMaterial("plastic");
	m_basicMeshes->DrawPlaneMesh();

	// =========================================================
	// FOREST BACKDROP - RIGHT SIDE
	// =========================================================
	scaleXYZ = glm::vec3(20.0f, 1.0f, 8.0f);
	XrotationDegrees = 90.0f;
	YrotationDegrees = 00.0f;
	ZrotationDegrees = 90.0f;
	positionXYZ = glm::vec3(18.0f, 6.0f, -4.0f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("forestTex");
	SetTextureUVScale(3.0f, 1.0f);
	SetShaderMaterial("plastic");
	m_basicMeshes->DrawPlaneMesh();

	// =========================================================
	// FOREST BACKDROP - FRONT
	// =========================================================
	scaleXYZ = glm::vec3(20.0f, 1.0f, 8.0f);
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(0.0f, 6.0f, 14.0f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("forestTex");
	SetTextureUVScale(2.0f, 1.0f);
	SetShaderMaterial("plastic");
	m_basicMeshes->DrawPlaneMesh();

	//// =========================================================
	//// FOREST BACKDROP
	//// =========================================================
	//scaleXYZ = glm::vec3(20.0f, 1.0f, 8.0f);
	//XrotationDegrees = 90.0f;
	//YrotationDegrees = 0.0f;
	//ZrotationDegrees = 0.0f;
	//positionXYZ = glm::vec3(0.0f, 7.0f, -28.0f);

	//SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	//SetShaderTexture("forestTex");
	//SetTextureUVScale(1.0f, 1.0f);
	//SetShaderMaterial("plastic"); 
	//m_basicMeshes->DrawPlaneMesh();

	// =========================================================
	// Near Sandbar
	// =========================================================
	scaleXYZ = glm::vec3(18.0f, 1.0f, 1.5f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(0.0f, -0.01f, -6.0f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("sand");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawPlaneMesh();

	// =========================================================
	// River
	// =========================================================
	scaleXYZ = glm::vec3(18.0f, 1.0f, 10.0f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(0.0f, -0.03f, -14.5f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("waterTex");
	SetTextureUVScale(3.0f, 2.0f);
	SetShaderMaterial("river");
	m_basicMeshes->DrawPlaneMesh();

	// =========================================================
	// Far Sandbar
	// =========================================================
	scaleXYZ = glm::vec3(18.0f, 1.0f, 1.5f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(0.0f, -0.01f, -23.0f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("sand");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawPlaneMesh();

	// =========================================================
	// LOGS
	// =========================================================

	// Log 1 - leans left-forward across the fire pit
	scaleXYZ = glm::vec3(0.09f, 0.09f, 0.80f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 45.0f;
	ZrotationDegrees = 90.0f;
	positionXYZ = glm::vec3(-0.28f, 0.09f, 0.00f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

	// apply the wood texture to the logs with lighter tiling to reduce squishing
	SetShaderTexture("logTex");
	SetTextureUVScale(1.0f, 1.2f);
	SetShaderMaterial("log");
	m_basicMeshes->DrawCylinderMesh(true, true, true);

	// Log 2 - leans right-forward, crossing Log 1 at the center
	scaleXYZ = glm::vec3(0.09f, 0.09f, 0.80f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = -45.0f;
	ZrotationDegrees = 90.0f;
	positionXYZ = glm::vec3(0.28f, 0.09f, 0.00f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("logTex");
	SetTextureUVScale(1.0f, 1.2f);
	SetShaderMaterial("log");
	m_basicMeshes->DrawCylinderMesh(true, true, true);

	// Log 3 - rests on top of logs 1 & 2, runs front-to-back
	scaleXYZ = glm::vec3(0.09f, 0.09f, 0.80f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 90.0f;
	positionXYZ = glm::vec3(0.0f, 0.28f, 0.0f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("logTex");
	SetTextureUVScale(1.0f, 1.2f);
	SetShaderMaterial("log");
	m_basicMeshes->DrawCylinderMesh(true, true, true);

	// Log 4 - rests on top of log 3, angled slightly for a natural pile look
	scaleXYZ = glm::vec3(0.09f, 0.09f, 0.75f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 60.0f;
	ZrotationDegrees = 90.0f;
	positionXYZ = glm::vec3(0.0f, 0.37f, 0.0f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("logTex");
	SetTextureUVScale(1.0f, 1.2f);
	SetShaderMaterial("log");
	m_basicMeshes->DrawCylinderMesh(true, true, true);

	// =========================================================
	// FLAMES
	// =========================================================

	// Main center flame - tall, upright, deep orange
	scaleXYZ = glm::vec3(0.22f, 0.85f, 0.22f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 2.0f;
	positionXYZ = glm::vec3(0.00f, 0.20f, 0.00f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(1.0f, 0.40f, 0.05f, 1.0f);
	m_basicMeshes->DrawConeMesh(true);

	// Left flame - offset left, slightly shorter, leans left
	scaleXYZ = glm::vec3(0.16f, 0.60f, 0.16f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 10.0f;
	positionXYZ = glm::vec3(-0.18f, 0.28f, 0.05f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(1.0f, 0.55f, 0.08f, 1.0f);
	m_basicMeshes->DrawConeMesh(true);

	// Right flame - offset right, slightly shorter, leans right
	scaleXYZ = glm::vec3(0.16f, 0.58f, 0.16f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = -10.0f;
	positionXYZ = glm::vec3(0.18f, 0.20f, 0.05f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(1.0f, 0.60f, 0.10f, 1.0f);
	m_basicMeshes->DrawConeMesh(true);

	// Front flicker - leans slightly forward, bright yellow-white core
	scaleXYZ = glm::vec3(0.12f, 0.45f, 0.12f);
	XrotationDegrees = -8.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(0.05f, 0.20f, 0.18f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(1.0f, 0.85f, 0.20f, 1.0f);
	m_basicMeshes->DrawConeMesh(true);

	// Rear flicker - leans slightly back
	scaleXYZ = glm::vec3(0.11f, 0.40f, 0.11f);
	XrotationDegrees = 8.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = -4.0f;
	positionXYZ = glm::vec3(-0.05f, 0.18f, -0.18f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(1.0f, 0.50f, 0.07f, 1.0f);
	m_basicMeshes->DrawConeMesh(true);

	// =========================================================
	// STONE RING
	// =========================================================
	// apply the stone texture to each sphere in the fire ring
	SetShaderTexture("stoneTex");
	SetShaderMaterial("stone");
	SetTextureUVScale(0.3f, 0.3f);

	scaleXYZ = glm::vec3(0.42f, 0.18f, 0.30f);
	positionXYZ = glm::vec3(0.85f, 0.08f, 0.02f);
	SetTransformations(scaleXYZ, 0.0f, 12.0f, 0.0f, positionXYZ);
	m_basicMeshes->DrawSphereMesh();

	scaleXYZ = glm::vec3(0.36f, 0.17f, 0.26f);
	positionXYZ = glm::vec3(0.58f, 0.08f, 0.52f);
	SetTransformations(scaleXYZ, 0.0f, -18.0f, 0.0f, positionXYZ);
	m_basicMeshes->DrawSphereMesh();

	scaleXYZ = glm::vec3(0.40f, 0.19f, 0.28f);
	positionXYZ = glm::vec3(0.00f, 0.18f, 0.72f);
	SetTransformations(scaleXYZ, 0.0f, 8.0f, 0.0f, positionXYZ);
	m_basicMeshes->DrawSphereMesh();

	scaleXYZ = glm::vec3(0.34f, 0.14f, 0.24f);
	positionXYZ = glm::vec3(-0.56f, 0.08f, 0.48f);
	SetTransformations(scaleXYZ, 0.0f, 22.0f, 0.0f, positionXYZ);
	m_basicMeshes->DrawSphereMesh();

	scaleXYZ = glm::vec3(0.46f, 0.18f, 0.32f);
	positionXYZ = glm::vec3(-0.78f, 0.08f, 0.00f);
	SetTransformations(scaleXYZ, 0.0f, -10.0f, 0.0f, positionXYZ);
	m_basicMeshes->DrawSphereMesh();

	scaleXYZ = glm::vec3(0.35f, 0.15f, 0.25f);
	positionXYZ = glm::vec3(-0.52f, 0.08f, -0.50f);
	SetTransformations(scaleXYZ, 0.0f, 16.0f, 0.0f, positionXYZ);
	m_basicMeshes->DrawSphereMesh();

	scaleXYZ = glm::vec3(0.41f, 0.16f, 0.27f);
	positionXYZ = glm::vec3(0.00f, 0.08f, -0.72f);
	SetTransformations(scaleXYZ, 0.0f, -14.0f, 0.0f, positionXYZ);
	m_basicMeshes->DrawSphereMesh();

	scaleXYZ = glm::vec3(0.33f, 0.14f, 0.23f);
	positionXYZ = glm::vec3(0.56f, 0.08f, -0.46f);
	SetTransformations(scaleXYZ, 0.0f, 20.0f, 0.0f, positionXYZ);
	m_basicMeshes->DrawSphereMesh();


	// =========================================================
	// CAMP CHAIR 1 - spaced further from table
	// =========================================================

	// Stump seat
	scaleXYZ = glm::vec3(0.35f, 0.35f, 0.35f);
	positionXYZ = glm::vec3(-2.7f, 0.02f, 1.5f);

	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderTexture("logTex");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("log");
	m_basicMeshes->DrawCylinderMesh(true, true, true);

	// Seat cushion
	scaleXYZ = glm::vec3(0.30f, 0.05f, 0.30f);
	positionXYZ = glm::vec3(-2.7f, 0.36f, 1.5f);

	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderTexture("fabricTex");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("fabric");
	m_basicMeshes->DrawCylinderMesh(true, true, true);

	// Wooden chair back
	scaleXYZ = glm::vec3(0.10f, 0.45f, 0.32f);
	positionXYZ = glm::vec3(-2.88f, 0.72f, 1.60f);

	SetTransformations(scaleXYZ, 0.0f, 35.0f, 0.0f, positionXYZ);
	SetShaderTexture("logTex");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("log");
	m_basicMeshes->DrawBoxMesh();

	// Back cushion
	scaleXYZ = glm::vec3(0.08f, 0.30f, 0.22f);
	positionXYZ = glm::vec3(-2.84f, 0.72f, 1.56f);

	SetTransformations(scaleXYZ, 0.0f, 35.0f, 0.0f, positionXYZ);
	SetShaderTexture("fabricTex");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("fabric");
	m_basicMeshes->DrawBoxMesh();

	// Backrest support
	scaleXYZ = glm::vec3(0.05f, 0.65f, 0.05f);
	positionXYZ = glm::vec3(-2.95f, 0.32f, 1.67f);

	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderTexture("logTex");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("log");
	m_basicMeshes->DrawCylinderMesh(true, true, true);

	// =========================================================
	// CAMP CHAIR 2 - spaced further from table
	// =========================================================

	// Stump seat
	scaleXYZ = glm::vec3(0.35f, 0.35f, 0.35f);
	positionXYZ = glm::vec3(-1.40f, 0.02f, 2.73f);

	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderTexture("logTex");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("log");
	m_basicMeshes->DrawCylinderMesh(true, true, true);

	// Seat cushion
	scaleXYZ = glm::vec3(0.30f, 0.05f, 0.30f);
	positionXYZ = glm::vec3(-1.40f, 0.36f, 2.73f);

	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderTexture("fabricTex");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("fabric");
	m_basicMeshes->DrawCylinderMesh(true, true, true);

	// Wooden chair back
	scaleXYZ = glm::vec3(0.10f, 0.45f, 0.32f);
	positionXYZ = glm::vec3(-1.58f, 0.72f, 2.83f);

	SetTransformations(scaleXYZ, 0.0f, 35.0f, 0.0f, positionXYZ);
	SetShaderTexture("logTex");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("log");
	m_basicMeshes->DrawBoxMesh();

	// Back cushion
	scaleXYZ = glm::vec3(0.08f, 0.30f, 0.22f);
	positionXYZ = glm::vec3(-1.54f, 0.72f, 2.79f);

	SetTransformations(scaleXYZ, 0.0f, 35.0f, 0.0f, positionXYZ);
	SetShaderTexture("fabricTex");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("fabric");
	m_basicMeshes->DrawBoxMesh();

	// Backrest support
	scaleXYZ = glm::vec3(0.05f, 0.65f, 0.05f);
	positionXYZ = glm::vec3(-1.65f, 0.32f, 2.90f);

	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderTexture("logTex");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("log");
	m_basicMeshes->DrawCylinderMesh(true, true, true);

	// =========================================================
	// LOG TABLE - cut off log stump placed between the chairs
	// =========================================================

	// Table stump body
	scaleXYZ = glm::vec3(0.28f, 0.32f, 0.28f);
	positionXYZ = glm::vec3(-2.05f, 0.02f, 2.10f);

	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderTexture("logTex");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("log");
	m_basicMeshes->DrawCylinderMesh(true, true, true);

	// =========================================================
	// MUGS - two mugs sitting on top of the log table
	// =========================================================

	// Mug 1 body - left side of table top
	scaleXYZ = glm::vec3(0.07f, 0.10f, 0.07f);
	positionXYZ = glm::vec3(-2.14f, 0.34f, 2.06f);

	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderColor(0.45f, 0.28f, 0.18f, 1.0f);
	m_basicMeshes->DrawCylinderMesh(true, true, true);

	// Mug 1 handle
	scaleXYZ = glm::vec3(0.03f, 0.06f, 0.03f);
	positionXYZ = glm::vec3(-2.20f, 0.38f, 2.06f);

	SetTransformations(scaleXYZ, 0.0f, 0.0f, 90.0f, positionXYZ);
	SetShaderColor(0.40f, 0.24f, 0.14f, 1.0f);
	m_basicMeshes->DrawTorusMesh();

	// Mug 2 body - right side of table top
	scaleXYZ = glm::vec3(0.07f, 0.10f, 0.07f);
	positionXYZ = glm::vec3(-1.96f, 0.34f, 2.14f);

	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderColor(0.30f, 0.20f, 0.45f, 1.0f);
	m_basicMeshes->DrawCylinderMesh(true, true, true);

	// Mug 2 handle
	scaleXYZ = glm::vec3(0.03f, 0.06f, 0.03f);
	positionXYZ = glm::vec3(-1.90f, 0.38f, 2.14f);

	SetTransformations(scaleXYZ, 0.0f, 0.0f, 90.0f, positionXYZ);
	SetShaderColor(0.25f, 0.16f, 0.40f, 1.0f);
	m_basicMeshes->DrawTorusMesh();

	// =========================================================
	// TENT - simple A-frame tent near the shoreline
	// =========================================================

	glm::vec3 tentCenter = glm::vec3(2.15f, 0.0f, -2.65f);
	float tentYaw = -35.0f;
	float tentYawRad = glm::radians(tentYaw);

	// Helper to rotate a local offset around Y by tentYaw
	// ensures all tent pieces stay aligned to tent orientation
	auto rotY = [&](glm::vec3 offset) -> glm::vec3 {
		return glm::vec3(
			offset.x * cos(tentYawRad) + offset.z * sin(tentYawRad),
			offset.y,
			-offset.x * sin(tentYawRad) + offset.z * cos(tentYawRad)
		);
		};

	// ---------------------------------------------------------
	// Tent floor
	// ---------------------------------------------------------
	scaleXYZ = glm::vec3(1.8f, 0.03f, 2.2f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = tentYaw;
	ZrotationDegrees = 0.0f;
	positionXYZ = tentCenter + rotY(glm::vec3(0.0f, 0.02f, 0.0f));

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("plasticTex");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("plastic");
	m_basicMeshes->DrawBoxMesh();

	// ---------------------------------------------------------
	// Left tent side - tightened inward toward ridge pole
	// ---------------------------------------------------------
	scaleXYZ = glm::vec3(0.06f, 1.3f, 2.2f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = tentYaw;
	ZrotationDegrees = -40.0f;
	positionXYZ = tentCenter + rotY(glm::vec3(-0.42f, 0.52f, 0.0f));

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("plasticTex");
	SetTextureUVScale(1.0f, 1.5f);
	SetShaderMaterial("plastic");
	m_basicMeshes->DrawBoxMesh();

	// ---------------------------------------------------------
	// Right tent side - tightened inward toward ridge pole
	// ---------------------------------------------------------
	scaleXYZ = glm::vec3(0.06f, 1.3f, 2.2f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = tentYaw;
	ZrotationDegrees = 40.0f;
	positionXYZ = tentCenter + rotY(glm::vec3(0.42f, 0.52f, 0.0f));

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("plasticTex");
	SetTextureUVScale(1.0f, 1.5f);
	SetShaderMaterial("plastic");
	m_basicMeshes->DrawBoxMesh();

	// ---------------------------------------------------------
	// Tent back panel - lowered to match adjusted side height
	// ---------------------------------------------------------
	scaleXYZ = glm::vec3(1.1f, 1.1f, 0.05f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = tentYaw;
	ZrotationDegrees = 0.0f;
	positionXYZ = tentCenter + rotY(glm::vec3(0.0f, 0.52f, -1.09f));

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("plasticTex");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("plastic");
	m_basicMeshes->DrawBoxMesh();

	// ---------------------------------------------------------
	// Left front flap - scaled down, tightened to match sides
	// ---------------------------------------------------------
	scaleXYZ = glm::vec3(0.35f, 1.0f, 0.04f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = tentYaw;
	ZrotationDegrees = -40.0f;
	positionXYZ = tentCenter + rotY(glm::vec3(-0.42f, 0.52f, 1.09f));

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("plasticTex");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("plastic");
	m_basicMeshes->DrawBoxMesh();

	// ---------------------------------------------------------
	// Right front flap - scaled down, tightened to match sides
	// ---------------------------------------------------------
	scaleXYZ = glm::vec3(0.35f, 1.0f, 0.04f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = tentYaw;
	ZrotationDegrees = 40.0f;
	positionXYZ = tentCenter + rotY(glm::vec3(0.42f, 0.52f, 1.09f));

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("plasticTex");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("plastic");
	m_basicMeshes->DrawBoxMesh();

	// ---------------------------------------------------------
	// Tent ridge pole - lowered to sit at peak of adjusted sides
	// ---------------------------------------------------------
	scaleXYZ = glm::vec3(0.04f, 0.04f, 1.5f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = tentYaw;
	ZrotationDegrees = 0.0f;
	positionXYZ = tentCenter + rotY(glm::vec3(0.0f, 1.05f, 0.0f));

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("logTex");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("log");
	m_basicMeshes->DrawCylinderMesh(true, true, true);
}