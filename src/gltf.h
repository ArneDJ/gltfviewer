#pragma once

#include "external/tiny_gltf.h"

#define MAX_NUM_JOINTS 128u

struct Vertex {
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec2 uv0;
	glm::vec2 uv1;
	glm::ivec4 joints;
	glm::vec4 weights;
};

namespace gltf {

struct Node;

struct Material {
	float metallicFactor = 1.0f;
	float roughnessFactor = 1.0f;
	glm::vec4 baseColorFactor = glm::vec4(0.0f);
	glm::vec4 emissiveFactor = glm::vec4(1.0f);
	GLuint baseColorTexture;
	GLuint metallicRoughnessTexture;
	GLuint normalTexture;
	GLuint occlusionTexture;
	GLuint emissiveTexture;
};

struct Primitive {
	uint32_t firstIndex;
	uint32_t indexCount;
	uint32_t firstVertex;
	uint32_t vertexCount;
	Material &material;
	bool indexed;

	Primitive(uint32_t firstIndex, uint32_t indexCount, uint32_t firstVertex, uint32_t vertexCount, Material &material) : firstIndex(firstIndex), indexCount(indexCount), firstVertex(firstVertex), vertexCount(vertexCount), material(material) {
		indexed = indexCount > 0;
	};
};

struct AnimationChannel {
	enum PathType { TRANSLATION, ROTATION, SCALE };
	PathType path;
	Node *node;
	uint32_t samplerIndex;
};

struct AnimationSampler {
	enum InterpolationType { LINEAR, STEP, CUBICSPLINE };
	InterpolationType interpolation;
	std::vector<float> inputs;
	std::vector<glm::vec4> outputsVec4;
};

struct Animation {
	std::string name;
	std::vector<AnimationSampler> samplers;
	std::vector<AnimationChannel> channels;
	float start = std::numeric_limits<float>::max();
	float end = std::numeric_limits<float>::min();
};

struct Mesh {
	std::vector<Primitive*> primitives;

	struct UniformBlock {
	glm::mat4 matrix;
	glm::mat4 jointMatrix[MAX_NUM_JOINTS]{};
	float jointcount{ 0 };
	} uniformBlock;

	Mesh(glm::mat4 matrix) {
		this->uniformBlock.matrix = matrix;
	};
	~Mesh() {
		for (Primitive* p : primitives) { delete p; }
	};
};

struct Skin {
	std::string name;
	Node *skeletonRoot = nullptr;
	std::vector<glm::mat4> inverseBindMatrices;
	std::vector<Node*> joints;
};

struct Node {
	Node *parent;
	uint32_t index;
	std::vector<Node*> children;
	glm::mat4 matrix{1.f};
	std::string name;
	Mesh *mesh;
	Skin *skin;
	int32_t skinIndex = -1;
	glm::vec3 translation{};
	glm::vec3 scale{ 1.0f };
	glm::quat rotation{};

	glm::mat4 localMatrix() {
		return glm::translate(glm::mat4(1.0f), translation) * glm::mat4(rotation) * glm::scale(glm::mat4(1.0f), scale) * matrix;
	}

	glm::mat4 getMatrix() {
		glm::mat4 m = localMatrix();
		gltf::Node *p = parent;
		while (p) {
			m = p->localMatrix() * m;
			p = p->parent;
		}
		return m;
	}

	void update() {
		if (mesh) {
			glm::mat4 m = getMatrix();
			if (skin) {
			mesh->uniformBlock.matrix = m;
			// Update join matrices
			glm::mat4 inverseTransform = glm::inverse(m);
			size_t numJoints = std::min((uint32_t)skin->joints.size(), MAX_NUM_JOINTS);
			for (size_t i = 0; i < numJoints; i++) {
			gltf::Node *jointNode = skin->joints[i];
			glm::mat4 jointMat = jointNode->getMatrix() * skin->inverseBindMatrices[i];
			jointMat = inverseTransform * jointMat;
			mesh->uniformBlock.jointMatrix[i] = jointMat;
			}
			mesh->uniformBlock.jointcount = (float)numJoints;
			}
		}

		for (auto& child : children) {
			child->update();
		}
	}

	~Node() {
		if (mesh) {
			delete mesh;
		}
		for (auto& child : children) { delete child; }
	}

};

class Model {
public:
	void importf(std::string fpath, float scale = 1.f);
	void updateAnimation(uint32_t index, float time);
	void display(Shader *shader);
	std::vector<Animation> animations;
private:
	GLuint VAO = 0;
	std::vector<Node*> nodes;
	std::vector<Node*> linearNodes;
	std::vector<Skin*> skins;
	std::vector<GLuint> textures;
	std::vector<Material> materials;
private:
	void loadTextures(tinygltf::Model &gltfModel);
	void loadMaterials(tinygltf::Model &gltfModel);
	void loadNode(gltf::Node *parent, const tinygltf::Node &node, uint32_t nodeIndex, const tinygltf::Model &model, std::vector<uint32_t> &indexBuffer, std::vector<Vertex> &vertexBuffer, float globalscale);
	void loadAnimations(tinygltf::Model &gltfModel);
	void loadSkins(tinygltf::Model &gltfModel);
private:
	Node* findNode(Node *parent, uint32_t index) {
		Node* nodeFound = nullptr;
		if (parent->index == index) { return parent; }
		for (auto& child : parent->children) {
			nodeFound = findNode(child, index);
			if (nodeFound) { break; }
		}
		return nodeFound;
	}

	Node* nodeFromIndex(uint32_t index) {
		Node* nodeFound = nullptr;
		for (auto &node : nodes) {
			nodeFound = findNode(node, index);
			if (nodeFound) { break; }
		}
		return nodeFound;
	}
};

};
