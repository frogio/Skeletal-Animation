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

	void LoadModel(const std::string& fileName);								// 주어진 경로로부터 Animated Mesh를 로드한다.

	void drawAnimation(float curTime);
	void drawSolid();															// Test Drawing

private:
	Assimp::Importer Importer;													// aiScene을 읽기 위한 Assimp Import 객체 
	const aiScene* pScene;														// 3D 모델 최상위 계층(root)

	std::vector<Mesh> meshes;													// 메쉬 배열
	std::vector<VertexBoneData> vertexWeights;									// 버텍스에 할당된 뼈의 가중치 정보
	unsigned int vertexStreamIdx;												// 메쉬들의 vertices 배열을 전부 일자로 병합하여 만든 가상의 vertices 배열의 Index

	unsigned int accBoneIdx;													// 뼈 Index 누적변수
	std::map<std::string, unsigned int> m_BoneMapping;							// 뼈 이름에서 Index로의 맵
	std::vector<BoneInfo> m_BoneInfo;											// Offset Matrix나 Final Transformation과 같은 뼈 정보를 내포하고있는 배열

	Matrix4f m_GlobalInverseTransform;

	// Assimp Scene 객체로부터 모델을 초기화한다.
	void LoadMeshInfo();

	void ProcessNode(aiNode* node, const aiScene* scene);
	Mesh ProcessMesh(aiMesh* mesh, const aiScene* scene);

	void LoadBones(unsigned int verticesPointer, const aiMesh* pMesh);										// 주어진 메쉬로부터 뼈 데이터를 가져온다.
	
	void BoneTransform(float TimeInSeconds, std::vector<Matrix4f>& Transforms);								// Scene 계층도를 탐험하고 각각의 주어진 시간에 따라 변환행렬을 업데이트한다.
	void ReadNodeHierarchy(float AnimationTime, const aiNode* pNode, const Matrix4f& ParentTransform);		// 노드 계층을 탐험하고 변환 행렬을 병합하는 재귀적인 함수

	// 두 키 프레임 사이에 Interpolating하는 함수들
	void CalcInterpolatedRotation(aiQuaternion& Out, float AnimationTime, const aiNodeAnim* pNodeAnim);
	void CalcInterpolatedScaling(aiVector3D& Out, float AnimationTime, const aiNodeAnim* pNodeAnim);
	void CalcInterpolatedTranslation(aiVector3D& Out, float AnimationTime, const aiNodeAnim* pNodeAnim);

	// 주어진 시간에서 바로 직전의 키 프레임의 변환 행렬의 인덱스를 가져온다.
	unsigned int GetCurRotationKey(float AnimationTime, const aiNodeAnim* pNodeAnim);
	unsigned int FindScale(float AnimationTime, const aiNodeAnim* pNodeAnim);
	unsigned int GetCurTranslationKey(float AnimationTime, const aiNodeAnim* pNodeAnim);
	

};

#endif