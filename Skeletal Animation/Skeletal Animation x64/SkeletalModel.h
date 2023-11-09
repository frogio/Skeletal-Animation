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
#include "mesh.h"

struct VertexBoneData
{
	unsigned int IDs[4];
	float Weights[4];

	VertexBoneData()
	{
		Reset();
	}

	void Reset()
	{
		memset(IDs, 0, 4 * sizeof(IDs[0]));
		memset(Weights, 0, 4 * sizeof(Weights[0]));
	}

	void AddBoneData(unsigned int BoneID, float Weight)
	{
		for (unsigned int i = 0; i < 4; i++) {

			if (Weights[i] == 0.0) {
				IDs[i] = BoneID;
				Weights[i] = Weight;
				return;
			}

		}

	}
};

struct BoneInfo
{
	Matrix4f FinalTransformation;
	Matrix4f BoneOffset;

	BoneInfo()
	{
		BoneOffset.SetZero();
		FinalTransformation.SetZero();
	}
};

class SkeletalModel{
public:

	SkeletalModel(const std::string& fileName);

	~SkeletalModel(); 

	void LoadModel(const std::string& fileName);								// �־��� ��ηκ��� Animated Mesh�� �ε��Ѵ�.

	void drawAnimation(float curTime);
	void drawSolid();															// Test Drawing

private:
	Assimp::Importer Importer;													// aiScene�� �б� ���� Assimp Import ��ü 
	const aiScene* pScene;														// 3D �� �ֻ��� ����(root)

	std::vector<Mesh> meshes;													// �޽� �迭
	std::vector<VertexBoneData> vertexWeights;									// ���ؽ��� �Ҵ�� ���� ����ġ ����
	unsigned int vertexStreamIdx;												// �޽����� vertices �迭�� ���� ���ڷ� �����Ͽ� ���� ������ vertices �迭�� Index

	unsigned int accBoneIdx;													// �� Index ��������
	std::map<std::string, unsigned int> m_BoneMapping;							// �� �̸����� Index���� ��
	std::vector<BoneInfo> m_BoneInfo;											// Offset Matrix�� Final Transformation�� ���� �� ������ �����ϰ��ִ� �迭

	Matrix4f m_GlobalInverseTransform;

	// Assimp Scene ��ü�κ��� ���� �ʱ�ȭ�Ѵ�.
	void LoadMeshInfo();

	void ProcessNode(aiNode* node, const aiScene* scene);
	Mesh ProcessMesh(aiMesh* mesh, const aiScene* scene);

	void LoadBones(unsigned int verticesPointer, const aiMesh* pMesh);										// �־��� �޽��κ��� �� �����͸� �����´�.
	
	void BoneTransform(float TimeInSeconds, std::vector<Matrix4f>& Transforms);								// Scene �������� Ž���ϰ� ������ �־��� �ð��� ���� ��ȯ����� ������Ʈ�Ѵ�.
	void ReadNodeHierarchy(float AnimationTime, const aiNode* pNode, const Matrix4f& ParentTransform);		// ��� ������ Ž���ϰ� ��ȯ ����� �����ϴ� ������� �Լ�

	// �� Ű ������ ���̿� Interpolating�ϴ� �Լ���
	void CalcInterpolatedRotation(aiQuaternion& Out, float AnimationTime, const aiNodeAnim* pNodeAnim);
	void CalcInterpolatedScaling(aiVector3D& Out, float AnimationTime, const aiNodeAnim* pNodeAnim);
	void CalcInterpolatedTranslation(aiVector3D& Out, float AnimationTime, const aiNodeAnim* pNodeAnim);

	// �־��� �ð����� �ٷ� ������ Ű �������� ��ȯ ����� �ε����� �����´�.
	unsigned int GetCurRotationKey(float AnimationTime, const aiNodeAnim* pNodeAnim);
	unsigned int FindScale(float AnimationTime, const aiNodeAnim* pNodeAnim);
	unsigned int GetCurTranslationKey(float AnimationTime, const aiNodeAnim* pNodeAnim);
	

};

#endif