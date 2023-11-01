#ifndef __MODEL_H__
#define __MODEL_H__

#include <iostream>
#include <string>
#include <vector>
#include <map>

#include <assimp/Importer.hpp>      // C++ importer interface
#include <assimp/scene.h>           // Output data structure
#include <assimp/postprocess.h>     // Post processing fla

#include "glut.h"
#include "glm/glm.hpp"
#include "Math3D.h"


struct VertexBoneData		// ���ؽ� �ϳ��� �ο��� ������ ����
{
	unsigned int IDs[4];	// �� ID
	float Weights[4];		// �� ����ġ

	VertexBoneData()
	{
		// 0's out the arrays. 
		Reset();
	}

	void Reset()
	{
		memset(IDs, 0, 4 * sizeof(IDs[0]));
		memset(Weights, 0, 4 * sizeof(Weights[0]));
	}

	void AddBoneData(unsigned int BoneID, float Weight)			// Vertex�� ���ο� ���� �Ҵ�.
	{
		for (unsigned int i = 0; i < 4; i++) {

			// ����ġ�� 0�� ���� ����ִ� ��, �� ������ ����.
			if (Weights[i] == 0.0) {
				// ID�� ����ġ ����
				IDs[i] = BoneID;
				Weights[i] = Weight;
				return;
			}

		}
		// should never get here - more bones than we have space for
		//assert(0);
	}
};

// �� ������ �����Ѵ�.
struct BoneInfo
{
	Matrix4f FinalTransformation; // ���ؽ��鿡 ������ ���� Transformation
	Matrix4f BoneOffset;		  // ������������ �� ���������� �ʱ� ������

	BoneInfo()
	{
		BoneOffset.SetZero();
		FinalTransformation.SetZero();
	}
};

// MeshEntry�� Assimp Scene���� ���� �о�� ������ Mesh�� ��Ÿ �������̴�.
// ���� �Ϲ������� �� Mesh�� �������� �����Ǿ� �ִ�.
#define INVALID_MATERIAL 0xFFFFFFFF
struct MeshEntry {

	unsigned int BaseVertex;		// ���ؽ� ����(Vertices)���� Ư�� Mesh�� �ش��ϴ� ������ ���� ù ���ؽ�		(���� ���ؽ� ����) :: �ᱹ �ε���, ���ؽ��� �ε���
	unsigned int BaseIndex;			// �ε��� ����(Indices)���� Ư�� Mesh�� �ش��ϴ� ������ ���� ù �ε���		(���ؽ� ��� ����) :: �ᱹ �ε���, �ε���(��� ����)�� �ε���
	unsigned int NumIndices;		// Mesh�� �����ִ� ��ü �ε��� ����
 
	unsigned int MaterialIndex;

	MeshEntry()
	{

		NumIndices = 0;
		BaseVertex = 0;
		BaseIndex = 0;
		MaterialIndex = INVALID_MATERIAL;
	}

	~MeshEntry() {}
};

class SkeletalModel{
public:

	SkeletalModel(const std::string& fileName);

	~SkeletalModel(); 

	void LoadMesh(const std::string& fileName);									// �־��� ��ηκ��� Animated Mesh�� �ε��Ѵ�.
	void BoneTransform(float TimeInSeconds, std::vector<Matrix4f>& Transforms); // Scene �������� Ž���ϰ� ������ �־��� �ð��� ���� ��ȯ����� ������Ʈ�Ѵ�.
	void adjustBoneTransform(unsigned int meshIdx, const Matrix4f& Transform);
	
	void drawAnimation(float curTime);
	void drawSolid();															// Test Drawing

	void PrintAllVertices();

private:
	Assimp::Importer Importer;													// aiScene�� �б� ���� Assimp Import ��ü 
	const aiScene* pScene;														// 3D �� �ֻ��� ����(root)

	unsigned int accBoneId;														// �� Index ��������
	std::map<std::string, unsigned int> m_BoneMapping;							// �� �̸����� Index���� ��
	std::vector<BoneInfo> m_BoneInfo;											// Offset Matrix�� Final Transformation�� ���� �� ������ �����ϰ��ִ� �迭,

	Matrix4f GlobalTransformation;												// ��ȯ ����� ��Ʈ ���
	Matrix4f m_GlobalInverseTransform;
	
	std::vector<glm::vec3> vertices;											// ���ؽ� �迭
	std::vector<unsigned int> indices;											// ���ؽ� ��� ����
	std::vector<glm::vec3> normals;												// face�� ���� �븻��

	std::vector<VertexBoneData> bones;											// ���ؽ��� �Ҵ�� ���� ����

	std::vector<MeshEntry> meshEntries;											// �޽� ��Ʈ���� �迭
	
	glm::vec3 CalcAnimationFaceNormal(int vIdx);
	void CalculFaceNormal();													// �޽��� Face ���

	// �־��� Assimp �޽��κ��� �����͸� ��ġ�Ѵ�.
	void InitMesh(unsigned int index, const aiMesh* paiMesh);

	// Assimp Scene ��ü�κ��� ���� �ʱ�ȭ�Ѵ�.
	void InitFromScene();

	void LoadBones(unsigned int MeshIndex, const aiMesh* pMesh, std::vector<VertexBoneData>& Bones);		// �־��� �޽��κ��� �� �����͸� �����´�.
	
	// �� Ű ������ ���̿� Interpolating�ϴ� �Լ���
	void CalcInterpolatedRotation(aiQuaternion& Out, float AnimationTime, const aiNodeAnim* pNodeAnim);
	void CalcInterpolatedScaling(aiVector3D& Out, float AnimationTime, const aiNodeAnim* pNodeAnim);
	void CalcInterpolatedTranslation(aiVector3D& Out, float AnimationTime, const aiNodeAnim* pNodeAnim);

	// �־��� �ð����� �ٷ� ������ Ű �������� ��ȯ ����� �ε����� �����´�.
	unsigned int FindRotation(float AnimationTime, const aiNodeAnim* pNodeAnim);
	unsigned int FindScale(float AnimationTime, const aiNodeAnim* pNodeAnim);
	unsigned int FindTranslation(float AnimationTime, const aiNodeAnim* pNodeAnim);
	
	// ��� ������ Ž���ϰ� ��ȯ ����� �����ϴ� ������� �Լ�
	void ReadNodeHierarchy(float AnimationTime, const aiNode* pNode, const Matrix4f& ParentTransform);

	


};

#endif