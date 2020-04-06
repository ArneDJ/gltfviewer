#pragma once

#include "external/tiny_gltf.h"

#define MAX_NUM_JOINTS 128u

struct vertex {
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec2 uv;
	glm::ivec4 joints;
	glm::vec4 weights;
};

namespace gltf {

struct node_t;

struct material_t {
	float metallicf = 1.0f;
	float roughnessf = 1.0f;
	glm::vec4 basecolor = glm::vec4(0.0f);
	GLuint basecolormap;
	GLuint metalroughmap;
	GLuint normalmap;
	GLuint occlusionmap;
	GLuint emissivemap;
};

struct primitive_t {
	uint32_t firstindex;
	uint32_t indexcount;
	uint32_t firstvertex;
	uint32_t vertexcount;
	bool indexed;
	material_t &material;

	primitive_t(uint32_t frstindex, uint32_t indexcnt, uint32_t frstvert, uint32_t vertcnt, material_t &material) : material(material) {
		firstindex = frstindex;
		indexcount = indexcnt;
		firstvertex = frstvert;
		vertexcount = vertcnt;
		indexed = indexcnt > 0;
	};
};

struct animchannel_t {
	enum pathtype { TRANSLATION, ROTATION, SCALE };
	pathtype path;
	node_t *target;
	uint32_t samplerindex;
};

struct animsampler_t {
	enum interpolationtype { LINEAR, STEP, CUBICSPLINE };
	interpolationtype interpolation;
	std::vector<float> inputs;
	std::vector<glm::vec4> outputs;
};

struct animation_t {
	std::string name;
	std::vector<animsampler_t> samplers;
	std::vector<animchannel_t> channels;
	float start = std::numeric_limits<float>::max();
	float end = std::numeric_limits<float>::min();
};

struct mesh_t {
	std::vector<primitive_t*> primitives;

	struct uniformblock {
	glm::mat4 matrix;
	glm::mat4 jointMatrix[MAX_NUM_JOINTS]{};
	float jointcount{ 0 };
	} uniformblock;

	mesh_t(glm::mat4 matrix) {
		this->uniformblock.matrix = matrix;
	};
	~mesh_t() {
		for (primitive_t *p : primitives) { delete p; }
	};
};

struct skin_t {
	std::string name;
	node_t *skeletonRoot = nullptr;
	std::vector<glm::mat4> inversebinds;
	std::vector<node_t*> joints;
};

struct node_t {
	node_t *parent;
	uint32_t index;
	std::vector<node_t*> children;
	glm::mat4 matrix{1.f};
	std::string name;
	mesh_t *mesh;
	skin_t *skin;
	int32_t skinIndex = -1;
	glm::vec3 translation{};
	glm::vec3 scale{ 1.0f };
	glm::quat rotation{};

	glm::mat4 localMatrix() {
		return glm::translate(glm::mat4(1.0f), translation) * glm::mat4(rotation) * glm::scale(glm::mat4(1.0f), scale) * matrix;
	}

	glm::mat4 getMatrix() {
		glm::mat4 m = localMatrix();
		gltf::node_t *p = parent;
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
			mesh->uniformblock.matrix = m;
			glm::mat4 inverseTransform = glm::inverse(m);
			size_t numJoints = std::min((uint32_t)skin->joints.size(), MAX_NUM_JOINTS);
			for (size_t i = 0; i < numJoints; i++) {
			gltf::node_t *jointNode = skin->joints[i];
			glm::mat4 jointMat = jointNode->getMatrix() * skin->inversebinds[i];
			jointMat = inverseTransform * jointMat;
			mesh->uniformblock.jointMatrix[i] = jointMat;
			}
			mesh->uniformblock.jointcount = (float)numJoints;
			}
		}

		for (auto& child : children) {
			child->update();
		}
	}

	~node_t() {
		if (mesh) {
			delete mesh;
		}
		for (auto &child : children) { delete child; }
	}

};

class Model {
public:
	void importf(std::string fpath, float scale = 1.f);
	void updateAnimation(uint32_t index, float time);
	void display(Shader *shader);
	std::vector<animation_t> animations;
private:
	GLuint VAO = 0;
	std::vector<node_t*> nodes;
	std::vector<node_t*> linearNodes;
	std::vector<skin_t*> skins;
	std::vector<GLuint> textures;
	std::vector<material_t> materials;
private:
	void load_textures(tinygltf::Model &gltfmodel);
	void load_materials(tinygltf::Model &gltfmodel);
	void load_node(gltf::node_t *parent, const tinygltf::Node &node, uint32_t nodeIndex, const tinygltf::Model &model, std::vector<uint32_t> &indexBuffer, std::vector<vertex> &vertexBuffer, float globalscale);
	void load_animations(tinygltf::Model &gltfModel);
	void load_skins(tinygltf::Model &gltfModel);
	void load_mesh(const tinygltf::Model &model, const tinygltf::Mesh &mesh, gltf::mesh_t *newmesh, std::vector<uint32_t> &indexbuffer, std::vector<vertex> &vertexbuffer);
private:
	node_t *findnode(node_t *parent, uint32_t index) {
		node_t* found = nullptr;
		if (parent->index == index) { return parent; }
		for (auto &child : parent->children) {
			found = findnode(child, index);
			if (found) { break; }
		}
		return found;
	}

	node_t *nodefrom(uint32_t index) {
		node_t *found = nullptr;
		for (auto &node : nodes) {
			found = findnode(node, index);
			if (found) { break; }
		}
		return found;
	}
};

};
