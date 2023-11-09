#ifndef __MESH_H__
#define __MESH_H__


#include <string>
#include <vector>

#include <assimp/scene.h>

#include "glm/glm.hpp"
#include "Math3D.h"
#include "gl/glut.h"


struct VertexBoneData;

struct Vertex {
	unsigned int streamIdx;														// ��ü �޽��� ���ؽ� ���͸� 1�������� ������ ����� �ε���
	glm::vec3 pos;
};

class Mesh {
private:
	aiScene* pScene;
	aiMesh* pMesh;
	std::vector<Vertex> vertices;	
	std::vector<unsigned int> indices;
	std::vector<glm::vec3> normals;	

	glm::vec3 CalculFaceNormal(int idx);


public:
	Mesh(std::vector<Vertex> _vertices, std::vector<glm::vec3> _normals, std::vector<unsigned int> _indices);
	void DrawWire();
	void DrawSolid();
	
	int GetVertexCount() {
		return vertices.size();
	}

	void DrawAnimation(std::vector<Matrix4f> & Transform, std::vector<VertexBoneData> & bones);
};

#endif