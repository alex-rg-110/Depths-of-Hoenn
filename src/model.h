#ifndef MODEL_H
#define MODEL_H

#include <glad/glad.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <iostream>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

struct MeshVertex {
    glm::vec3 Position;
    glm::vec2 TexCoord;
    glm::vec3 Normal;
};

class Model {
public:
    GLuint VAO, VBO;
    int numVertices;
    GLuint textureID;
    bool hasTexture = false;
    std::string directory;

    Model(const char* path) {
        directory = std::string(path);
        directory = directory.substr(0, directory.find_last_of('/'));
        load(path);
    }

    void makeObject(Shader shader) {
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(MeshVertex), vertices.data(), GL_STATIC_DRAW);

        auto pos = glGetAttribLocation(shader.ID, "position");
        glEnableVertexAttribArray(pos);
        glVertexAttribPointer(pos, 3, GL_FLOAT, false, sizeof(MeshVertex), (void*)0);

        auto tex = glGetAttribLocation(shader.ID, "tex_coord");
        glEnableVertexAttribArray(tex);
        glVertexAttribPointer(tex, 2, GL_FLOAT, false, sizeof(MeshVertex), (void*)offsetof(MeshVertex, TexCoord));

        auto nor = glGetAttribLocation(shader.ID, "normal");
        glEnableVertexAttribArray(nor);
        glVertexAttribPointer(nor, 3, GL_FLOAT, false, sizeof(MeshVertex), (void*)offsetof(MeshVertex, Normal));

        glBindVertexArray(0);
    }

    void draw() {
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, numVertices);
    }

private:
    std::vector<MeshVertex> vertices;

    void load(const char* path) {
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenNormals);
        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
            std::cout << "Assimp error: " << importer.GetErrorString() << std::endl;
            return;
        }
        processNode(scene->mRootNode, scene);
        numVertices = vertices.size();
        std::cout << "Loaded model with " << numVertices << " vertices" << std::endl;

        // Load first texture found
        for (unsigned int i = 0; i < scene->mNumMaterials; i++) {
            aiMaterial* mat = scene->mMaterials[i];
            if (mat->GetTextureCount(aiTextureType_DIFFUSE) > 0) {
                aiString str;
                mat->GetTexture(aiTextureType_DIFFUSE, 0, &str);
                std::string texPath = directory + "/" + str.C_Str();
                loadTexture(texPath.c_str());
                break;
            }
        }
    }

    void processNode(aiNode* node, const aiScene* scene) {
        for (unsigned int i = 0; i < node->mNumMeshes; i++) {
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            processMesh(mesh, scene);
        }
        for (unsigned int i = 0; i < node->mNumChildren; i++) {
            processNode(node->mChildren[i], scene);
        }
    }

    void processMesh(aiMesh* mesh, const aiScene* scene) {
        for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
            aiFace face = mesh->mFaces[i];
            for (unsigned int j = 0; j < face.mNumIndices; j++) {
                unsigned int idx = face.mIndices[j];
                MeshVertex v;
                v.Position = glm::vec3(mesh->mVertices[idx].x, mesh->mVertices[idx].y, mesh->mVertices[idx].z);
                v.Normal = mesh->HasNormals() ? glm::vec3(mesh->mNormals[idx].x, mesh->mNormals[idx].y, mesh->mNormals[idx].z) : glm::vec3(0,1,0);
                v.TexCoord = mesh->mTextureCoords[0] ? glm::vec2(mesh->mTextureCoords[0][idx].x, mesh->mTextureCoords[0][idx].y) : glm::vec2(0,0);
                vertices.push_back(v);
            }
        }
    }

    void loadTexture(const char* path) {
        glGenTextures(1, &textureID);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        int w, h, ch;
        unsigned char* data = stbi_load(path, &w, &h, &ch, 0);
        if (data) {
            GLenum fmt = ch == 4 ? GL_RGBA : GL_RGB;
            glTexImage2D(GL_TEXTURE_2D, 0, fmt, w, h, 0, fmt, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);
            hasTexture = true;
            std::cout << "Loaded texture: " << path << std::endl;
        } else {
            std::cout << "Failed to load texture: " << path << std::endl;
        }
        stbi_image_free(data);
    }
};
#endif
