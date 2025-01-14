#include "Globals.h"
#include "Application.h"
#include "ModuleImporter.h"
#include "ModuleScene.h"
#include "GameObject.h"
#include "ModuleFileSystem.h"
#include "PathNode.h"
#include "ResourceBase.h"
#include "ResourceMesh.h"
#include "ResourceTexture.h"

#include "c_Mesh.h"
#include "MeshData.h"
#include "c_Material.h"
#include "MaterialData.h"
#include "c_Transform.h"
#include "TransformData.h"
#include "model.h"

#include "Random.h"
#include <map>
#include <functional>
#include <vector>
#include <string>

#include <fstream> // Added this

#include "Assimp/include/mesh.h"
#include "Assimp/include/material.h"
#include "Assimp/include/texture.h"
#include "Assimp/include/matrix4x4.h"
#include "Assimp/include/vector3.h"
#include "Assimp/include/quaternion.h"

#include "Assimp/include/cimport.h"
#include "Assimp/include/scene.h"
#include "Assimp/include/postprocess.h"

#pragma comment (lib, "Assimp/libx86/assimp.lib")

//#include "Devil/include/il.h"			// not necessary, ilut.h includes it
#pragma comment( lib, "Devil/libx86/DevIL.lib" )
//#include "Devil/include/ilu.h"		// not necessary, ilut.h includes it
#pragma comment( lib, "Devil/libx86/ILU.lib" )
#include "Devil/include/ilut.h"
#pragma comment( lib, "Devil/libx86/ILUT.lib" )

ModuleImporter::ModuleImporter(Application* app, bool start_enabled) : Module(app, start_enabled)
{

}

ModuleImporter::~ModuleImporter()
{

}

bool ModuleImporter::Init()
{
	LOG("Init Module Imput");
	bool ret = true;
	
	ilInit();
	iluInit();
	ilutInit();
	ilutRenderer(ILUT_OPENGL);

	// Stream log messages to Debug window
	struct aiLogStream stream;
	stream = aiGetPredefinedLogStream(aiDefaultLogStream_DEBUGGER, nullptr);
	aiAttachLogStream(&stream);

	return ret;
}

bool ModuleImporter::Start()
{
	LOG("Importing scene test");

	return true;
}

update_status ModuleImporter::PreUpdate(float dt)
{
	update_status ret = UPDATE_CONTINUE;

	return ret;
}

bool ModuleImporter::CleanUp()
{
	LOG("CleanUp Module Importer");

	// detach log stream
	aiDetachAllLogStreams();

	return true;
}

/////////////////////////////////////////////////////////////////
//////////////MODEL IMPORTER (.FBX)/////////////////////////////

//Loads and imports the model into the engine but does not create it at the scene
 void ModuleImporter::LoadModel(const char* file_path)
{
	const aiScene* aiScene = aiImportFile(file_path, 0);
	if (aiScene != nullptr)
	{
		if (aiScene->mRootNode != nullptr)
		{
			ImportModelToLibrary(aiScene->mRootNode, aiScene, file_path);
		}
	}
}

void ModuleImporter::ImportModelToLibrary(aiNode* sceneRoot, const aiScene* currentScene, const char* fileName)
{
	if (sceneRoot->mNumChildren > 0)
	{
		for (int i = 0; i < sceneRoot->mNumChildren; i++)
		{
			NodeToResource(sceneRoot->mChildren[i], currentScene, fileName);
		}
	}
	else LOG("ERROR: Trying to load empty scene!");
}

void ModuleImporter::NodeToResource(aiNode* currentNode, const aiScene* currentScene, const char* filePath)
{
	if (currentNode->mNumMeshes > 0)
	{
		for (int i = 0; i < currentNode->mNumMeshes; i++)
		{
			uint currentAiMeshIndex = currentNode->mMeshes[i];
			aiMesh* currentAiMesh = currentScene->mMeshes[currentAiMeshIndex]; //find the correct mesh at the scene with the index given by mMeshes array


			TMYMODEL* myModel = new TMYMODEL();
			CreateAndSaveResourceMesh(currentAiMesh, myModel, filePath, currentNode->mName.C_Str());
			RELEASE(myModel);
		}
	}
	if (currentNode->mNumChildren > 0)
	{
		for (int i = 0; i < currentNode->mNumChildren; i++)
		{
			NodeToResource(currentNode->mChildren[i], currentScene, filePath);
		}
	} 
}

//Take the path to a .FBX, import it to our custom format and load it into scene
void ModuleImporter::LoadModelToScene(const char* file_path)
{
	// For each file, check if a .meta exists
	std::string path;
	std::string filename;
	App->fileSystem->SplitFilePath(file_path, &path, &filename);

	std::string fullName = path + filename + META_EXTENSION;

	// If the meta exists then load all related resources 
	if (App->fileSystem->Exists(fullName.c_str()))
	{
		int j = 0;

		std::string GOname;
		App->fileSystem->SplitFilePathInverse(fullName.c_str(), &GOname);
		GameObject* GO = App->scene->CreateGameObject(GOname.c_str(), App->scene->rootNode);
		//------- Root game object created ------//
		LOG("Trying to import file from assets that already has a meta. File name: %s", file_path);
		//read the meta file and get the resource id. Then, loop every resource in the corresponding meta files and add it as childs of the GO.
		while (App->fileSystem->Exists(fullName.c_str()))
		{
			char* buffer = nullptr;
			ParsonNode metaRoot = App->resources->LoadMetaFile(fullName.c_str(), &buffer);
			if (!metaRoot.NodeIsValid())
			{
				LOG("Error: Could not get the Meta Root Node! error path: %s", fullName.c_str());
				return;
			}

			uint32 resourceUid = (uint32)metaRoot.GetNumber("UID");
			uint32 materialUid = (uint32)metaRoot.GetNumber("materialUID");
			std::string textureName = metaRoot.GetString("texName");
			std::string name = metaRoot.GetString("Name");

			GameObject* childGO = App->scene->CreateGameObject(name.c_str(), GO);

			RELEASE_ARRAY(buffer);
			//------Meta file readed and 1st child created------//

			//-----Add component phase-----//			
			if (materialUid != 0)
			{
				std::string libPath = TEXTURES_PATH + std::to_string(materialUid);
				libPath += TEXTURES_EXTENSION;

				std::string assPath = ASSETS_TEXTURES_PATH + textureName;
				assPath += TEXTURES_EXTENSION;

				ResourceBase* rb = new ResourceBase(materialUid, assPath, libPath, ResourceType::TEXTURE);
				App->resources->library.emplace(materialUid, *rb);

				Resource* resTexture = App->resources->GetResource(materialUid);
				childGO->CreateComponent(resTexture); //create the componentTexture first
			}
			
			if (resourceUid != 0)
			{
				std::string libPath = MESHES_PATH + std::to_string(resourceUid);
				libPath += CUSTOM_FF_EXTENSION;

				std::string assPath = ASSETS_MODELS_PATH + name;
				assPath += ".fbx";

				ResourceBase* rb = new ResourceBase(resourceUid, assPath, libPath, ResourceType::MESH);
				App->resources->library.emplace(resourceUid, *rb);

				Resource* resMesh = App->resources->GetResource(resourceUid);
				childGO->CreateComponent(resMesh); //create the componentTexture first
			}

			j++;
			fullName = path + filename + std::to_string(j) + META_EXTENSION;
		}
	}
	
	//if there is no meta file for this fbx yet, import it unsing assimp.
	else
	{
		const aiScene* aiScene = aiImportFile(file_path, aiProcessPreset_TargetRealtime_MaxQuality);
		if (aiScene != nullptr)
		{
			if (aiScene->mRootNode != nullptr)
			{
				ImportModelToScene(aiScene->mRootNode, aiScene, file_path);
			}
		}
	}
}

void ModuleImporter::ImportModelToScene(aiNode* sceneRoot, const aiScene* currentScene, const char* fileName)
{
	if (sceneRoot->mNumChildren > 0)
	{
		std::string GOname;
		App->fileSystem->SplitFilePathInverse(fileName, &GOname);
		GameObject* GO = App->scene->CreateGameObject(GOname.c_str(), App->scene->rootNode);
		GOname.clear();

		for (int i = 0; i < sceneRoot->mNumChildren; i++)
		{
			LoadNode(GO, sceneRoot->mChildren[i], currentScene, fileName);
		}
	}
	else LOG("ERROR: Trying to load empty scene!");
}

void ModuleImporter::LoadNode(GameObject* parent, aiNode* currentNode, const aiScene* currentScene, const char* path) //STACK OVERFLOW?!
{
	GameObject* GO = App->scene->CreateGameObject(currentNode->mName.C_Str(), parent);

	{
		aiVector3D aiScale;
		aiQuaternion aiRotation;
		aiVector3D aiTranslation;
		currentNode->mTransformation.Decompose(aiScale, aiRotation, aiTranslation);							// --- Getting the Transform stored in the node.

		c_Transform* transform = GO->GetComponentTransform();

		transform->SetLocalPosition({ aiTranslation.x, aiTranslation.y, aiTranslation.z });
		transform->SetLocalRotation({ aiRotation.x, aiRotation.y, aiRotation.z, aiRotation.w });

		transform->SetLocalScale({ aiScale.x, aiScale.y, aiScale.z });
	}//-----Get transform-----/

	if (currentNode->mNumMeshes > 0)
	{
		for (int i = 0; i < currentNode->mNumMeshes; i++)
		{
			uint currentAiMeshIndex = currentNode->mMeshes[i];
			aiMesh* currentAiMesh = currentScene->mMeshes[currentAiMeshIndex]; //find the correct mesh at the scene with the index given by mMeshes array
			
			uint tempIndex = currentAiMesh->mMaterialIndex; //look for textures first. Import them if found
			if (tempIndex >= 0)
			{
				aiMaterial* currentMaterial = currentScene->mMaterials[currentAiMesh->mMaterialIndex]; //access the mesh material using the mMaterialIndex
				aiString texPath;
				if (currentMaterial->GetTexture(aiTextureType_DIFFUSE, 0, &texPath) == AI_SUCCESS)
				{
					std::string	fullPath = ASSETS_PATH;
					fullPath += texPath.C_Str();
					const char* filePath = fullPath.c_str();
					std::string fileName = {};
					App->fileSystem->SplitFilePath(filePath, &fullPath, &fileName);

					comodínGuapissimo = fileName;
					fullPath = fullPath + fileName + META_EXTENSION;
					filePath = {};

					if (App->fileSystem->Exists(fullPath.c_str()))
					{
						char* buffer = nullptr;
						ParsonNode metaRoot = App->resources->LoadMetaFile(fullPath.c_str(), &buffer); //load texture meta

						Resource* resTexture = App->resources->GetResource((uint32)metaRoot.GetNumber("UID")); //get the resourceTexture
						RELEASE_ARRAY(buffer);

						GO->CreateComponent(resTexture);
					}
					else
					{ //--------Save Texture into Lib-------//
						uint32 uuid = Random::GetRandomUint();
						std::string fullPath = ASSETS_TEXTURES_PATH + fileName + TEXTURES_EXTENSION; //Assets Path
						//create the resource texture and add it to library		
						fileName = TEXTURES_PATH + std::to_string(uuid) + TEXTURES_EXTENSION;		//Lib path
						App->fileSystem->DuplicateFile(fullPath.c_str(), fileName.c_str());
	
						ResourceBase* temp = new ResourceBase(uuid, fullPath, fileName, ResourceType::TEXTURE);
						App->resources->library.emplace(uuid, *temp);
						//write the meta file to connect the assets file with the resource in lib
						App->resources->SaveMetaFile(*temp, comodínGuapissimo.c_str(), uuid);
						
						Resource* texData = App->resources->GetResource(uuid);
						GO->CreateComponent(texData);
					}
					//tempMesh->texture = texData;	//TODO assign the texture to the current mesh
				}
			}
			TMYMODEL* myModel = new TMYMODEL();
			/*for (size_t i = 0; i < currentAiMesh->mNumVertices; i++)
			{
				myModel->textCoords[i] = 0.0f;
			}*/

			uint32 UID = 0;
			if(GO->GetComponentMaterial() != nullptr)
				UID = CreateAndSaveResourceMesh(currentAiMesh, myModel, currentNode->mName.C_Str(), path,  GO->GetComponentMaterial()->GetResourceUID());
			else
				UID = CreateAndSaveResourceMesh(currentAiMesh, myModel, currentNode->mName.C_Str(), path);

			RELEASE(myModel);

			Resource* myRes = App->resources->GetResource(UID);
			if (myRes == nullptr)
			{
				LOG("ERROR: ");
			}
			//App->scene->meshPool.push_back(tempMesh);

			GO->CreateComponent(myRes);
			
		}
	}
	if (currentNode->mNumChildren > 0)
	{
		for (int i = 0; i < currentNode->mNumChildren; i++)
		{
			LoadNode(GO, currentNode->mChildren[i], currentScene, path);
		}
	}
}


void ModuleImporter::ImportMesh(aiMesh* mesh, ResourceMesh* myMesh)
{
	// Copying number of vertices
	myMesh->vertexCount = mesh->mNumVertices;

	// Copying vertex positions
	myMesh->vPosData = new float[myMesh->vertexCount * 3];
	memcpy(myMesh->vPosData, mesh->mVertices, sizeof(float) * myMesh->vertexCount * 3);

	LOG("New mesh with %d vertices", myMesh->vertexCount);

	// Copying faces
	if (mesh->HasFaces())
	{
		myMesh->indicesCount = mesh->mNumFaces * 3;
		myMesh->indicesData = new uint[myMesh->indicesCount];

		for (uint i = 0; i < mesh->mNumFaces; i++)
		{
			if (mesh->mFaces[i].mNumIndices != 3)
			{
				LOG("Warning, geometry face with != indices!");
			}
			else
			{
				memcpy(&myMesh->indicesData[i * 3], mesh->mFaces[i].mIndices, 3 * sizeof(uint));
			}
		}
	}

	// Copying Texture coordinates
	if (mesh->mTextureCoords != NULL)
	{
		myMesh->vTexCoordsData = new float[myMesh->vertexCount * 2];

		for (uint i = 0; i < myMesh->vertexCount; i++)
		{
				myMesh->vTexCoordsData[i*2] = mesh->mTextureCoords[0][i].x;
				myMesh->vTexCoordsData[(i*2)+1] = mesh->mTextureCoords[0][i].y;
		}
	}
	else
	{
		LOG("Warning, No texture coordinates found");
	}
	App->renderer3D->PrepareMesh(myMesh);

	// Copying Normals
	if (mesh->HasNormals())
	{
		myMesh->vNormData = new float[myMesh->vertexCount * 3];

		for (uint i = 0; i < myMesh->vertexCount; i++)
		{
			memcpy(&myMesh->vNormData[(i * 3)], &mesh->mNormals[i].x, sizeof(float));
			memcpy(&myMesh->vNormData[(i * 3) + 1], &mesh->mNormals[i].y, sizeof(float));
			memcpy(&myMesh->vNormData[(i * 3) + 2], &mesh->mNormals[i].z, sizeof(float));
		}
	}
	else
	{
		LOG("Warning, no normals found");
	}
}

uint ModuleImporter::LoadTextureFromPath(const char* path, uint32 uuid)
{
	uint textureBuffer = 0;
	ILuint id_img = 0;
	ILenum error;

	if (path != nullptr)
	{
		std::string filePath = {};
		std::string	fileName = {};
		App->fileSystem->SplitFilePath(path, &filePath, &fileName);

		filePath = filePath + fileName + META_EXTENSION;
		fileName.clear();

		//if (App->fileSystem->Exists(filePath.c_str()))
		//{
		//	char* buffer = nullptr;
		//	ParsonNode metaRoot = App->resources->LoadMetaFile(filePath.c_str(), &buffer); //load texture meta

		//	uint32 resourceUid = (uint32)metaRoot.GetNumber("UID"); //get the resourceTexture UID
		//	RELEASE_ARRAY(buffer);

		//	ResourceTexture* resTexture = (ResourceTexture*)App->resources->GetResource(resourceUid); //get the resourceTexture

		//	return resTexture->GetTextureID();
		//}
		//else
		{
			ilGenImages(1, (ILuint*)&id_img);
			ilBindImage(id_img);

			error = ilGetError();

			if (error)
				LOG("ERROR: Failed generating/binding image: %s", iluErrorString(error));

			if (ilLoad(IL_TYPE_UNKNOWN, path))
			{
				ILinfo ImageInfo;
				iluGetImageInfo(&ImageInfo);
				if (ImageInfo.Origin == IL_ORIGIN_UPPER_LEFT)
					iluFlipImage();

				textureBuffer = ilutGLBindTexImage();
				error = ilGetError();

				if (error)
				{
					LOG("ERROR: Failed generating/binding image: %s", iluErrorString(error));
					textureBuffer = 0;
				}
			}
			else LOG("ERROR: Failed loading image: %s", iluErrorString(ilGetError()));

			ilDeleteImages(1, &id_img); // Because we have already copied image data into texture data we can release memory used by image.
		}
	}

	return textureBuffer;
}

void ModuleImporter::ImportToCustomFF(const char* libPath) //this is actually mesh to customff
{
	const aiScene* aiScene = aiImportFile(libPath, aiProcessPreset_TargetRealtime_MaxQuality);
	if (aiScene != nullptr)
	{
		if (aiScene->mRootNode != nullptr)
		{
			ImportModelToLibrary(aiScene->mRootNode, aiScene, libPath);
		}
	}

}

uint32 ModuleImporter::CreateAndSaveResourceMesh(const aiMesh* mesh, TMYMODEL* myModel, const char* name, const char* assetsPath, uint32 textureUID)
{
	//create custom file format inside TMYMODEL class
	myModel = CreateCustomMesh(mesh);

	//generate a UUID for the resource. It will also be the name of the file
	uint32 uuid = Random::GetRandomUint();
	std::string path = MESHES_PATH + std::to_string(uuid) + CUSTOM_FF_EXTENSION;

	//save the TMYMODEL into a file
	if (SaveCustomMeshFile(myModel, path.c_str()) == true)
	{
		LOG("%s Saved correctly", path);
	}
	else
	{
		LOG("%s Saved INCORRECTLY", path);
	}
	
	ResourceBase temp = *(new ResourceBase(uuid, assetsPath, path, ResourceType::MESH));

	App->resources->library.emplace(uuid, temp);

	App->resources->SaveMetaFile(temp, name, textureUID);
	
	return uuid;
}

TMYMODEL* ModuleImporter::CreateCustomMesh(const aiMesh* m)
{
	TMYMODEL* mymodel = (TMYMODEL*)malloc(sizeof(TMYMODEL));

	mymodel->verticesSizeBytes = m->mNumVertices * sizeof(float) * 3;//3==x,y,z
	mymodel->vertices = (float*)malloc(mymodel->verticesSizeBytes);
	memcpy(mymodel->vertices, m->mVertices, mymodel->verticesSizeBytes);

	mymodel->normalsSizeBytes = m->mNumVertices * sizeof(float) * 3;//3==x,y,z equal vertex
	mymodel->normals = (float*)malloc(mymodel->normalsSizeBytes);
	memcpy(mymodel->normals, m->mNormals, mymodel->normalsSizeBytes);

	mymodel->textCoordSizeBytes = m->mNumVertices * sizeof(float) * 2;//3==u,v
	mymodel->textCoords = (float*)malloc(mymodel->textCoordSizeBytes);
	if (m->HasTextureCoords(0))
	{
		for (int i = 0; i < m->mNumVertices; i++)
		{
			*(mymodel->textCoords + i * 2) = m->mTextureCoords[0][i].x;
			*(mymodel->textCoords + i * 2 + 1) = 1.0 - m->mTextureCoords[0][i].y; //this coord image is inverted
		}
	}
	mymodel->indiceSizeBytes = m->mNumFaces * sizeof(unsigned) * 3; //3==indices/face
	mymodel->indices = (unsigned*)malloc(mymodel->indiceSizeBytes);
	for (int i = 0; i < m->mNumFaces; i++)
	{
		aiFace* f = m->mFaces + i;
		*(mymodel->indices + 0 + i * 3) = f->mIndices[0];
		*(mymodel->indices + 1 + i * 3) = f->mIndices[1];
		*(mymodel->indices + 2 + i * 3) = f->mIndices[2];
	}

	return mymodel;
}

bool ModuleImporter::SaveCustomMeshFile(const TMYMODEL* m, const char* path)
{
	//My format
	//Header
	//a)unsigned for  verticesSizeBytes
	//b)unsigned for   normalsSizeBytes
	//c)unsigned for textCoordSizeBytes
	//d)unsigned for    indiceSizeBytes
	//e)unsigned for      infoSizeBytes
	//so the header size is 5 * sizeof(unsigned) -> 20 bytes

	std::ofstream myfile;
	myfile.open(path, std::ios::in | std::ios::app | std::ios::binary);
	if (myfile.is_open())
	{
		myfile.write((char*)m, 5 * sizeof(unsigned)); //write header

		myfile.write((char*)m->vertices, m->verticesSizeBytes);
		myfile.write((char*)m->normals, m->normalsSizeBytes);
	/*	if (m->textCoordSizeBytes != 0)
		{
			myfile.write((char*)m->textCoords, m->textCoordSizeBytes);
		}*/
		myfile.write((char*)m->textCoords, m->textCoordSizeBytes);
		myfile.write((char*)m->indices, m->indiceSizeBytes);
		//myfile.write((char*)m->info, m->infoSizeBytes); // xx

		myfile.close();

		return true;
	}
	else
	{
		return false;
	}

}

bool ModuleImporter::CustomMeshToScene(const char* path, ResourceMesh* mesh)
{
	std::ifstream myfile;
	myfile.open(path, std::ios::binary);

	if (myfile.is_open())
	{
		TMYMODEL* mymodel = (TMYMODEL*)malloc(sizeof(TMYMODEL));
		myfile.read((char*)mymodel, 5 * sizeof(unsigned)); //READ HEADER

		mymodel->vertices = (float*)malloc(mymodel->verticesSizeBytes);
		myfile.read((char*)mymodel->vertices, mymodel->verticesSizeBytes);

		mymodel->normals = (float*)malloc(mymodel->normalsSizeBytes);
		myfile.read((char*)mymodel->normals, mymodel->normalsSizeBytes);
		//if (mymodel->textCoordSizeBytes != 0)
		//{
		//	mymodel->textCoords = (float*)malloc(mymodel->textCoordSizeBytes);
		//	myfile.read((char*)mymodel->textCoords, mymodel->textCoordSizeBytes);
		//}
		mymodel->textCoords = (float*)malloc(mymodel->textCoordSizeBytes);
		myfile.read((char*)mymodel->textCoords, mymodel->textCoordSizeBytes);

		mymodel->indices = (unsigned*)malloc(mymodel->indiceSizeBytes);
		myfile.read((char*)mymodel->indices, mymodel->indiceSizeBytes);

		myfile.close();

		mesh->indicesData = mymodel->indices;
		mesh->vNormData = mymodel->normals;
		mesh->vTexCoordsData = mymodel->textCoords;
		mesh->vPosData = mymodel->vertices;

		mesh->vertexCount = (mymodel->verticesSizeBytes / (sizeof(float) * 3));
		mesh->indicesCount = (mymodel->indiceSizeBytes / sizeof(int));

		App->renderer3D->PrepareMesh(mesh);

		LOG("SUCCESS loading %s", path);
	}
	else
	{
		LOG("FAILED loading %s", path);
		return NULL;
	}
}

uint32 ModuleImporter::EmplaceTextureForParticle(const char* assPath)
{
	uint32 uuid = Random::GetRandomUint();
	std::string assetsPath = assPath;
	std::string fileName = TEXTURES_PATH + std::to_string(uuid) + TEXTURES_EXTENSION;
	App->fileSystem->DuplicateFile(assetsPath.c_str(), fileName.c_str());

	std::string name = {};
	App->fileSystem->SplitFilePathInverse(assPath, &name);

	ResourceBase* temp = new ResourceBase(uuid, assetsPath, fileName, ResourceType::TEXTURE);
	App->resources->library.emplace(uuid, *temp);

	App->resources->SaveMetaFile(*temp, name.c_str(), uuid);

	return uuid;
}

void ModuleImporter::Someting(uint32 uuid)
{
	
	Resource* texData = App->resources->GetResource(uuid);

	textureBuffer1 = GetTextureIDForParticle(texData->assetsPath.c_str(),uuid);
	
}


uint ModuleImporter::GetTextureIDForParticle(const char* path, uint32 uuid)
{
	uint textureBuffer = 0;
	ILuint id_img = 0;
	ILenum error;

	if (path != nullptr)
	{
		std::string filePath = {};
		std::string	fileName = {};
		App->fileSystem->SplitFilePath(path, &filePath, &fileName);

		filePath = filePath + fileName + META_EXTENSION;
		fileName.clear();

		//if (App->fileSystem->Exists(filePath.c_str()))
		//{
		//	char* buffer = nullptr;
		//	ParsonNode metaRoot = App->resources->LoadMetaFile(filePath.c_str(), &buffer); //load texture meta

		//	uint32 resourceUid = (uint32)metaRoot.GetNumber("UID"); //get the resourceTexture UID
		//	RELEASE_ARRAY(buffer);

		//	ResourceTexture* resTexture = (ResourceTexture*)App->resources->GetResource(resourceUid); //get the resourceTexture

		//	return resTexture->GetTextureID();
		//}
		//else
		{
			ilGenImages(1, (ILuint*)&id_img);
			ilBindImage(id_img);

			error = ilGetError();

			if (error)
				LOG("ERROR: Failed generating/binding image: %s", iluErrorString(error));

			if (ilLoad(IL_TYPE_UNKNOWN, path))
			{
				ILinfo ImageInfo;
				iluGetImageInfo(&ImageInfo);
				if (ImageInfo.Origin == IL_ORIGIN_UPPER_LEFT)
					iluFlipImage();

				textureBuffer = ilutGLBindTexImage();
				error = ilGetError();

				if (error)
				{
					LOG("ERROR: Failed generating/binding image: %s", iluErrorString(error));
					textureBuffer = 0;
				}
			}
			else LOG("ERROR: Failed loading image: %s", iluErrorString(ilGetError()));

			ilDeleteImages(1, &id_img); // Because we have already copied image data into texture data we can release memory used by image.
		}
	}

	return textureBuffer;
}

MeshData* ModuleImporter::CustomMeshToScene(const char* path)
{
	MeshData* loadedMesh = new MeshData();

	std::ifstream myfile;
	myfile.open(path, std::ios::binary);

	if (myfile.is_open())
	{
		TMYMODEL* mymodel = (TMYMODEL*)malloc(sizeof(TMYMODEL));
		myfile.read((char*)mymodel, 5 * sizeof(unsigned)); //READ HEADER

		mymodel->vertices = (float*)malloc(mymodel->verticesSizeBytes);
		myfile.read((char*)mymodel->vertices, mymodel->verticesSizeBytes);

		mymodel->normals = (float*)malloc(mymodel->normalsSizeBytes);
		myfile.read((char*)mymodel->normals, mymodel->normalsSizeBytes);

		mymodel->textCoords = (float*)malloc(mymodel->textCoordSizeBytes);
		myfile.read((char*)mymodel->textCoords, mymodel->textCoordSizeBytes);

		mymodel->indices = (unsigned*)malloc(mymodel->indiceSizeBytes);
		myfile.read((char*)mymodel->indices, mymodel->indiceSizeBytes);

		myfile.close();


		loadedMesh->indicesData = mymodel->indices;
		loadedMesh->vNormData = mymodel->normals;
		loadedMesh->vTexCoordsData = mymodel->textCoords;
		loadedMesh->vPosData = mymodel->vertices;

		loadedMesh->vertexCount = (mymodel->verticesSizeBytes / (sizeof(float) * 3));
		loadedMesh->indicesCount = (mymodel->indiceSizeBytes / sizeof(int));

		App->renderer3D->PrepareMesh(loadedMesh);

		LOG("SUCCESS loading %s", path);
		return loadedMesh;
	}
	else
	{
		LOG("FAILED loading %s", path);
		return NULL;
	}
}