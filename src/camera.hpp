class Camera {
public:
	glm::vec3 center;
	glm::vec3 up;
	glm::vec3 eye;

	Camera(glm::vec3 pos);

	void update(float delta);
	glm::mat4 view(void);

private:
	float yaw;
	float pitch;

	float sensitivity;
	float speed;
};
