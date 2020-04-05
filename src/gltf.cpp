#include <iostream>
#include <stdlib.h>
#include <string>
#include <fstream>
#include <vector>

#include <GL/glew.h>
#include <GL/gl.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>

#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE_WRITE
#define STBI_MSC_SECURE_CRT

#include "texture.hpp"
#include "shader.hpp"
#include "gltf.h"

#define BUFFER_OFFSET(i) ((char *)NULL + (i))

static GLuint bind_VAO(std::vector<uint32_t> &indexbuffer, std::vector<Vertex> &vertexbuffer)
{
	GLuint VAO;
	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);

	GLuint EBO;
	glGenBuffers(1, &EBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint32_t)*indexbuffer.size(), indexbuffer.data(), GL_STATIC_DRAW);

	GLuint VBO;
	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);

	glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex)*vertexbuffer.size(), vertexbuffer.data(), GL_STATIC_DRAW);
	// positions
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), BUFFER_OFFSET(offsetof(Vertex, position)));
	glEnableVertexAttribArray(0);
	// normals
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), BUFFER_OFFSET(offsetof(Vertex, normal)));
	glEnableVertexAttribArray(1);
	// texcoords
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), BUFFER_OFFSET(offsetof(Vertex, uv0)));
	glEnableVertexAttribArray(2);
	// joints
	glVertexAttribIPointer(3, 4, GL_UNSIGNED_INT, sizeof(Vertex), BUFFER_OFFSET(offsetof(Vertex, joints)));
	glEnableVertexAttribArray(3);
	// weights
	glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), BUFFER_OFFSET(offsetof(Vertex, weights)));
	glEnableVertexAttribArray(4);

	return VAO;
}

static GLuint fromglTfImage(tinygltf::Image &gltfimage)
{
	GLuint texture;

	unsigned char *buffer = nullptr;
	buffer = &gltfimage.image[0];

	struct image_t image;
	image.nchannels = gltfimage.component;
	image.width = gltfimage.width;
	image.height = gltfimage.height;
	image.data = buffer;

	texture = gen_texture(&image, GL_SRGB8_ALPHA8, GL_RGBA, GL_UNSIGNED_BYTE);

	//if (buffer != nullptr) { delete [] buffer; }

	std::cout << gltfimage.width << std::endl;
	std::cout << gltfimage.height << std::endl;
	std::cout << "component: " << gltfimage.component << std::endl;
	std::cout << "bits: " << gltfimage.bits << std::endl;
	std::cout << "pixel_type: " << gltfimage.pixel_type << std::endl;
	std::cout << "image size: " << gltfimage.image.size() << std::endl;
	std::cout << texture << std::endl;
	return texture;
}

static inline int32_t GetTypeSizeInBytes(uint32_t ty) {
  if (ty == TINYGLTF_TYPE_SCALAR) {
    return 1;
  } else if (ty == TINYGLTF_TYPE_VEC2) {
    return 2;
  } else if (ty == TINYGLTF_TYPE_VEC3) {
    return 3;
  } else if (ty == TINYGLTF_TYPE_VEC4) {
    return 4;
  } else if (ty == TINYGLTF_TYPE_MAT2) {
    return 4;
  } else if (ty == TINYGLTF_TYPE_MAT3) {
    return 9;
  } else if (ty == TINYGLTF_TYPE_MAT4) {
    return 16;
  } else {
    // Unknown componenty type
    return -1;
  }
}

void gltf::Model::loadNode(gltf::Node *parent, const tinygltf::Node &node, uint32_t nodeIndex, const tinygltf::Model &model, std::vector<uint32_t> &indexBuffer, std::vector<Vertex> &vertexBuffer, float globalscale)
{
	gltf::Node *newNode = new Node{};
	newNode->index = nodeIndex;
	newNode->parent = parent;
	newNode->name = node.name;
	newNode->skinIndex = node.skin;
	newNode->matrix = glm::mat4(1.0f);

	// Generate local node matrix
	glm::vec3 translation = glm::vec3(0.0f);
	if (node.translation.size() == 3) {
		translation = glm::make_vec3(node.translation.data());
		newNode->translation = translation;
	}
	glm::mat4 rotation = glm::mat4(1.0f);
	if (node.rotation.size() == 4) {
		glm::quat q = glm::make_quat(node.rotation.data());
		newNode->rotation = glm::mat4(q);
	}
	glm::vec3 scale = glm::vec3(1.0f);
	if (node.scale.size() == 3) {
		scale = glm::make_vec3(node.scale.data());
	}
	if (node.matrix.size() == 16) {
		newNode->matrix = glm::make_mat4x4(node.matrix.data());
	};

	// Node with children
	if (node.children.size() > 0) {
		for (size_t i = 0; i < node.children.size(); i++) {
			loadNode(newNode, model.nodes[node.children[i]], node.children[i], model, indexBuffer, vertexBuffer, globalscale);
		}
	}

	// Node contains mesh data
	if (node.mesh > -1) {
		const tinygltf::Mesh mesh = model.meshes[node.mesh];
		Mesh *newMesh = new Mesh(newNode->matrix);
		for (size_t j = 0; j < mesh.primitives.size(); j++) {
			const tinygltf::Primitive &primitive = mesh.primitives[j];
			uint32_t indexStart = static_cast<uint32_t>(indexBuffer.size());
			uint32_t vertexStart = static_cast<uint32_t>(vertexBuffer.size());
			uint32_t indexCount = 0;
			uint32_t vertexCount = 0;
			bool hasSkin = false;
     			bool hasIndices = primitive.indices > -1;

			// Indices
			if (hasIndices) {
				const tinygltf::Accessor &accessor = model.accessors[primitive.indices > -1 ? primitive.indices : 0];
				const tinygltf::BufferView &bufferView = model.bufferViews[accessor.bufferView];
				const tinygltf::Buffer &buffer = model.buffers[bufferView.buffer];

				indexCount = static_cast<uint32_t>(accessor.count);
				const void *dataPtr = &(buffer.data[accessor.byteOffset + bufferView.byteOffset]);

				switch (accessor.componentType) {
				case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT: {
				const uint32_t *buf = static_cast<const uint32_t*>(dataPtr);
				for (size_t index = 0; index < accessor.count; index++) {
				indexBuffer.push_back(buf[index]);
				}
				break;
				}
				case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT: {
				const uint16_t *buf = static_cast<const uint16_t*>(dataPtr);
				for (size_t index = 0; index < accessor.count; index++) {
				indexBuffer.push_back(buf[index]);
				}
				break;
				}
				case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE: {
				const uint8_t *buf = static_cast<const uint8_t*>(dataPtr);
				for (size_t index = 0; index < accessor.count; index++) {
				indexBuffer.push_back(buf[index]);
				}
				break;
				}
				default:
				std::cerr << "Index component type " << accessor.componentType << " not supported!" << std::endl;
				return;
				}
			}

			// import vertex data
			const float *bufferPos = nullptr;
			const float *bufferNormals = nullptr;
			const float *bufferTexCoordSet0 = nullptr;
			const float *bufferTexCoordSet1 = nullptr;
			const uint16_t *bufferJoints = nullptr;
			const float *bufferWeights = nullptr;

			int posByteStride;
			int normByteStride;
			int uv0ByteStride;
			int uv1ByteStride;
			int jointByteStride;
			int weightByteStride;

			// Position attribute is required
      			assert(primitive.attributes.find("POSITION") != primitive.attributes.end());

			const tinygltf::Accessor &posAccessor = model.accessors[primitive.attributes.find("POSITION")->second];
			const tinygltf::BufferView &posView = model.bufferViews[posAccessor.bufferView];
			bufferPos = reinterpret_cast<const float *>(&(model.buffers[posView.buffer].data[posAccessor.byteOffset + posView.byteOffset]));
			vertexCount = static_cast<uint32_t>(posAccessor.count);
      			posByteStride = posAccessor.ByteStride(posView) ? (posAccessor.ByteStride(posView) / sizeof(float)) : GetTypeSizeInBytes(TINYGLTF_TYPE_VEC3);


			if (primitive.attributes.find("NORMAL") != primitive.attributes.end()) {
			const tinygltf::Accessor &normAccessor = model.accessors[primitive.attributes.find("NORMAL")->second];
			const tinygltf::BufferView &normView = model.bufferViews[normAccessor.bufferView];
			bufferNormals = reinterpret_cast<const float *>(&(model.buffers[normView.buffer].data[normAccessor.byteOffset + normView.byteOffset]));
			normByteStride = normAccessor.ByteStride(normView) ? (normAccessor.ByteStride(normView) / sizeof(float)) : GetTypeSizeInBytes(TINYGLTF_TYPE_VEC3);
			}

			if (primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end()) {
			const tinygltf::Accessor &uvAccessor = model.accessors[primitive.attributes.find("TEXCOORD_0")->second];
			const tinygltf::BufferView &uvView = model.bufferViews[uvAccessor.bufferView];
			bufferTexCoordSet0 = reinterpret_cast<const float *>(&(model.buffers[uvView.buffer].data[uvAccessor.byteOffset + uvView.byteOffset]));
			uv0ByteStride = uvAccessor.ByteStride(uvView) ? (uvAccessor.ByteStride(uvView) / sizeof(float)) : GetTypeSizeInBytes(TINYGLTF_TYPE_VEC2);
			}
			if (primitive.attributes.find("TEXCOORD_1") != primitive.attributes.end()) {
			const tinygltf::Accessor &uvAccessor = model.accessors[primitive.attributes.find("TEXCOORD_1")->second];
			const tinygltf::BufferView &uvView = model.bufferViews[uvAccessor.bufferView];
			bufferTexCoordSet1 = reinterpret_cast<const float *>(&(model.buffers[uvView.buffer].data[uvAccessor.byteOffset + uvView.byteOffset]));
			uv1ByteStride = uvAccessor.ByteStride(uvView) ? (uvAccessor.ByteStride(uvView) / sizeof(float)) : GetTypeSizeInBytes(TINYGLTF_TYPE_VEC2);
			}

			// Skinning
			// Joints
			if (primitive.attributes.find("JOINTS_0") != primitive.attributes.end()) {
			const tinygltf::Accessor &jointAccessor = model.accessors[primitive.attributes.find("JOINTS_0")->second];
			const tinygltf::BufferView &jointView = model.bufferViews[jointAccessor.bufferView];
			bufferJoints = reinterpret_cast<const uint16_t *>(&(model.buffers[jointView.buffer].data[jointAccessor.byteOffset + jointView.byteOffset]));
			jointByteStride = jointAccessor.ByteStride(jointView) ? (jointAccessor.ByteStride(jointView) / sizeof(bufferJoints[0])) : GetTypeSizeInBytes(TINYGLTF_TYPE_VEC4);
			}

			if (primitive.attributes.find("WEIGHTS_0") != primitive.attributes.end()) {
			const tinygltf::Accessor &weightAccessor = model.accessors[primitive.attributes.find("WEIGHTS_0")->second];
			const tinygltf::BufferView &weightView = model.bufferViews[weightAccessor.bufferView];
			bufferWeights = reinterpret_cast<const float *>(&(model.buffers[weightView.buffer].data[weightAccessor.byteOffset + weightView.byteOffset]));
			weightByteStride = weightAccessor.ByteStride(weightView) ? (weightAccessor.ByteStride(weightView) / sizeof(float)) : GetTypeSizeInBytes(TINYGLTF_TYPE_VEC4);
			}

			hasSkin = (bufferJoints && bufferWeights);

			for (size_t v = 0; v < posAccessor.count; v++) {
			Vertex vert{};
			vert.position = glm::make_vec3(&bufferPos[v * posByteStride]);
			vert.normal = glm::normalize(glm::vec3(bufferNormals ? glm::make_vec3(&bufferNormals[v * normByteStride]) : glm::vec3(0.0f)));
			vert.uv0 = bufferTexCoordSet0 ? glm::make_vec2(&bufferTexCoordSet0[v * uv0ByteStride]) : glm::vec2(0.0f);
			vert.uv1 = bufferTexCoordSet1 ? glm::make_vec2(&bufferTexCoordSet1[v * uv1ByteStride]) : glm::vec2(0.0f);

			vert.joints = hasSkin ? glm::ivec4(glm::make_vec4(&bufferJoints[v * jointByteStride])) : glm::ivec4(0.0f);
			vert.weights = hasSkin ? glm::make_vec4(&bufferWeights[v * weightByteStride]) : glm::vec4(0.0f);
			// Fix for all zero weights
			if (glm::length(vert.weights) == 0.0f) { vert.weights = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f); }
			vertexBuffer.push_back(vert);
			}

			gltf::Primitive *newPrimitive = new struct Primitive(indexStart, indexCount, vertexStart, vertexCount, primitive.material > -1 ? materials[primitive.material] : materials.back());
			newMesh->primitives.push_back(newPrimitive);
		}

		newNode->mesh = newMesh;
	}

	if (parent) {
		parent->children.push_back(newNode);
	} else {
		nodes.push_back(newNode);
	}
	linearNodes.push_back(newNode);
}

void gltf::Model::loadAnimations(tinygltf::Model &gltfModel)
{
	for (tinygltf::Animation &anim : gltfModel.animations) {
		gltf::Animation animation{};
		animation.name = anim.name;
		if (anim.name.empty()) { animation.name = std::to_string(animations.size()); }

		// Samplers
		for (auto &samp : anim.samplers) {
			gltf::AnimationSampler sampler{};

			if (samp.interpolation == "LINEAR") { sampler.interpolation = AnimationSampler::InterpolationType::LINEAR; }
			if (samp.interpolation == "STEP") { sampler.interpolation = AnimationSampler::InterpolationType::STEP; }
			if (samp.interpolation == "CUBICSPLINE") { sampler.interpolation = AnimationSampler::InterpolationType::CUBICSPLINE; }

			// Read sampler input time values
			{
			const tinygltf::Accessor &accessor = gltfModel.accessors[samp.input];
			const tinygltf::BufferView &bufferView = gltfModel.bufferViews[accessor.bufferView];
			const tinygltf::Buffer &buffer = gltfModel.buffers[bufferView.buffer];

			assert(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);

			const void *dataPtr = &buffer.data[accessor.byteOffset + bufferView.byteOffset];
			const float *buf = static_cast<const float*>(dataPtr);
			for (size_t index = 0; index < accessor.count; index++) {
				sampler.inputs.push_back(buf[index]);
			}

			for (auto input : sampler.inputs) {
				if (input < animation.start) { animation.start = input; };
				if (input > animation.end) { animation.end = input; }
			}
			}

			// Read sampler output T/R/S values
			{
			const tinygltf::Accessor &accessor = gltfModel.accessors[samp.output];
			const tinygltf::BufferView &bufferView = gltfModel.bufferViews[accessor.bufferView];
			const tinygltf::Buffer &buffer = gltfModel.buffers[bufferView.buffer];

			assert(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);

			const void *dataPtr = &buffer.data[accessor.byteOffset + bufferView.byteOffset];

			switch (accessor.type) {
			case TINYGLTF_TYPE_VEC3: {
			const glm::vec3 *buf = static_cast<const glm::vec3*>(dataPtr);
			for (size_t index = 0; index < accessor.count; index++) {
			sampler.outputsVec4.push_back(glm::vec4(buf[index], 0.0f));
			}
			break;
			}
			case TINYGLTF_TYPE_VEC4: {
			const glm::vec4 *buf = static_cast<const glm::vec4*>(dataPtr);
			for (size_t index = 0; index < accessor.count; index++) {
			sampler.outputsVec4.push_back(buf[index]);
			}
			break;
			}
			default: {
			std::cout << "unknown type" << std::endl;
			break;
			}
			}
			}

			animation.samplers.push_back(sampler);
		}

		// Channels
		for (auto &source: anim.channels) {
			gltf::AnimationChannel channel{};

			if (source.target_path == "rotation") {
			channel.path = AnimationChannel::PathType::ROTATION;
			}
			if (source.target_path == "translation") {
			channel.path = AnimationChannel::PathType::TRANSLATION;
			}
			if (source.target_path == "scale") {
			channel.path = AnimationChannel::PathType::SCALE;
			}
			if (source.target_path == "weights") {
			std::cout << "weights not yet supported, skipping channel" << std::endl;
			continue;
			}
			channel.samplerIndex = source.sampler;
			channel.node = nodeFromIndex(source.target_node);
			if (!channel.node) {
			continue;
			}

			animation.channels.push_back(channel);
		}

		animations.push_back(animation);
	}
}

void gltf::Model::loadSkins(tinygltf::Model &gltfModel)
{
	for (tinygltf::Skin &source : gltfModel.skins) {
		gltf::Skin *newSkin = new Skin{};
    newSkin->name = source.name;

    // Find skeleton root node
    if (source.skeleton > -1) {
     newSkin->skeletonRoot = nodeFromIndex(source.skeleton);
    }

    // Find joint nodes
    for (int jointIndex : source.joints) {
     Node* node = nodeFromIndex(jointIndex);
     if (node) {
      newSkin->joints.push_back(nodeFromIndex(jointIndex));
     }
    }

    // Get inverse bind matrices from buffer
    if (source.inverseBindMatrices > -1) {
     const tinygltf::Accessor &accessor = gltfModel.accessors[source.inverseBindMatrices];
     const tinygltf::BufferView &bufferView = gltfModel.bufferViews[accessor.bufferView];
     const tinygltf::Buffer &buffer = gltfModel.buffers[bufferView.buffer];
     newSkin->inverseBindMatrices.resize(accessor.count);
     memcpy(newSkin->inverseBindMatrices.data(), &buffer.data[accessor.byteOffset + bufferView.byteOffset], accessor.count * sizeof(glm::mat4));
    }

    skins.push_back(newSkin);
   }

}

void gltf::Model::loadTextures(tinygltf::Model &gltfModel)
{
   for (tinygltf::Texture &tex : gltfModel.textures) {
    tinygltf::Image image = gltfModel.images[tex.source];
    GLuint texture;
    texture = fromglTfImage(image);
    textures.push_back(texture);
   }
}

void gltf::Model::loadMaterials(tinygltf::Model &gltfModel)
{
	for (tinygltf::Material &mat : gltfModel.materials) {
    gltf::Material material{};
    if (mat.values.find("baseColorTexture") != mat.values.end()) {
     material.baseColorTexture = textures[mat.values["baseColorTexture"].TextureIndex()];
     material.texCoordSets.baseColor = mat.values["baseColorTexture"].TextureTexCoord();
    }
    if (mat.values.find("metallicRoughnessTexture") != mat.values.end()) {
     material.metallicRoughnessTexture = textures[mat.values["metallicRoughnessTexture"].TextureIndex()];
     material.texCoordSets.metallicRoughness = mat.values["metallicRoughnessTexture"].TextureTexCoord();
    }
    if (mat.values.find("roughnessFactor") != mat.values.end()) {
     material.roughnessFactor = static_cast<float>(mat.values["roughnessFactor"].Factor());
    }
    if (mat.values.find("metallicFactor") != mat.values.end()) {
     material.metallicFactor = static_cast<float>(mat.values["metallicFactor"].Factor());
    }
    if (mat.values.find("baseColorFactor") != mat.values.end()) {
     material.baseColorFactor = glm::make_vec4(mat.values["baseColorFactor"].ColorFactor().data());
    }
    if (mat.additionalValues.find("normalTexture") != mat.additionalValues.end()) {
     material.normalTexture = textures[mat.additionalValues["normalTexture"].TextureIndex()];
     material.texCoordSets.normal = mat.additionalValues["normalTexture"].TextureTexCoord();
    }
    if (mat.additionalValues.find("emissiveTexture") != mat.additionalValues.end()) {
     material.emissiveTexture = textures[mat.additionalValues["emissiveTexture"].TextureIndex()];
     material.texCoordSets.emissive = mat.additionalValues["emissiveTexture"].TextureTexCoord();
    }
    if (mat.additionalValues.find("occlusionTexture") != mat.additionalValues.end()) {
     material.occlusionTexture = textures[mat.additionalValues["occlusionTexture"].TextureIndex()];
     material.texCoordSets.occlusion = mat.additionalValues["occlusionTexture"].TextureTexCoord();
    }
    /*
    if (mat.additionalValues.find("alphaMode") != mat.additionalValues.end()) {
     tinygltf::Parameter param = mat.additionalValues["alphaMode"];
     if (param.string_value == "BLEND") {
      material.alphaMode = Material::ALPHAMODE_BLEND;
     }
     if (param.string_value == "MASK") {
      material.alphaCutoff = 0.5f;
      material.alphaMode = Material::ALPHAMODE_MASK;
     }
    }
    */
    /*
    if (mat.additionalValues.find("alphaCutoff") != mat.additionalValues.end()) {
     material.alphaCutoff = static_cast<float>(mat.additionalValues["alphaCutoff"].Factor());
    }
    */
    if (mat.additionalValues.find("emissiveFactor") != mat.additionalValues.end()) {
     material.emissiveFactor = glm::vec4(glm::make_vec3(mat.additionalValues["emissiveFactor"].ColorFactor().data()), 1.0);
     material.emissiveFactor = glm::vec4(0.0f);
    }


    materials.push_back(material);
   }
   // Push a default material at the end of the list for meshes with no material assigned
   materials.push_back(Material());

}

void gltf::Model::importf(std::string fpath, float scale)
{
	tinygltf::Model gltfModel;
	tinygltf::TinyGLTF gltfContext;
	std::string error;
	std::string warning;

	bool binary = false;
	size_t extpos = fpath.rfind('.', fpath.length());
	if (extpos != std::string::npos) {
	binary = (fpath.substr(extpos + 1, fpath.length() - extpos) == "glb");
	}

	bool fileLoaded = binary ? gltfContext.LoadBinaryFromFile(&gltfModel, &error, &warning, fpath.c_str()) : gltfContext.LoadASCIIFromFile(&gltfModel, &error, &warning, fpath.c_str());

	std::vector<uint32_t> indexBuffer;
	std::vector<Vertex> vertexBuffer;

	if (fileLoaded) {
		//loadTextureSamplers(gltfModel);
		loadTextures(gltfModel);
		loadMaterials(gltfModel);
		const tinygltf::Scene &scene = gltfModel.scenes[gltfModel.defaultScene > -1 ? gltfModel.defaultScene : 0];
		for (size_t i = 0; i < scene.nodes.size(); i++) {
			const tinygltf::Node node = gltfModel.nodes[scene.nodes[i]];
			loadNode(nullptr, node, scene.nodes[i], gltfModel, indexBuffer, vertexBuffer, scale);
		}

		VAO = bind_VAO(indexBuffer, vertexBuffer);


		if (gltfModel.animations.size() > 0) { loadAnimations(gltfModel); }
		loadSkins(gltfModel);

		for (auto node : linearNodes) {
		// Assign skins
		if (node->skinIndex > -1) {
		node->skin = skins[node->skinIndex];
		}
		// Initial pose
		if (node->mesh) {
		node->update();
		}
		}
	} else {
		std::cerr << "Could not load gltf file: " << error << std::endl;
		return;
	}
}

void gltf::Model::updateAnimation(uint32_t index, float time)
{
	if (animations.empty()) {
		std::cout << ".glTF does not contain animation." << std::endl;
		return;
	}
	if (index > static_cast<uint32_t>(animations.size()) - 1) {
		std::cout << "No animation with index " << index << std::endl;
		return;
	}
	Animation &animation = animations[index];

	bool updated = false;
	for (auto& channel : animation.channels) {
		gltf::AnimationSampler &sampler = animation.samplers[channel.samplerIndex];
		if (sampler.inputs.size() > sampler.outputsVec4.size()) { continue; }

		for (size_t i = 0; i < sampler.inputs.size() - 1; i++) {
			if ((time >= sampler.inputs[i]) && (time <= sampler.inputs[i + 1])) {
				float u = std::max(0.0f, time - sampler.inputs[i]) / (sampler.inputs[i + 1] - sampler.inputs[i]);
				if (u <= 1.0f) {
					switch (channel.path) {
					case gltf::AnimationChannel::PathType::TRANSLATION: {
					glm::vec4 trans = glm::mix(sampler.outputsVec4[i], sampler.outputsVec4[i + 1], u);
					channel.node->translation = glm::vec3(trans);
					break;
					}
					case gltf::AnimationChannel::PathType::SCALE: {
					glm::vec4 trans = glm::mix(sampler.outputsVec4[i], sampler.outputsVec4[i + 1], u);
					channel.node->scale = glm::vec3(trans);
					break;
					}
					case gltf::AnimationChannel::PathType::ROTATION: {
					glm::quat q1;
					q1.x = sampler.outputsVec4[i].x;
					q1.y = sampler.outputsVec4[i].y;
					q1.z = sampler.outputsVec4[i].z;
					q1.w = sampler.outputsVec4[i].w;
					glm::quat q2;
					q2.x = sampler.outputsVec4[i + 1].x;
					q2.y = sampler.outputsVec4[i + 1].y;
					q2.z = sampler.outputsVec4[i + 1].z;
					q2.w = sampler.outputsVec4[i + 1].w;
					channel.node->rotation = glm::normalize(glm::slerp(q1, q2, u));
					break;
					}
					}
					updated = true;
				}
			}
		}
	}

	if (updated) {
		for (auto &node : nodes) {
			node->update();
		}
	}
}

void gltf::Model::display(Shader *shader)
{
	glBindVertexArray(VAO);

	shader->uniform_bool("skinned", !skins.empty());

	for (gltf::Node *node : linearNodes) {
		if (node->mesh) {
			glm::mat4 m = node->getMatrix();
			//shader->uniform_mat4("model", glm::scale(m, glm::vec3(0.01, 0.01, 0.01)));
			shader->uniform_mat4("model", m);
			shader->uniform_array_mat4("u_joint_matrix", node->mesh->uniformBlock.jointcount, node->mesh->uniformBlock.jointMatrix); 
			for (const gltf::Primitive *prim : node->mesh->primitives) {
shader->uniform_vec3("basedcolor", prim->material.baseColorFactor);
glActiveTexture(GL_TEXTURE0);
glBindTexture(GL_TEXTURE_2D, prim->material.baseColorTexture);
glActiveTexture(GL_TEXTURE1);
glBindTexture(GL_TEXTURE_2D, prim->material.metallicRoughnessTexture);
glActiveTexture(GL_TEXTURE2);
glBindTexture(GL_TEXTURE_2D, prim->material.normalTexture);


				if (prim->hasIndices == false) {
					glDrawArrays(GL_TRIANGLES, prim->firstVertex, prim->vertexCount);
				} else {
					glDrawElementsBaseVertex(GL_TRIANGLES, prim->indexCount, GL_UNSIGNED_INT, (GLvoid *)((prim->firstIndex)*sizeof(GL_UNSIGNED_INT)), prim->firstVertex);
				/* TODO use primitive restart */
				}
			}
		}
	}
}

