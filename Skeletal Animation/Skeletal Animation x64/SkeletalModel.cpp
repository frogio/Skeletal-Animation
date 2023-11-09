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
	LoadModel(fileName);
}

SkeletalModel::~SkeletalModel()
{

}

void SkeletalModel::LoadModel(const std::string& fileName)
{	

	pScene = Importer.ReadFile(fileName.c_str(),
		aiProcess_JoinIdenticalVertices |
		aiProcess_Triangulate |
		aiProcess_FlipUVs |
		aiProcess_SortByPType |
		aiProcess_GenSmoothNormals |
		aiProcess_LimitBoneWeights
	
	);

	if (pScene) {

		m_GlobalInverseTransform = pScene->mRootNode->mTransformation;
		m_GlobalInverseTransform.Inverse();
		// ?? 이게 왜 필요한가?...

		LoadMeshInfo();
	}
	else {
		printf("Error parsing '%s': '%s'\n", fileName.c_str(), Importer.GetErrorString());
	}

}

void SkeletalModel::LoadMeshInfo()
{

	vertexStreamIdx = 0;
	ProcessNode(pScene->mRootNode, pScene);
	vertexWeights.resize(vertexStreamIdx);

	std::cout << "Count of vertex : " << vertexStreamIdx << std::endl;
	std::cout << "Count of vertex weight: " << vertexStreamIdx << std::endl;

	int verticesPointer = 0;

	for (unsigned int i = 0; i < meshes.size(); i++) {
		if (pScene->mMeshes[i]->HasBones()){
			LoadBones(verticesPointer, pScene->mMeshes[i]);
			verticesPointer += meshes[i].GetVertexCount();
		}
	}


}

void SkeletalModel::ProcessNode(aiNode* node, const aiScene* scene) {

	for (unsigned int i = 0; i < node->mNumMeshes; i++) {
		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
		meshes.push_back(ProcessMesh(mesh, scene));
	}

	for (unsigned int i = 0; i < node->mNumChildren; i++)
		ProcessNode(node->mChildren[i], scene);

}

Mesh SkeletalModel::ProcessMesh(aiMesh* mesh, const aiScene* scene) {

	std::vector<Vertex> vertices;
	std::vector<unsigned int> indices;
	std::vector<glm::vec3> normals;

	for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
		Vertex vertex;
		glm::vec3 normal;

		vertex.pos.x = mesh->mVertices[i].x;
		vertex.pos.y = mesh->mVertices[i].y;
		vertex.pos.z = mesh->mVertices[i].z;
		vertex.streamIdx = vertexStreamIdx++;

		normal.x = mesh->mNormals[i].x;
		normal.y = mesh->mNormals[i].y;
		normal.z = mesh->mNormals[i].z;

		vertices.push_back(vertex);
		normals.push_back(normal);

	}
	
	for (unsigned int i = 0; i < mesh->mNumFaces; i++)
	{
		aiFace* face = &mesh->mFaces[i];
		for (unsigned int j = 0; j < face->mNumIndices; j++)
			indices.push_back(face->mIndices[j]);
	}

	return Mesh(vertices, normals, indices);

}

void SkeletalModel::LoadBones(unsigned int verticesPointer, const aiMesh* pMesh)
{

	for (unsigned int i = 0; i < pMesh->mNumBones; i++) {

		std::string BoneName(pMesh->mBones[i]->mName.data);
		unsigned int BoneIndex = 0;

		if (m_BoneMapping.find(BoneName) == m_BoneMapping.end()) {

			BoneIndex = accBoneIdx;
			accBoneIdx++;

			BoneInfo bi;
			m_BoneInfo.push_back(bi);
		}

		else 	
			BoneIndex = m_BoneMapping[BoneName];

		m_BoneMapping[BoneName] = BoneIndex;
		m_BoneInfo[BoneIndex].BoneOffset = pMesh->mBones[i]->mOffsetMatrix;
		
		for (unsigned int j = 0; j < pMesh->mBones[i]->mNumWeights; j++) {

			unsigned int VertexIdx = verticesPointer + pMesh->mBones[i]->mWeights[j].mVertexId;
			float Weight = pMesh->mBones[i]->mWeights[j].mWeight;
			vertexWeights[VertexIdx].AddBoneData(BoneIndex, Weight);
		}

	}

}

////////////////////////////////////////////////////////////////////////////////////
// Interpolate keys...
//
//
////////////////////////////////////////////////////////////////////////////////////

void SkeletalModel::BoneTransform(float TimeInSeconds, std::vector<Matrix4f>& Transforms)
{
	Matrix4f Identity;
	Identity.InitIdentity();

	float TicksPerSecond = pScene->mAnimations[0]->mTicksPerSecond;
	float TimeInTicks = TimeInSeconds * TicksPerSecond;	
	float AnimationTime = fmod(TimeInTicks, pScene->mAnimations[0]->mDuration);

	ReadNodeHierarchy(AnimationTime, pScene->mRootNode, Identity);

	Transforms.resize(m_BoneInfo.size());

	for (unsigned int i = 0; i < m_BoneInfo.size(); i++)
		Transforms[i] = m_BoneInfo[i].FinalTransformation;

}


unsigned int SkeletalModel::GetCurRotationKey(float AnimationTime, const aiNodeAnim* pNodeAnim)
{
	for (unsigned int i = 0; i < pNodeAnim->mNumRotationKeys - 1; i++) {

		if (AnimationTime < (float)pNodeAnim->mRotationKeys[i + 1].mTime)
			return i;

	}

	return 0;
}

unsigned int SkeletalModel::FindScale(float AnimationTime, const aiNodeAnim* pNodeAnim)
{

	for (unsigned int i = 0; i < pNodeAnim->mNumScalingKeys - 1; i++) {
		if (AnimationTime < (float)pNodeAnim->mScalingKeys[i + 1].mTime) {
			return i;
		}
	}

	return 0;
}

unsigned int SkeletalModel::GetCurTranslationKey(float AnimationTime, const aiNodeAnim* pNodeAnim)
{

	for (unsigned int i = 0; i < pNodeAnim->mNumPositionKeys - 1; i++) {
		if (AnimationTime < (float)pNodeAnim->mPositionKeys[i + 1].mTime) {
			return i;
		}
	}

	return 0;
}


void SkeletalModel::CalcInterpolatedRotation(aiQuaternion& Out, float AnimationTime, const aiNodeAnim* pNodeAnim)
{
	if (pNodeAnim->mNumRotationKeys == 1) {
		Out = pNodeAnim->mRotationKeys[0].mValue;
		return;
	}

	unsigned int RotationIndex = GetCurRotationKey(AnimationTime, pNodeAnim);
	unsigned int NextRotationIndex = (RotationIndex + 1);
	
	float DeltaTime = pNodeAnim->mRotationKeys[NextRotationIndex].mTime - pNodeAnim->mRotationKeys[RotationIndex].mTime;
	float Factor = (AnimationTime - (float)pNodeAnim->mRotationKeys[RotationIndex].mTime) / DeltaTime;
	
	const aiQuaternion& StartRotationQ = pNodeAnim->mRotationKeys[RotationIndex].mValue;
	const aiQuaternion& EndRotationQ = pNodeAnim->mRotationKeys[NextRotationIndex].mValue;

	aiQuaternion::Interpolate(Out, StartRotationQ, EndRotationQ, Factor);
	Out = Out.Normalize();
}

void SkeletalModel::CalcInterpolatedScaling(aiVector3D& Out, float AnimationTime, const aiNodeAnim* pNodeAnim)
{
	if (pNodeAnim->mNumScalingKeys == 1) {
		Out = pNodeAnim->mScalingKeys[0].mValue;
		return;
	}

	unsigned int ScalingIndex = FindScale(AnimationTime, pNodeAnim);
	unsigned int NextScalingIndex = (ScalingIndex + 1);
	
	float DeltaTime = pNodeAnim->mScalingKeys[NextScalingIndex].mTime - pNodeAnim->mScalingKeys[ScalingIndex].mTime;
	float Factor = (AnimationTime - (float)pNodeAnim->mScalingKeys[ScalingIndex].mTime) / DeltaTime;
	
	const aiVector3D& Start = pNodeAnim->mScalingKeys[ScalingIndex].mValue;
	const aiVector3D& End = pNodeAnim->mScalingKeys[NextScalingIndex].mValue;

	aiVector3D Delta = End - Start;
	Out = Start + Factor * Delta;
}

void SkeletalModel::CalcInterpolatedTranslation(aiVector3D& Out, float AnimationTime, const aiNodeAnim* pNodeAnim)
{
	if (pNodeAnim->mNumPositionKeys == 1) {
		Out = pNodeAnim->mPositionKeys[0].mValue;
		return;
	}


	unsigned int PositionIndex = GetCurTranslationKey(AnimationTime, pNodeAnim);
	unsigned int NextPositionIndex = (PositionIndex + 1);
	
	float DeltaTime = pNodeAnim->mPositionKeys[NextPositionIndex].mTime - pNodeAnim->mPositionKeys[PositionIndex].mTime;
	float Factor = (AnimationTime - (float)pNodeAnim->mPositionKeys[PositionIndex].mTime) / DeltaTime;
	
	const aiVector3D& Start = pNodeAnim->mPositionKeys[PositionIndex].mValue;
	const aiVector3D& End = pNodeAnim->mPositionKeys[NextPositionIndex].mValue;

	aiVector3D Delta = End - Start;
	Out = Start + Factor * Delta;
}

void SkeletalModel::ReadNodeHierarchy(float AnimationTime, const aiNode* pNode, const Matrix4f& ParentTransform)
{
	Matrix4f IdentityTest;
	IdentityTest.InitIdentity();

	std::string NodeName(pNode->mName.data);

	const aiAnimation* pAnimation = pScene->mAnimations[0];

	Matrix4f NodeTransformation(pNode->mTransformation);

	const aiNodeAnim* pNodeAnim = NULL;

	for (unsigned i = 0; i < pAnimation->mNumChannels; i++) {
		const aiNodeAnim* pNodeAnimIndex = pAnimation->mChannels[i];
	
		if (std::string(pNodeAnimIndex->mNodeName.data) == NodeName) {
			pNodeAnim = pNodeAnimIndex;
			break;
		}
	}

	if (pNodeAnim) {

		//aiVector3D Scaling;
		//CalcInterpolatedScaling(Scaling, AnimationTime, pNodeAnim);
		//Matrix4f ScalingM;
		//ScalingM.InitScaleTransform(Scaling.x, Scaling.y, Scaling.z);

		aiQuaternion RotationQ;
		CalcInterpolatedRotation(RotationQ, AnimationTime, pNodeAnim);
		Matrix4f RotationM = Matrix4f(RotationQ.GetMatrix());

		aiVector3D Translation;
		CalcInterpolatedTranslation(Translation, AnimationTime, pNodeAnim);
		Matrix4f TranslationM;
		TranslationM.InitTranslationTransform(Translation.x, Translation.y, Translation.z);

		NodeTransformation = TranslationM * RotationM;/* *ScalingM;*/
	}

	Matrix4f GlobalTransformation = ParentTransform * NodeTransformation;

	if (m_BoneMapping.find(NodeName) != m_BoneMapping.end()) {

		unsigned int BoneIndex = m_BoneMapping[NodeName];
		m_BoneInfo[BoneIndex].FinalTransformation = m_GlobalInverseTransform * GlobalTransformation * m_BoneInfo[BoneIndex].BoneOffset;

	}

	for (unsigned i = 0; i < pNode->mNumChildren; i++)
		ReadNodeHierarchy(AnimationTime, pNode->mChildren[i], GlobalTransformation);

}

////////////////////////////////////////////////////////////////////////////////////
// Drawing
//
//
////////////////////////////////////////////////////////////////////////////////////

void SkeletalModel::drawAnimation(float curTime)
{

	std::vector<Matrix4f> Transforms;

	BoneTransform(curTime, Transforms);

	for (int i = 0; i < meshes.size(); i++)
		meshes[i].DrawAnimation(Transforms, vertexWeights);


}

void SkeletalModel::drawSolid() {

	for (unsigned int i = 0; i < meshes.size(); i++)
		meshes[i].DrawSolid();

}
