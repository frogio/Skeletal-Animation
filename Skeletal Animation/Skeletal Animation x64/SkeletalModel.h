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


struct VertexBoneData		// 버텍스 하나에 부여된 뼈들의 정보
{
	unsigned int IDs[4];	// 뼈 ID
	float Weights[4];		// 뼈 가중치

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

	void AddBoneData(unsigned int BoneID, float Weight)			// Vertex에 새로운 뼈를 할당.
	{
		for (unsigned int i = 0; i < 4; i++) {

			// 가중치가 0인 곳이 비어있는 곳, 뼈 정보를 삽입.
			if (Weights[i] == 0.0) {
				// ID와 가중치 삽입
				IDs[i] = BoneID;
				Weights[i] = Weight;
				return;
			}

		}
		// should never get here - more bones than we have space for
		//assert(0);
	}
};

// 뼈 정보를 저장한다.
struct BoneInfo
{
	Matrix4f FinalTransformation; // 버텍스들에 적용할 최종 Transformation
	Matrix4f BoneOffset;		  // 지역공간에서 뼈 공간으로의 초기 오프셋

	BoneInfo()
	{
		BoneOffset.SetZero();
		FinalTransformation.SetZero();
	}
};

// MeshEntry는 Assimp Scene으로 부터 읽어온 각각의 Mesh의 메타 데이터이다.
// 모델은 일반적으로 이 Mesh의 집합으로 구성되어 있다.
#define INVALID_MATERIAL 0xFFFFFFFF
struct MeshEntry {

	unsigned int BaseVertex;		// 버텍스 버퍼(Vertices)에서 특정 Mesh에 해당하는 영역의 가장 첫 버텍스		(순수 버텍스 정보) :: 결국 인덱스, 버텍스의 인덱스
	unsigned int BaseIndex;			// 인덱스 버퍼(Indices)에서 특정 Mesh에 해당하는 영역의 가장 첫 인덱스		(버텍스 찍는 순서) :: 결국 인덱스, 인덱스(찍는 순서)의 인덱스
	unsigned int NumIndices;		// Mesh가 갖고있는 전체 인덱스 개수
 
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

	void LoadMesh(const std::string& fileName);									// 주어진 경로로부터 Animated Mesh를 로드한다.
	void BoneTransform(float TimeInSeconds, std::vector<Matrix4f>& Transforms); // Scene 계층도를 탐험하고 각각의 주어진 시간에 따라 변환행렬을 업데이트한다.
	void adjustBoneTransform(unsigned int meshIdx, const Matrix4f& Transform);
	
	void drawAnimation(float curTime);
	void drawSolid();															// Test Drawing

	void PrintAllVertices();

private:
	Assimp::Importer Importer;													// aiScene을 읽기 위한 Assimp Import 객체 
	const aiScene* pScene;														// 3D 모델 최상위 계층(root)

	unsigned int accBoneId;														// 뼈 Index 누적변수
	std::map<std::string, unsigned int> m_BoneMapping;							// 뼈 이름에서 Index로의 맵
	std::vector<BoneInfo> m_BoneInfo;											// Offset Matrix나 Final Transformation과 같은 뼈 정보를 내포하고있는 배열,

	Matrix4f GlobalTransformation;												// 변환 행렬의 루트 노드
	Matrix4f m_GlobalInverseTransform;
	
	std::vector<glm::vec3> vertices;											// 버텍스 배열
	std::vector<unsigned int> indices;											// 버텍스 찍는 순서
	std::vector<glm::vec3> normals;												// face에 대한 노말값

	std::vector<VertexBoneData> bones;											// 버텍스에 할당된 뼈의 정보

	std::vector<MeshEntry> meshEntries;											// 메쉬 엔트리의 배열
	
	glm::vec3 CalcAnimationFaceNormal(int vIdx);
	void CalculFaceNormal();													// 메쉬의 Face 계산

	// 주어진 Assimp 메쉬로부터 데이터를 패치한다.
	void InitMesh(unsigned int index, const aiMesh* paiMesh);

	// Assimp Scene 객체로부터 모델을 초기화한다.
	void InitFromScene();

	void LoadBones(unsigned int MeshIndex, const aiMesh* pMesh, std::vector<VertexBoneData>& Bones);		// 주어진 메쉬로부터 뼈 데이터를 가져온다.
	
	// 두 키 프레임 사이에 Interpolating하는 함수들
	void CalcInterpolatedRotation(aiQuaternion& Out, float AnimationTime, const aiNodeAnim* pNodeAnim);
	void CalcInterpolatedScaling(aiVector3D& Out, float AnimationTime, const aiNodeAnim* pNodeAnim);
	void CalcInterpolatedTranslation(aiVector3D& Out, float AnimationTime, const aiNodeAnim* pNodeAnim);

	// 주어진 시간에서 바로 직전의 키 프레임의 변환 행렬의 인덱스를 가져온다.
	unsigned int FindRotation(float AnimationTime, const aiNodeAnim* pNodeAnim);
	unsigned int FindScale(float AnimationTime, const aiNodeAnim* pNodeAnim);
	unsigned int FindTranslation(float AnimationTime, const aiNodeAnim* pNodeAnim);
	
	// 노드 계층을 탐험하고 변환 행렬을 병합하는 재귀적인 함수
	void ReadNodeHierarchy(float AnimationTime, const aiNode* pNode, const Matrix4f& ParentTransform);

	


};

#endif