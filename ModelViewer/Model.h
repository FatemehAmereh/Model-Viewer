#ifndef MODEL_H
#define MODEL_H

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "Shader.h"
#include "cyTriMesh.h"
#include <glad/glad.h> 

class Model {
private:
	cyTriMesh ctm;
    unsigned int materialCount;
    std::vector<GLuint*> textures;
    Shader shader;
    GLuint VAO, VBO;

    //transformation
    float yaw = 0;
    float pitch = 0;
    float cameraDistance = 0;
    glm::vec4 lightPos;
    glm::mat4 projection = glm::mat4(1.0f);
    glm::mat4 view = glm::mat4(1.0f);

	Model();
public:
	Model(cyTriMesh ctm, Shader shader, glm::mat4 view, glm::mat4 projection): ctm(ctm), 
        materialCount(0), shader(shader), view(view), projection(projection){
        stbi_set_flip_vertically_on_load(true);

        #pragma region Vertex
        ctm.ComputeBoundingBox();
        cyVec3f boundMiddle = (ctm.GetBoundMax() + ctm.GetBoundMin()) / 2.0f;
        float ModelSize = (ctm.GetBoundMax() - ctm.GetBoundMin()).Length() / 20.0f;

        std::vector<float> vertices;
        for (size_t i = 0; i < ctm.NF(); i++)
        {
            for (size_t j = 0; j < 3; j++)
            {
                //vertices
                vertices.push_back((ctm.V(ctm.F(i).v[j]).x - boundMiddle.x) / ModelSize);
                vertices.push_back((ctm.V(ctm.F(i).v[j]).y - boundMiddle.y) / ModelSize);
                vertices.push_back((ctm.V(ctm.F(i).v[j]).z - boundMiddle.z) / ModelSize);
                //normals
                vertices.push_back(ctm.VN(ctm.FN(i).v[j]).x);
                vertices.push_back(ctm.VN(ctm.FN(i).v[j]).y);
                vertices.push_back(ctm.VN(ctm.FN(i).v[j]).z);
                //texture coordinates
                vertices.push_back(ctm.VT(ctm.FT(i).v[j]).x);
                vertices.push_back(ctm.VT(ctm.FT(i).v[j]).y);
            }
        }

        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(2);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
        #pragma endregion

        #pragma region Materials & Textures
        materialCount = ctm.NM();
        for (size_t i = 0; i < materialCount; i++)
        {
            GLuint* textureTmp = new GLuint[5];
            ManageTextures(textureTmp, ctm, i);
            textures.push_back(textureTmp);
        }
        shader.use();
        shader.setInt("ambientTexture", 0);
        shader.setInt("diffuseTexture", 1);
        shader.setInt("specularTexture", 2);
        shader.setInt("specularExponentTexture", 3);
        shader.setInt("alphaTexture", 4);
        #pragma endregion  
	}

    void Draw() {

        glm::mat4 modelMatrix = glm::mat4(1.0f);
        modelMatrix = glm::translate(modelMatrix, glm::vec3(0, 0, -cameraDistance));
        modelMatrix = glm::rotate(modelMatrix, glm::radians(pitch), glm::vec3(1, 0, 0));
        modelMatrix = glm::rotate(modelMatrix, glm::radians(yaw), glm::vec3(0, 1, 0));
        modelMatrix = glm::rotate(modelMatrix, glm::radians(-90.0f), glm::vec3(1, 0, 0));

        shader.use();
        shader.setMat4("mvp", projection * view * modelMatrix);
        shader.setMat4("mv", view * modelMatrix);
        shader.setMat4("mvN", glm::transpose(glm::inverse(view * modelMatrix)));   //mv for Normals
        shader.setVec3("lightPosition", lightPos);

        for (size_t i = 0; i < materialCount; i++)
        {
            GLuint* t = textures[i];
            shader.setBool("usetexa", true);
            shader.setBool("usetexd", true);
            shader.setBool("usetexs", true);
            shader.setBool("usetexsc", true);

            if (!ctm.M(i).map_Ka) {
                shader.setVec3("ka", glm::vec3(ctm.M(i).Ka[0], ctm.M(i).Ka[1], ctm.M(i).Ka[2]));
                shader.setBool("usetexa", false);
            }
            if (!ctm.M(i).map_Kd) {
                shader.setVec3("kd", glm::vec3(ctm.M(i).Kd[0], ctm.M(i).Kd[1], ctm.M(i).Kd[2]));
                shader.setBool("usetexd", false);
            }
            if (!ctm.M(i).map_Ks) {
                shader.setVec3("ks", glm::vec3(ctm.M(i).Ks[0], ctm.M(i).Ks[1], ctm.M(i).Ks[2]));
                shader.setBool("usetexs", false);
            }
            if (!ctm.M(i).map_Ns) {
                shader.setFloat("specularExponent", ctm.M(i).Ns);
                shader.setBool("usetexsc", false);
            }

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, t[0]);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, t[1]);
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, t[2]);
            glActiveTexture(GL_TEXTURE3);
            glBindTexture(GL_TEXTURE_2D, t[3]);
            glActiveTexture(GL_TEXTURE4);
            glBindTexture(GL_TEXTURE_2D, t[4]);

            glBindVertexArray(VAO);
            glDrawArrays(GL_TRIANGLES, ctm.GetMaterialFirstFace(i) * 3, ctm.GetMaterialFaceCount(i) * 3);
        }
    }

    void ManageTextures(GLuint* textures, cyTriMesh ctm, size_t materialNum) {
        if (ctm.M(materialNum).map_Ka) {
            glGenTextures(1, &textures[0]);
            glBindTexture(GL_TEXTURE_2D, textures[0]);
            SetTextureData(ctm.M(materialNum).map_Ka);
        }
        if (ctm.M(materialNum).map_Kd) {
            glGenTextures(1, &textures[1]);
            glBindTexture(GL_TEXTURE_2D, textures[1]);
            SetTextureData(ctm.M(materialNum).map_Kd);
        }
        if (ctm.M(materialNum).map_Ks) {
            glGenTextures(1, &textures[2]);
            glBindTexture(GL_TEXTURE_2D, textures[2]);
            SetTextureData(ctm.M(materialNum).map_Ks);
        }
        if (ctm.M(materialNum).map_Ns) {
            glGenTextures(1, &textures[3]);
            glBindTexture(GL_TEXTURE_2D, textures[3]);
            SetTextureData(ctm.M(materialNum).map_Ns);
        }
        if (ctm.M(materialNum).map_d) {
            glGenTextures(1, &textures[4]);
            glBindTexture(GL_TEXTURE_2D, textures[4]);
            SetTextureData(ctm.M(materialNum).map_d);
        }
    }

    void SetTextureData(const char* name) {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        int width, height, nrChannels;
        unsigned char* data = stbi_load(name, &width, &height, &nrChannels, 0);
        if (data)
        {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);
        }
        else
        {
            std::cout << "Failed to load texture " << name << std::endl;
        }
        stbi_image_free(data);
    }

    void setLightPosition(glm::vec4 lp) {
        lightPos = view * lp;
    }

    ~Model() {
        for (size_t i = 0; i < materialCount; i++)
        {
            delete textures[i];
            glDeleteVertexArrays(1, &VAO);
            glDeleteBuffers(1, &VBO);
        }
    }

    void rotate(float xoffset, float yoffset) {
        yaw += xoffset;
        pitch += yoffset;
    }

    void adjustDistanceFromCamera(float offset) {
        cameraDistance += offset;
    }
};

#endif // !MODEL_H
