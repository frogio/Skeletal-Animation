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
		// ?? �̰� �� �ʿ��Ѱ�?...


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

	// �޽��� Base Vertex, Index�� �����Ѵ�.
	for (unsigned int i = 0; i < meshEntries.size(); i++) {

		// �� �޽��� �����ִ� �ε��� �迭�� ũ��
		meshEntries[i].NumIndices = pScene->mMeshes[i]->mNumFaces * 3/*�� �ϳ��� 3���� ���ؽ��� ���������Ƿ� * 3 */;

		// �޽��� ���� ù ���ؽ�(base vertex) ����
		meshEntries[i].BaseVertex = NumVertices;

		// �޽��� ���� ù �ε���(base index) ����
		meshEntries[i].BaseIndex = NumIndices;

		// �޽��� �����ִ� �� ���ؽ��� �ε��� ���� ���� ���ؽ��� �ε����� Base�� ������Ʈ�Ѵ�.
		NumVertices += pScene->mMeshes[i]->mNumVertices;
		NumIndices += meshEntries[i].NumIndices;
	}

	// ������ ������ �����Ѵ�.
	vertices.reserve(NumVertices);
	bones.resize(NumVertices);
	indices.reserve(NumIndices);


	// �޽��� �ϳ��� �ʱ�ȭ �Ѵ�.
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

	// ���ؽ� ���͸� ä��� �����Ѵ�.
	for (unsigned int i = 0; i < paiMesh->mNumVertices; i++) {

		const aiVector3D* pPos = &(paiMesh->mVertices[i]);
		glm::vec3 glmTempPos = glm::vec3(pPos->x, pPos->y, pPos->z);
		vertices.push_back(glmTempPos);

	}
 
	// �ε��� �迭�� �ʱ�ȭ �Ѵ�.
	for (unsigned int i = 0; i < paiMesh->mNumFaces; i++) {
		const aiFace& Face = paiMesh->mFaces[i];
		//assert(Face.mNumIndices == 3);
		indices.push_back(Face.mIndices[0]);
		indices.push_back(Face.mIndices[1]);
		indices.push_back(Face.mIndices[2]);

		//for (unsigned int j = 0; j < Face.mNumIndices; j++)
		//	indices.push_back(Face.mIndices[j]);

	}

	// �޽��� ���� �ε��Ѵ�.
	if (paiMesh->HasBones())
		LoadBones(index, paiMesh, bones);
}

void SkeletalModel::LoadBones(unsigned int MeshIndex, const aiMesh* pMesh, std::vector<VertexBoneData>& Bones)
{

	// aiMesh ��ü�� aiBone��ü �迭�� �ν��Ͻ��� ���´�.
	// �޽��� �����ϴ� ���� ��� �����´�.
	for (unsigned int i = 0; i < pMesh->mNumBones; i++) {

		// �� �̸��� �����ϰ� �̿� �����ϴ� ID�� �����Ѵ�.
		std::string BoneName(pMesh->mBones[i]->mName.data);
		unsigned int BoneIndex = 0;

		// �� �ȿ� ���� ���� ���. 
		if (m_BoneMapping.find(BoneName) == m_BoneMapping.end()) {

			// ���� ID�� �����Ѵ�. Id�� 0���� ������Ű�鼭 �ο��Ѵ�.
			BoneIndex = accBoneId;
			accBoneId++;

			// �� ���Ϳ� ���ο� �� ������ �����Ѵ�. 
			BoneInfo bi;
			m_BoneInfo.push_back(bi);
		}
		else {
			// �� ID�� �̹� �����Ѵٸ� �� �̸��� �ش��ϴ� ID�� �ִ´�.
			BoneIndex = m_BoneMapping[BoneName];
		}

		// Bone ID�� �����Ǹ� Map�� �߰��Ѵ�.
		m_BoneMapping[BoneName] = BoneIndex;

		// �޽� �������� �� �������� ��ȯ���� �ִ� offset Matrix�� ������ �߰��Ѵ�.
		m_BoneInfo[BoneIndex].BoneOffset = pMesh->mBones[i]->mOffsetMatrix;

		// �� ��(���� ������ Bone)�� ���� ������ �޴� ��� ���ؽ��鿡 ���Ͽ� �ݺ��� ó���Ѵ�.
		// pMesh->mBones[i]->mNumWeights, ���� ���� ������ �ִ� ����ġ ����
		for (unsigned int j = 0; j < pMesh->mBones[i]->mNumWeights; j++) {

			unsigned int VertexID = meshEntries[MeshIndex].BaseVertex + pMesh->mBones[i]->mWeights[j].mVertexId;
			// vertices �迭(Vertex Buffer)�� ���� Index�� ����Ѵ�.
			// BaseVertex�� Vertices�� Index offset, �� Mesh�� Vertex Index ��������
			// ���� Index + VertexId�� vertices �迭 ���� �ε����� ã�Ƴ���.

			// ���� ���ؽ��� �󸶳� ������ �ִ����� ���� ��. (���ؽ� ����ġ)
			float Weight = pMesh->mBones[i]->mWeights[j].mWeight;

			// ���� ���ؽ��� �ش��ϴ� vertexID�� Weight�� ������ �ִ´�, �ִ� 4���� ���� ������ ���ؽ��� ������ �� �� �ִ�. 
			Bones[VertexID].AddBoneData(BoneIndex, Weight);
		}

	}

}

////////////////////////////////////////////////////////////////////////////////////
// Interpolate keys...
//
//
////////////////////////////////////////////////////////////////////////////////////

void SkeletalModel::BoneTransform(float TimeInSeconds, std::vector<Matrix4f>& Transforms)		// ���� �ð��� �� ������ ����, BoneTransform Matrix�� ����
{
	Matrix4f Identity;
	Identity.InitIdentity();

	float TicksPerSecond = pScene->mAnimations[0]->mTicksPerSecond;
	float TimeInTicks = TimeInSeconds * TicksPerSecond;
	float AnimationTime = fmod(TimeInTicks, pScene->mAnimations[0]->mDuration);
	// fmod : �ε��Ҽ��� ������ ����
	// fmod(8.0, 3.1) > return 1.8
	// mDuration�� �ִϸ��̼� ��ü �ð�.
	// AnimationTime ���� ������ [0, pScene->mAnimations[0]->mDuration)
	/*
	std::cout << "TicksPerSecond : " << TicksPerSecond << std::endl;
	std::cout << "TImeInTicks : " << TimeInTicks << std::endl;
	std::cout << "AnimationTime : " << AnimationTime << std::endl;
	*/

	ReadNodeHierarchy(AnimationTime, pScene->mRootNode, Identity);

	Transforms.resize(accBoneId);

	// Populates transforms vector with new bone transformation matrices. 
	// Transforms ���͵��� ���ο� TransformationMatrix�� ä���.
	for (unsigned int i = 0; i < accBoneId; i++)
		Transforms[i] = m_BoneInfo[i].FinalTransformation;

}

void SkeletalModel::adjustBoneTransform(unsigned int meshIdx, const Matrix4f& Transform) {

	// adjust Bone Transform to vertices...



}


unsigned int SkeletalModel::FindRotation(float AnimationTime, const aiNodeAnim* pNodeAnim)
{
	// ȸ�� Ű�������� �����ϴ��� Ȯ���Ѵ�.
	//assert(pNodeAnim->mNumRotationKeys > 0);

	// Find the rotation key just before the current animation time and return the index. 
	// ���� �ִϸ��̼� Ÿ�� �ٷ� ������ ȸ�� Ű �������� �������� index�� �����Ѵ�.
	for (unsigned int i = 0; i < pNodeAnim->mNumRotationKeys - 1; i++) {

		if (AnimationTime < (float)pNodeAnim->mRotationKeys[i + 1].mTime) {
			// i + 1�� index �̹Ƿ� pNodeAnim->mNumRotationKeys - 1������ Ȯ���Ѵ�.
			return i;
			// �ٷ� ���� �������� �����´�.
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

	// ����(Interpolate)�ϱ����ؼ� �ּ� �ΰ��� �������� �ʿ��ϴ�.
	if (pNodeAnim->mNumRotationKeys == 1) {
		Out = pNodeAnim->mRotationKeys[0].mValue;
		return;
	}

	// ���� ȸ�� Ű�������� �����´�.
	unsigned int RotationIndex = FindRotation(AnimationTime, pNodeAnim);

	// ���� ȸ�� Ű�������� ����ϰ� ��踦 üũ�Ѵ�.
	unsigned int NextRotationIndex = (RotationIndex + 1);
	//assert(NextRotationIndex < pNodeAnim->mNumRotationKeys);

	// �� Ű�����Ӱ� �ð� ������ ���Ѵ�.
	float DeltaTime = pNodeAnim->mRotationKeys[NextRotationIndex].mTime - pNodeAnim->mRotationKeys[RotationIndex].mTime;

	// Calculate the elapsed time within the delta time.
	// ���� �ð��� ����� Ű ������ �ð�(DeltaTime) ���� ��� ��ġ�� �ش��ϴ� �� ���Ѵ�. (ratio [0, 1]) 
	float Factor = (AnimationTime - (float)pNodeAnim->mRotationKeys[RotationIndex].mTime) / DeltaTime;
	//assert(Factor >= 0.0f && Factor <= 1.0f);

	// ���� �����Ӱ� ���� �������� �� ���ʹϾ��� ȹ���Ѵ�.
	const aiQuaternion& StartRotationQ = pNodeAnim->mRotationKeys[RotationIndex].mValue;
	const aiQuaternion& EndRotationQ = pNodeAnim->mRotationKeys[NextRotationIndex].mValue;

	// ���������� �� �����Ӱ� ������ �Ѵ�. (Assimp Code)
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
	// ���� ����� �̸��� �����´�.
	std::string NodeName(pNode->mName.data);

	// ù��° �ִϸ��̼��� ����Ѵ�.
	// �ϳ��� �𵨸� ����(dae)�� �� �̻��� �ִϸ��̼� ����� ���� �� �ִ�.
	const aiAnimation* pAnimation = pScene->mAnimations[0];

	// ����� �θ�� ������ ��ȯ����� �����´�.
	Matrix4f NodeTransformation(pNode->mTransformation);

	// ���� ä��
	const aiNodeAnim* pNodeAnim = NULL;

	// ���� ����� ù��° �ִϸ��̼� ä���� ã�´�.
	for (unsigned i = 0; i < pAnimation->mNumChannels; i++) {
		const aiNodeAnim* pNodeAnimIndex = pAnimation->mChannels[i];

		// If there is a match for a channel with the current node's name, then we've found the animation channel. 
		// ���� ����� �̸��� ä���� ��� �̸��� ��ġ�� ���
		// ���� ��忡 �����ϴ� ä���� ã�� ��.
		if (std::string(pNodeAnimIndex->mNodeName.data) == NodeName) {
			pNodeAnim = pNodeAnimIndex;
			// ���� ä���� �����Ѵ�.
		}
	}

	if (pNodeAnim) {
		// ���� ä���� ��ȿ�� ���, ä�ηκ��� ��ȯ����� �����Ѵ�.

		//// Interpolate scaling and generate scaling transformation matrix
		//aiVector3D Scaling;
		//CalcInterpolatedScaling(Scaling, AnimationTime, pNodeAnim);
		//Matrix4f ScalingM;
		//ScalingM.InitScaleTransform(Scaling.x, Scaling.y, Scaling.z);

		// Rotation	������ �����ϰ� ȸ�� ����� �����Ѵ�.
		aiQuaternion RotationQ;
		CalcInterpolatedRotation(RotationQ, AnimationTime, pNodeAnim);
		Matrix4f RotationM = Matrix4f(RotationQ.GetMatrix());

		// Translation ������ �����ϰ� Translation ��ȯ ����� �����Ѵ�.
		aiVector3D Translation;
		CalcInterpolatedTranslation(Translation, AnimationTime, pNodeAnim);
		Matrix4f TranslationM;
		TranslationM.InitTranslationTransform(Translation.x, Translation.y, Translation.z);

		// ���� ��ȯ ��ĵ��� �����Ѵ�.
		NodeTransformation = TranslationM * RotationM;/* *ScalingM;*/
	}

	Matrix4f GlobalTransformation = ParentTransform * NodeTransformation;

	// ���� ��ȯ�� �迭 �� �ε��̵� ���� �����Ѵ�.
	// ? �� �κ��� �� �𸣰���
	if (m_BoneMapping.find(NodeName) != m_BoneMapping.end()) {
		unsigned int BoneIndex = m_BoneMapping[NodeName];
		// �� �̸����κ��� BoneIdx�� �޾ƿ´�.

		m_BoneInfo[BoneIndex].FinalTransformation = m_GlobalInverseTransform * GlobalTransformation * m_BoneInfo[BoneIndex].BoneOffset;
		//;
	// ���ؽ��� ������ ���� Transformation Matrix
	// m_GlobalInverseTransform : it is used to convert the bone transform into model space
	// m_GlobalInverseTransform : �� �������� �� �������� �������´�.
	// GlobalTransformation : ��� ���� ����� ���յ� ��ȯ ���
	// m_BoneInfo[BoneIndex].BoneOffset : �� �������� �� �������� �������´�.
	}


	// �ڽ� ��忡�Ե� �Ȱ��� �����Ѵ�. (preOrder ������� ��ȸ)
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
	// ��ǥ ��ȯ�� ���� ���ؽ����� �����ؾ� ��.

	// BoneTransform..
	std::vector<Matrix4f> Transforms;

	BoneTransform(curTime, Transforms);

	// ���ؽ� ������ BoneTransform ����
	for (int i = 0; i < vertices.size(); i++) {

		Matrix4f BoneTransform;
		BoneTransform.SetZero();
		BoneTransform += (Transforms[bones[i].IDs[0]] * bones[i].Weights[0]);
		BoneTransform += (Transforms[bones[i].IDs[1]] * bones[i].Weights[1]);
		BoneTransform += (Transforms[bones[i].IDs[2]] * bones[i].Weights[2]);
		BoneTransform += (Transforms[bones[i].IDs[3]] * bones[i].Weights[3]);
		// BoneTransform ����
			
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
		// ��鿡 ���� �븻���ʹ� ���ؽ��� 3���� ��� �����ϹǷ� / 3�� ���ش�.
		glBegin(GL_POLYGON);
		for (int j = 0; j < 3; j++)
			glVertex3f(vertices[indices[i + j]].x, vertices[indices[i + j]].y, vertices[indices[i + j]].z);
		glEnd();


	}


	glPopMatrix();
}

void SkeletalModel::CalculFaceNormal() {			// ��鿡 ���� �븻���� ���Ѵ�.

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