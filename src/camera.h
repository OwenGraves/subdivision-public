#ifndef CAMERA_H
#define CAMERA_H

#include <glm/glm.hpp>

class Camera {
public:
	glm::mat4 get_view_matrix() const;
    bool checkCollision(glm::vec3 pos, glm::mat3 heights);
	void zoom(float scale);
    void lookDown();
	void zoomPlane(float scale, glm::mat3 heights);
	void strafe(float scale);
	void strafePlane(float scale, glm::mat3 heights);
	void strafeVert(float scale);
    void rotateMouse(double x, double y);
    void rotateLeft();
    void rotateRight();
    bool changeY(float amount, int height);
	glm::vec3 getEye() {
		return eye_;
	};
	glm::vec3 getLook() {
	    return look_;
	}
    double x = -1.0, y = -1.0;
    bool updateRotate = false;

private:
	float camera_distance_ = 3.0;
	glm::vec3 look_ = glm::normalize(glm::vec3(0.0f, -10.0f, -10.0f));
	glm::vec3 up_ = glm::normalize(glm::vec3(0.0f, 1.0, 0.0f));
	glm::vec3 eye_ = glm::vec3(0.0f, 10.0f, 10.0f);
	glm::vec3 center_ = glm::vec3(eye_ + camera_distance_ * look_);
	//

	void updateLook();
	void updateCenter();
	glm::vec3 calcTan();
};

#endif
