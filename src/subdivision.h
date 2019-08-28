#ifndef subdivision_H
#define subdivision_H

#include <glm/glm.hpp>
#include <vector>

using namespace std;
using namespace glm;

class subdivision {
public:
	void generate_geometry(vector<vec3>& obj_vertices,
	                       vector<uvec3>& obj_faces) const;
    void generate_quad_geometry(vector<vec3>& obj_vertices,
                           vector<uvec4>& obj_faces) const;
	void catmull(vector<vec3>& obj_vertices,
				 vector<uvec4>& obj_faces);
	void loop(std::vector<glm::vec3> &obj_vertices, std::vector<glm::uvec3> &obj_faces);
private:
	vector<uvec2> get_tri_edges(uvec3 face);
    vector<uvec2> get_quad_edges(uvec4 face);
    uvec3 sort(uvec3 face);
	uvec2 sort(uvec2 edge);
	vector<uvec3> get_adj_faces(uvec2 edge, vector<uvec3>& obj_faces);
    vector<uvec4> get_adj_faces(uvec2 edge, vector<uvec4>& obj_faces);
    vector<uvec3> get_adj_faces(uint vertex, vector<uvec3>& obj_faces);
	vector<uvec4> get_adj_faces(uint vertex, vector<uvec4>& obj_faces);
	uint get_other_vertex(uvec2 edge, uvec3 face);
};

#endif
