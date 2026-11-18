#include <iostream>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

int main()
{
    Assimp::Importer importer;

    const aiScene* scene = importer.ReadFile(
        "assets/models/lamp.glb",
        aiProcess_Triangulate |
        aiProcess_GenSmoothNormals |
        aiProcess_JoinIdenticalVertices
    );

    if (!scene || !scene->mRootNode || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE))
    {
        std::cerr << "ERROR: " << importer.GetErrorString() << std::endl;
        return 1;
    }

    std::cout << "MODEL LOADED SUCCESSFULLY!\n";
    std::cout << "Meshes: " << scene->mNumMeshes << "\n";
    std::cout << "Materials: " << scene->mNumMaterials << "\n";

    for (unsigned int i = 0; i < scene->mNumMeshes; i++)
    {
        aiMesh* mesh = scene->mMeshes[i];

        std::cout << "\nMesh " << i << "\n";
        std::cout << "Vertices: " << mesh->mNumVertices << "\n";
        std::cout << "Faces: " << mesh->mNumFaces << "\n";
        std::cout << "Has normals: " << mesh->HasNormals() << "\n";
        std::cout << "Has texture coordinates: "
                  << mesh->HasTextureCoords(0) << "\n";
    }

    return 0;
}
