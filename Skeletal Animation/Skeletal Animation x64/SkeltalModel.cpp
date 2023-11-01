#include <iostream>
#include <stdlib.h>



#include "SkeletalModel.h"


////////////////////////////////////////////////////////////////////////////////////
// initialize Skeletal Model...
//
//
////////////////////////////////////////////////////////////////////////////////////


SkeletalModel::SkeletalModel(const std::string& fileName)
{

	// Initialize Skeletal Model
	LoadMesh(fileName);

}

SkeletalModel::~SkeletalModel()
{

}

void SkeletalModel::LoadMesh(const std::string& fileName)
{	
	

	// Load Mesh...


	pScene = Importer.ReadFile(fileName.c_str(),
		aiProcess_JoinIdenticalVertices |
		aiProcess_SortByPType |
		aiProcess_Triangulate |
		aiProcess_GenSmoothNormals |
		aiProcess_FlipUVs |
		aiProcess_LimitBoneWeights);

	if (pScene) {

		m_GlobalInverseTransform = pScene->mRootNode->mTransformation;
		m_GlobalInverseTransform.Inverse();
		// ?? 이게 왜 필요한가?...


		InitFromScene();
	}
	else {
		printf("Error parsing '%s': '%s'\n", fileName.c_str(), Importer.GetErrorString());
	}

}

void SkeletalModel::InitFromScene()
{
	meshEntries.resize(pScene->mNumMeshes);

	unsigned int NumVertices = 0;
	unsigned int NumIndices = 0;

	// 메쉬의 Base Vertex, Index를 설정한다.
	for (unsigned int i = 0; i < meshEntries.size(); i++) {

		// 한 메쉬가 갖고있는 인덱스 배열의 크기
		meshEntries[i].NumIndices = pScene->mMeshes[i]->mNumFaces * 3/*면 하나당 3개의 버텍스를 갖고있으므로 * 3 */;

		// 메쉬의 가장 첫 버텍스(base vertex) 설정
		meshEntries[i].BaseVertex = NumVertices;

		// 메쉬의 가장 첫 인덱스(base index) 설정
		meshEntries[i].BaseIndex = NumIndices;

		// 메쉬가 갖고있는 총 버텍스와 인덱스 수를 더해 버텍스와 인덱스의 Base를 업데이트한다.
		NumVertices += pScene->mMeshes[i]->mNumVertices;
		NumIndices += meshEntries[i].NumIndices;
	}

	// 벡터의 공간을 예약한다.
	vertices.reserve(NumVertices);
	bones.resize(NumVertices);
	indices.reserve(NumIndices);


	// 메쉬를 하나씩 초기화 한다.
	for (unsigned int i = 0; i < meshEntries.size(); i++) {
		
		const aiMesh* paiMesh = pScene->mMeshes[i];
		InitMesh(i, paiMesh);

	}
	std::cout << "Count of vertex : " << NumVertices << std::endl;
	std::cout << "Count of index : " << NumIndices << std::endl;
	std::cout << "Count of bone : " << NumVertices << std::endl;

}

void SkeletalModel::InitMesh(unsigned int index, const aiMesh* paiMesh)
{
	const aiVector3D Zero3D(0.0f, 0.0f, 0.0f);

	// 버텍스 벡터를 채우기 시작한다.
	for (unsigned int i = 0; i < paiMesh->mNumVertices; i++) {

		const aiVector3D* pPos = &(paiMesh->mVertices[i]);
		glm::vec3 glmTempPos = glm::vec3(pPos->x, pPos->y, pPos->z);
		vertices.push_back(glmTempPos);

	}
 
	// 인덱스 배열을 초기화 한다.
	for (unsigned int i = 0; i < paiMesh->mNumFaces; i++) {
		const aiFace& Face = paiMesh->mFaces[i];
		//assert(Face.mNumIndices == 3);
		indices.push_back(Face.mIndices[0]);
		indices.push_back(Face.mIndices[1]);
		indices.push_back(Face.mIndices[2]);

		//for (unsigned int j = 0; j < Face.mNumIndices; j++)
		//	indices.push_back(Face.mIndices[j]);

	}

	// 메쉬의 뼈를 로딩한다.
	if (paiMesh->HasBones())
		LoadBones(index, paiMesh, bones);
}

void SkeletalModel::LoadBones(unsigned int MeshIndex, const aiMesh* pMesh, std::vector<VertexBoneData>& Bones)
{

	// aiMesh 객체가 aiBone객체 배열을 인스턴스로 갖는다.
	// 메쉬에 존재하는 뼈를 모두 가져온다.
	for (unsigned int i = 0; i < pMesh->mNumBones; i++) {

		// 뼈 이름을 추출하고 이에 대응하는 ID를 결정한다.
		std::string BoneName(pMesh->mBones[i]->mName.data);
		unsigned int BoneIndex = 0;

		// 맵 안에 없는 뼈일 경우. 
		if (m_BoneMapping.find(BoneName) == m_BoneMapping.end()) {

			// 뼈의 ID를 설정한다. Id는 0부터 누적시키면서 부여한다.
			BoneIndex = accBoneId;
			accBoneId++;

			// 뼈 벡터에 새로운 뼈 정보를 삽입한다. 
			BoneInfo bi;
			m_BoneInfo.push_back(bi);
		}
		else {
			// 뼈 ID가 이미 존재한다면 뼈 이름에 해당하는 ID를 넣는다.
			BoneIndex = m_BoneMapping[BoneName];
		}

		// Bone ID가 결정되면 Map에 추가한다.
		m_BoneMapping[BoneName] = BoneIndex;

		// 메쉬 공간에서 뼈 공간으로 변환시켜 주는 offset Matrix를 추출해 추가한다.
		m_BoneInfo[BoneIndex].BoneOffset = pMesh->mBones[i]->mOffsetMatrix;

		// 이 뼈(현재 루프의 Bone)의 의해 영향을 받는 모든 버텍스들에 대하여 반복해 처리한다.
		// pMesh->mBones[i]->mNumWeights, 현재 뼈에 영향을 주는 가중치 개수
		for (unsigned int j = 0; j < pMesh->mBones[i]->mNumWeights; j++) {

			unsigned int VertexID = meshEntries[MeshIndex].BaseVertex + pMesh->mBones[i]->mWeights[j].mVertexId;
			// vertices 배열(Vertex Buffer)의 절대 Index를 계산한다.
			// BaseVertex는 Vertices의 Index offset, 즉 Mesh의 Vertex Index 시작지점
			// 기준 Index + VertexId로 vertices 배열 내의 인덱스를 찾아낸다.

			// 뼈가 버텍스에 얼마나 영향을 주는지에 관한 값. (버텍스 가중치)
			float Weight = pMesh->mBones[i]->mWeights[j].mWeight;

			// 현재 버텍스에 해당하는 vertexID와 Weight을 쌍으로 넣는다, 최대 4개의 뼈가 동일한 버텍스에 영향을 줄 수 있다. 
			Bones[VertexID].AddBoneData(BoneIndex, Weight);
		}

	}

}

////////////////////////////////////////////////////////////////////////////////////
// Interpolate keys...
//
//
////////////////////////////////////////////////////////////////////////////////////

void SkeletalModel::BoneTransform(float TimeInSeconds, std::vector<Matrix4f>& Transforms)		// 현재 시간을 초 단위로 제공, BoneTransform Matrix를 리턴
{
	Matrix4f Identity;
	Identity.InitIdentity();

	float TicksPerSecond = pScene->mAnimations[0]->mTicksPerSecond;
	float TimeInTicks = TimeInSeconds * TicksPerSecond;
	float AnimationTime = fmod(TimeInTicks, pScene->mAnimations[0]->mDuration);
	// fmod : 부동소수점 나머지 연산
	// fmod(8.0, 3.1) > return 1.8
	// mDuration은 애니메이션 전체 시간.
	// AnimationTime 값의 범위는 [0, pScene->mAnimations[0]->mDuration)
	/*
	std::cout << "TicksPerSecond : " << TicksPerSecond << std::endl;
	std::cout << "TImeInTicks : " << TimeInTicks << std::endl;
	std::cout << "AnimationTime : " << AnimationTime << std::endl;
	*/

	ReadNodeHierarchy(AnimationTime, pScene->mRootNode, Identity);

	Transforms.resize(accBoneId);

	// Populates transforms vector with new bone transformation matrices. 
	// Transforms 벡터들을 새로운 TransformationMatrix에 채운다.
	for (unsigned int i = 0; i < accBoneId; i++)
		Transforms[i] = m_BoneInfo[i].FinalTransformation;

}

void SkeletalModel::adjustBoneTransform(unsigned int meshIdx, const Matrix4f& Transform) {

	// adjust Bone Transform to vertices...



}


unsigned int SkeletalModel::FindRotation(float AnimationTime, const aiNodeAnim* pNodeAnim)
{
	// 회전 키프레임이 존재하는지 확인한다.
	//assert(pNodeAnim->mNumRotationKeys > 0);

	// Find the rotation key just before the current animation time and return the index. 
	// 현재 애니메이션 타임 바로 직전의 회전 키 프레임을 가져오고 index를 리턴한다.
	for (unsigned int i = 0; i < pNodeAnim->mNumRotationKeys - 1; i++) {

		if (AnimationTime < (float)pNodeAnim->mRotationKeys[i + 1].mTime) {
			// i + 1이 index 이므로 pNodeAnim->mNumRotationKeys - 1까지만 확인한다.
			return i;
			// 바로 직전 프레임을 가져온다.
		}

	}
	//assert(0);

	return 0;
}

unsigned int SkeletalModel::FindScale(float AnimationTime, const aiNodeAnim* pNodeAnim)
{
	//assert(pNodeAnim->mNumScalingKeys > 0);

	// Find the scaling key just before the current animation time and return the index. 
	for (unsigned int i = 0; i < pNodeAnim->mNumScalingKeys - 1; i++) {
		if (AnimationTime < (float)pNodeAnim->mScalingKeys[i + 1].mTime) {
			return i;
		}
	}
	//assert(0);

	return 0;
}

unsigned int SkeletalModel::FindTranslation(float AnimationTime, const aiNodeAnim* pNodeAnim)
{
	//assert(pNodeAnim->mNumPositionKeys > 0);

	// Find the translation key just before the current animation time and return the index. 
	for (unsigned int i = 0; i < pNodeAnim->mNumPositionKeys - 1; i++) {
		if (AnimationTime < (float)pNodeAnim->mPositionKeys[i + 1].mTime) {
			return i;
		}
	}
	//assert(0);

	return 0;
}


void SkeletalModel::CalcInterpolatedRotation(aiQuaternion& Out, float AnimationTime, const aiNodeAnim* pNodeAnim)
{

	// 보간(Interpolate)하기위해선 최소 두개의 프레임이 필요하다.
	if (pNodeAnim->mNumRotationKeys == 1) {
		Out = pNodeAnim->mRotationKeys[0].mValue;
		return;
	}

	// 현재 회전 키프레임을 가져온다.
	unsigned int RotationIndex = FindRotation(AnimationTime, pNodeAnim);

	// 다음 회전 키프레임을 계산하고 경계를 체크한다.
	unsigned int NextRotationIndex = (RotationIndex + 1);
	//assert(NextRotationIndex < pNodeAnim->mNumRotationKeys);

	// 두 키프레임간 시간 간격을 구한다.
	float DeltaTime = pNodeAnim->mRotationKeys[NextRotationIndex].mTime - pNodeAnim->mRotationKeys[RotationIndex].mTime;

	// Calculate the elapsed time within the delta time.
	// 현재 시간이 계산한 키 프레임 시간(DeltaTime) 내에 어느 위치에 해당하는 지 구한다. (ratio [0, 1]) 
	float Factor = (AnimationTime - (float)pNodeAnim->mRotationKeys[RotationIndex].mTime) / DeltaTime;
	//assert(Factor >= 0.0f && Factor <= 1.0f);

	// 현재 프레임과 다음 프레임의 두 쿼터니언을 획득한다.
	const aiQuaternion& StartRotationQ = pNodeAnim->mRotationKeys[RotationIndex].mValue;
	const aiQuaternion& EndRotationQ = pNodeAnim->mRotationKeys[NextRotationIndex].mValue;

	// 최종적으로 두 프레임간 보간을 한다. (Assimp Code)
	aiQuaternion::Interpolate(Out, StartRotationQ, EndRotationQ, Factor);

	// Normalize and set the reference. 
	Out = Out.Normalize();
}

void SkeletalModel::CalcInterpolatedScaling(aiVector3D& Out, float AnimationTime, const aiNodeAnim* pNodeAnim)
{
	// we need at least two values to interpolate...
	if (pNodeAnim->mNumScalingKeys == 1) {
		Out = pNodeAnim->mScalingKeys[0].mValue;
		return;
	}

	unsigned int ScalingIndex = FindScale(AnimationTime, pNodeAnim);
	unsigned int NextScalingIndex = (ScalingIndex + 1);
	//assert(NextScalingIndex < pNodeAnim->mNumScalingKeys);
	float DeltaTime = pNodeAnim->mScalingKeys[NextScalingIndex].mTime - pNodeAnim->mScalingKeys[ScalingIndex].mTime;
	float Factor = (AnimationTime - (float)pNodeAnim->mScalingKeys[ScalingIndex].mTime) / DeltaTime;
	//assert(Factor >= 0.0f && Factor <= 1.0f);
	const aiVector3D& Start = pNodeAnim->mScalingKeys[ScalingIndex].mValue;
	const aiVector3D& End = pNodeAnim->mScalingKeys[NextScalingIndex].mValue;

	aiVector3D Delta = End - Start;
	Out = Start + Factor * Delta;
}

void SkeletalModel::CalcInterpolatedTranslation(aiVector3D& Out, float AnimationTime, const aiNodeAnim* pNodeAnim)
{
	// we need at least two values to interpolate...
	if (pNodeAnim->mNumPositionKeys == 1) {
		Out = pNodeAnim->mPositionKeys[0].mValue;
		return;
	}


	unsigned int PositionIndex = FindTranslation(AnimationTime, pNodeAnim);
	unsigned int NextPositionIndex = (PositionIndex + 1);
	//assert(NextPositionIndex < pNodeAnim->mNumPositionKeys);
	float DeltaTime = pNodeAnim->mPositionKeys[NextPositionIndex].mTime - pNodeAnim->mPositionKeys[PositionIndex].mTime;
	float Factor = (AnimationTime - (float)pNodeAnim->mPositionKeys[PositionIndex].mTime) / DeltaTime;
	//assert(Factor >= 0.0f && Factor <= 1.0f);
	const aiVector3D& Start = pNodeAnim->mPositionKeys[PositionIndex].mValue;
	const aiVector3D& End = pNodeAnim->mPositionKeys[NextPositionIndex].mValue;

	aiVector3D Delta = End - Start;
	Out = Start + Factor * Delta;
}

void SkeletalModel::ReadNodeHierarchy(float AnimationTime, const aiNode* pNode, const Matrix4f& ParentTransform)
{
	Matrix4f IdentityTest;
	IdentityTest.InitIdentity();

	// Obtain the name of the current node 
	// 현재 노드의 이름을 가져온다.
	std::string NodeName(pNode->mName.data);

	// 첫번째 애니메이션을 사용한다.
	// 하나의 모델링 파일(dae)에 둘 이상의 애니메이션 모션을 가질 수 있다.
	const aiAnimation* pAnimation = pScene->mAnimations[0];

	// 노드의 부모와 연관된 변환행렬을 가져온다.
	Matrix4f NodeTransformation(pNode->mTransformation);

	// 현재 채널
	const aiNodeAnim* pNodeAnim = NULL;

	// 현재 노드의 첫번째 애니메이션 채널을 찾는다.
	for (unsigned i = 0; i < pAnimation->mNumChannels; i++) {
		const aiNodeAnim* pNodeAnimIndex = pAnimation->mChannels[i];

		// If there is a match for a channel with the current node's name, then we've found the animation channel. 
		// 현재 노드의 이름과 채널의 노드 이름이 일치할 경우
		// 현재 노드에 대응하는 채널을 찾은 것.
		if (std::string(pNodeAnimIndex->mNodeName.data) == NodeName) {
			pNodeAnim = pNodeAnimIndex;
			// 현재 채널을 지정한다.
		}
	}

	if (pNodeAnim) {
		// 현재 채널이 유효할 경우, 채널로부터 변환행렬을 추출한다.

		//// Interpolate scaling and generate scaling transformation matrix
		//aiVector3D Scaling;
		//CalcInterpolatedScaling(Scaling, AnimationTime, pNodeAnim);
		//Matrix4f ScalingM;
		//ScalingM.InitScaleTransform(Scaling.x, Scaling.y, Scaling.z);

		// Rotation	보간을 적용하고 회전 행렬을 생성한다.
		aiQuaternion RotationQ;
		CalcInterpolatedRotation(RotationQ, AnimationTime, pNodeAnim);
		Matrix4f RotationM = Matrix4f(RotationQ.GetMatrix());

		// Translation 보간을 적용하고 Translation 변환 행렬을 생성한다.
		aiVector3D Translation;
		CalcInterpolatedTranslation(Translation, AnimationTime, pNodeAnim);
		Matrix4f TranslationM;
		TranslationM.InitTranslationTransform(Translation.x, Translation.y, Translation.z);

		// 위의 변환 행렬들을 병합한다.
		NodeTransformation = TranslationM * RotationM;/* *ScalingM;*/
	}

	Matrix4f GlobalTransformation = ParentTransform * NodeTransformation;

	// 최종 변환을 배열 내 인덱싱된 뼈에 적용한다.
	// ? 이 부분을 잘 모르겠음
	if (m_BoneMapping.find(NodeName) != m_BoneMapping.end()) {
		unsigned int BoneIndex = m_BoneMapping[NodeName];
		// 뼈 이름으로부터 BoneIdx를 받아온다.

		m_BoneInfo[BoneIndex].FinalTransformation = m_GlobalInverseTransform * GlobalTransformation * m_BoneInfo[BoneIndex].BoneOffset;
		//;
	// 버텍스에 적용할 최종 Transformation Matrix
	// m_GlobalInverseTransform : it is used to convert the bone transform into model space
	// m_GlobalInverseTransform : 뼈 공간에서 모델 공간으로 돌려놓는다.
	// GlobalTransformation : 모든 조상 노드의 병합된 변환 행렬
	// m_BoneInfo[BoneIndex].BoneOffset : 모델 공간에서 뼈 공간으로 돌려놓는다.
	}


	// 자식 노드에게도 똑같이 적용한다. (preOrder 방식으로 순회)
	for (unsigned i = 0; i < pNode->mNumChildren; i++) {
		ReadNodeHierarchy(AnimationTime, pNode->mChildren[i], GlobalTransformation);
	}
}

////////////////////////////////////////////////////////////////////////////////////
// Drawing
//
//
////////////////////////////////////////////////////////////////////////////////////

void SkeletalModel::drawAnimation(float curTime)
{
	
	std::vector<glm::vec3> copy_vertices(vertices);
	// 좌표 변환은 원본 버텍스부터 시작해야 함.

	// BoneTransform..
	std::vector<Matrix4f> Transforms;

	BoneTransform(curTime, Transforms);

	// 버텍스 단위로 BoneTransform 적용
	for (int i = 0; i < vertices.size(); i++) {

		Matrix4f BoneTransform;
		BoneTransform.SetZero();
		BoneTransform += (Transforms[bones[i].IDs[0]] * bones[i].Weights[0]);
		BoneTransform += (Transforms[bones[i].IDs[1]] * bones[i].Weights[1]);
		BoneTransform += (Transforms[bones[i].IDs[2]] * bones[i].Weights[2]);
		BoneTransform += (Transforms[bones[i].IDs[3]] * bones[i].Weights[3]);
		// BoneTransform 병합
			
		Vector4f v;
		v.x = copy_vertices[i].x;
		v.y = copy_vertices[i].y;
		v.z = copy_vertices[i].z;
		v.w = 1.f;
		
		v = BoneTransform * v;

		copy_vertices[i].x = v.x;
		copy_vertices[i].y = v.y;
		copy_vertices[i].z = v.z;

	}

	// drawing
	glPushMatrix();
	
	for (unsigned int i = 0; i < meshEntries.size(); i++) {
		
		for (int k = 0; k < meshEntries[i].NumIndices - 2; k += 3){

			int idx = k + meshEntries[i].BaseIndex;
			
			glBegin(GL_POLYGON);
			
			glm::vec3 normal = CalcAnimationFaceNormal(idx);
			glNormal3f(normal.x, normal.y, normal.z);

			for (int v = 0; v < 3; v++)
				glVertex3f(copy_vertices[indices[idx + v]].x, copy_vertices[indices[idx + v]].y, copy_vertices[indices[idx + v]].z);

			glEnd();
		}
	}

	glPopMatrix();
	//system("pause");
}

void SkeletalModel::drawSolid() {

	//printf("vertex count : %d\n", vertices.size());
	glPushMatrix();
	CalculFaceNormal();

	for (unsigned int i = 0; i < indices.size() - 2; i += 3) {

		glNormal3f(normals[i / 3].x, normals[i / 3].y, normals[i / 3].z);
		// 평면에 대한 노말벡터는 버텍스를 3개씩 묶어서 설정하므로 / 3을 해준다.
		glBegin(GL_POLYGON);
		for (int j = 0; j < 3; j++)
			glVertex3f(vertices[indices[i + j]].x, vertices[indices[i + j]].y, vertices[indices[i + j]].z);
		glEnd();


	}


	glPopMatrix();
}

void SkeletalModel::CalculFaceNormal() {			// 평면에 대한 노말값을 구한다.

	for (unsigned int i = 0; i < indices.size() - 2; i += 3) {

		glm::vec3 v0 = vertices[indices[i + 1]] - vertices[indices[i]];
		glm::vec3 v1 = vertices[indices[i + 2]] - vertices[indices[i]];

		glm::vec3 normal = glm::normalize(glm::cross(v0, v1));

		normals.push_back(normal);
	}

}

glm::vec3 SkeletalModel::CalcAnimationFaceNormal(int vIdx) {


	glm::vec3 v0 = vertices[indices[vIdx + 1]] - vertices[indices[vIdx]];
	glm::vec3 v1 = vertices[indices[vIdx + 2]] - vertices[indices[vIdx]];

	glm::vec3 normal = glm::normalize(glm::cross(v0, v1));

	return normal;


}


void SkeletalModel::PrintAllVertices() {

	for (int i = 0; i < vertices.size(); i++)
		std::cout << "vertices : " << vertices[i].x << " " << vertices[i].y <<  " " << vertices[i].z << std::endl;

}