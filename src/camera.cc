#include <glm/gtx/rotate_vector.hpp>
#include "camera.h"
#include <math.h>
#include <iostream>
#include <algorithm>

using namespace std;
namespace {
	float pan_speed = 0.2f;
	float roll_speed = 0.1f;
	float rotation_speed = 0.05f;
	float zoom_speed = 0.2f; // hard coded in main when checking movement
};

void Camera::updateLook() {
	look_ = (center_ - eye_)/camera_distance_;
}

void Camera::updateCenter() {
	center_ = eye_ + camera_distance_ * look_;
}

glm::vec3 Camera::calcTan() {
	return glm::cross(look_, up_);
}

bool Camera::checkCollision(glm::vec3 pos, glm::mat3 heights) {
	glm::vec3 actualeye = glm::vec3(pos.x - 0.5, pos.y + 0.5, pos.z - 0.5);
	glm::vec3 remeye = actualeye - glm::vec3(floor(actualeye.x), floor(actualeye.y), floor(actualeye.z));
	float radius = 0.5f;
//	std::cout<< "actual: " << actualeye.x << " " << actualeye.y << " " << actualeye.z << std::endl;
//	std::cout<< "rem: " << remeye.x << " " << remeye.y << " " << remeye.z << std::endl;
	for (int i = 0; i < heights.length(); i++) {
		for (int j = 0; j < heights.length(); j++) {
			if (heights[j][i] >= actualeye.y - 1.75 - 0.02) {
//				cout << "ij" << i << " " << j << endl;
//				std::cout<< "actual: " << actualeye.x << " " << actualeye.y << " " << actualeye.z << std::endl;
//				std::cout<< "rem: " << remeye.x << " " << remeye.y << " " << remeye.z << std::endl;
				if (i == 0 && j == 1) { // top middle
					if (remeye.z < radius)
						return true;
				} else if (i == 2 && j == 1) { // bot mid
					if ((1 -remeye.z) < radius)
						return true;
				} else if (i == 1 && j == 0) { // mid left
					if (remeye.x < radius)
						return true;
				} else if (i == 1 && j == 2) { // mid right
					if (remeye.x > radius)
						return true;
				} else if (i == 0 && j == 0) { // top left // -- corners
					if (sqrt(remeye.x * remeye.x + remeye.z * remeye.z) < radius)
						return true;
				} else if (i == 0 && j == 2) { // top right
					if (sqrt((1 - remeye.x) * (1 - remeye.x) + remeye.z * remeye.z) < radius)
						return true;
				} else if (i == 2 && j == 0) { // bot left
					if (sqrt(remeye.x * remeye.x + (1 - remeye.z) * (1 -remeye.z)) < radius)
						return true;
				} else if (i == 2 && j == 2) { // bot right
					if (sqrt((1 - remeye.x) * (1 - remeye.x) + (1 -remeye.z) * (1 -remeye.z)) < radius)
						return true;
				}
			}
		}
	}
	return false;
}

void Camera::zoom(float scale) {
    eye_ += scale * (zoom_speed * look_);
    center_ += scale * (zoom_speed * look_);
    updateLook();
}

void Camera::zoomPlane(float scale, glm::mat3 heights) {
    glm::vec3 change = scale * (zoom_speed * glm::vec3(look_.x, 0, look_.z));
    if (!checkCollision(eye_ + change, heights)) {
        eye_ += change;
        center_ += change;
    }
	updateLook();
}

void Camera::strafe(float scale) {
	eye_ += scale * (pan_speed * calcTan());
	center_ += scale * (pan_speed * calcTan());
	updateLook();
}

// true if hit ground
bool Camera::changeY(float amount, int height) {
	bool ret = false;
	if (-eye_.y + height + 0.01f + 1.75f > amount)
		ret = true;
	amount = max(-eye_.y + height + 0.01f + 1.75f, amount);
	eye_.y += amount;
	center_.y += amount;
	updateLook();
	return ret;
}

void Camera::strafePlane(float scale, glm::mat3 heights) {
	glm::vec3 tan = calcTan();
	glm::vec3 change = scale * (pan_speed * glm::vec3(tan.x, 0, tan.z));
	if (!checkCollision(eye_ + change, heights)) {
		eye_ += change;
		center_ += change;
	}
	updateLook();
}

void Camera::strafeVert(float scale) {
	eye_ += scale * (pan_speed * up_);
	center_ += scale * (pan_speed * up_);
	updateLook();
}

void Camera::rotateMouse(double x, double y) {
	glm::vec3 mousedir = glm::normalize((float)(-x) * calcTan() + (float) y * up_);
	glm::vec3 axis = glm::normalize(glm::cross(mousedir, glm::normalize(look_)));
	look_ = glm::normalize(glm::rotate(look_, rotation_speed, axis));
    up_ = glm::normalize(glm::rotate(up_, rotation_speed, axis));
    //up_ = glm::cross(axis, look_);
	updateCenter();
}

void Camera::lookDown() {
	look_ = glm::vec3(0,-1,0);
	up_ = glm::vec3(0,0,-1);
    updateCenter();
}

void Camera::rotateLeft() {
	up_ = glm::rotate(up_, rotation_speed, look_);
}

void Camera::rotateRight() {
	up_ = glm::rotate(up_, rotation_speed, -look_);
}

// FIXME: Calculate the view matrix
glm::mat4 Camera::get_view_matrix() const
{
	auto view = glm::mat4();
	auto look4 = glm::vec4(look_, 0.0f);
	auto tan4 = glm::vec4(glm::cross(look_, up_), 0.0f);
	auto up4 = glm::vec4(up_, 0.0f);
	auto eye4 = glm::vec4(eye_, 1.0f);
//	auto center4 = glm::vec4(center_, 1.0f);
	view[0] = tan4;
	view[1] = up4;
	view[2] = -look4;
	view[3] = eye4;

	return glm::inverse(view);
}
