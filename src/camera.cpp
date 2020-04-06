#include <SDL2/SDL.h>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "camera.hpp"

#define DEFAULT_SENSITIVITY 0.1f
#define DEFAULT_SPEED 10.f

Camera::Camera(glm::vec3 pos = glm::vec3(0.f, 0.f, 0.f)) 
{
	yaw = 0.f;
	pitch = 0.f;
	
	sensitivity = DEFAULT_SENSITIVITY;
	speed = DEFAULT_SPEED;

	center = glm::vec3(0.f, 0.f, 0.f);
	up = glm::vec3(0.f, 1.f, 0.f);
	eye = pos;
}

void Camera::update(float delta)
{
	int x, y;
	const Uint8 *keystates = SDL_GetKeyboardState(NULL);
	SDL_GetRelativeMouseState(&x, &y);

	yaw += (float)x * sensitivity * delta;
	pitch -= (float)y * sensitivity * delta;

	const float MAX_ANGLE = 1.57f;
	const float MIN_ANGLE = -1.57f;
	if (pitch > MAX_ANGLE) { pitch = MAX_ANGLE; }
	if (pitch < MIN_ANGLE) { pitch = MIN_ANGLE; }

	// point the camera in a direction based on mouse input
	center.x = cos(yaw) * cos(pitch);
	center.y = sin(pitch);
	center.z = sin(yaw) * cos(pitch);
	center = glm::normalize(center);

	// move the camera in the new direction
	const float modifier = speed * delta;
	if (keystates[SDL_SCANCODE_W]) { eye += modifier * center; }
	if (keystates[SDL_SCANCODE_S]) { eye -= modifier * center; }
	if (keystates[SDL_SCANCODE_D]) { eye += modifier * glm::normalize(glm::cross(center, up)); }
	if (keystates[SDL_SCANCODE_A]) { eye -= modifier * glm::normalize(glm::cross(center, up)); }
}

glm::mat4 Camera::view(void)
{
	return glm::lookAt(eye, eye + center, up);
}
