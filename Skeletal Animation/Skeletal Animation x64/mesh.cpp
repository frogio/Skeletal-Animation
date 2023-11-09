#include <iostream>

#include "SkeletalModel.h"
#include "mesh.h"


Mesh::Mesh(std::vector<Vertex> _vertices, std::vector<glm::vec3> _normals, std::vector<unsigned int> _indices) : 
	vertices(_vertices), normals(_normals) , indices(_indices)

{}

void Mesh::DrawWire() {
	glPushMatrix();
		glDisable(GL_LIGHTING);
		
		for (unsigned int i = 0; i < indices.size(); i += 3) {		
			glBegin(GL_LINES);

			for (int j = 0; j < 3; j++){

				int start = indices[i + j % 3];
				int end = indices[i + (j + 1) % 3];
				glVertex3f(vertices[start].pos.x, vertices[start].pos.y, vertices[start].pos.z);
				glVertex3f(vertices[end].pos.x, vertices[end].pos.y, vertices[end].pos.z);

			}

			glEnd();
		}
		glEnable(GL_LIGHTING);
	glPopMatrix();
}

void Mesh::DrawSolid() {

	glPushMatrix();

	for (unsigned int i = 0; i < indices.size(); i += 3) {
		glBegin(GL_POLYGON);
		
		glm::vec3 normal = CalculFaceNormal(i);
		glNormal3f(normal.x, normal.y, normal.z);
		for (int j = 0; j < 3; j++)
			glVertex3f(vertices[indices[i + j]].pos.x, vertices[indices[i + j]].pos.y, vertices[indices[i + j]].pos.z);

		glEnd();
	}

	glPopMatrix();
}

void Mesh::DrawAnimation(std::vector<Matrix4f> & Transforms, std::vector<VertexBoneData>& bones) {

	std::vector<Vertex> copy_vertices(vertices);

	for (int i = 0; i < vertices.size(); i++) {

		int vertexStreamIdx = vertices[i].streamIdx;

		Matrix4f BoneTransform;
		BoneTransform.SetZero();
		BoneTransform += (Transforms[bones[vertexStreamIdx].IDs[0]] * bones[vertexStreamIdx].Weights[0]);
		BoneTransform += (Transforms[bones[vertexStreamIdx].IDs[1]] * bones[vertexStreamIdx].Weights[1]);
		BoneTransform += (Transforms[bones[vertexStreamIdx].IDs[2]] * bones[vertexStreamIdx].Weights[2]);
		BoneTransform += (Transforms[bones[vertexStreamIdx].IDs[3]] * bones[vertexStreamIdx].Weights[3]);
		// BoneTransform º´ÇÕ

		Vector4f v;
		v.x = copy_vertices[i].pos.x;
		v.y = copy_vertices[i].pos.y;
		v.z = copy_vertices[i].pos.z;
		v.w = 1.f;

		v = BoneTransform * v;

		copy_vertices[i].pos.x = v.x;
		copy_vertices[i].pos.y = v.y;
		copy_vertices[i].pos.z = v.z;

	}
	
	// drawing
	glPushMatrix();

	for (unsigned int i = 0; i < indices.size(); i += 3) {
		glBegin(GL_POLYGON);

		for (int j = 0; j < 3; j++){
			glNormal3f(normals[indices[i + j]].x, normals[indices[i + j]].y, normals[indices[i + j]].z);
			glVertex3f(copy_vertices[indices[i + j]].pos.x, copy_vertices[indices[i + j]].pos.y, copy_vertices[indices[i + j]].pos.z);
		}
		glEnd();
	}
	glPopMatrix();
}


glm::vec3 Mesh::CalculFaceNormal(int idx) {
		
	glm::vec3 v0 = vertices[indices[idx + 1]].pos - vertices[indices[idx]].pos;
	glm::vec3 v1 = vertices[indices[idx + 2]].pos - vertices[indices[idx]].pos;

	return glm::normalize(glm::cross(v0, v1));
}
