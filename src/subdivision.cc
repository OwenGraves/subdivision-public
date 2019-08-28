#include <iostream>
#include "subdivision.h"
#include <glm/gtx/string_cast.hpp>
#include <unordered_set>
#include <unordered_map>
#include "glm/gtx/hash.hpp"

using namespace std;
using namespace glm;

void generate_cube(vector<vec3>& obj_vertices,
				   vector<uvec3>& obj_faces,
				   vec3 min, vec3 max) {
	auto start = obj_vertices.size();

	vec3 vecs[8];
	vecs[0] = vec3(min.x, max.y, min.z);
	vecs[1] = min;
	vecs[2] = vec3(min.x, min.y, max.z);
	vecs[3] = vec3(min.x, max.y, max.z);
	vecs[4] = vec3(max.x, max.y, min.z);
	vecs[5] = vec3(max.x, min.y, min.z);
	vecs[6] = vec3(max.x, min.y, max.z);
	vecs[7] = max;

	for (auto i : vecs) {
		obj_vertices.push_back(i);
	}

	vec3 tris[12];
	//
	tris[0] = vec3(start + 0,start + 1,start + 2);
	tris[1] = vec3(start + 0,start + 2,start + 3);

	tris[2] = vec3(start + 1,start + 0,start + 5);
	tris[3] = vec3(start + 4,start + 5,start + 0);

	tris[4] = vec3(start + 3,start + 4,start + 0);
	tris[5] = vec3(start + 4,start + 3,start + 7);

	tris[6] = vec3(start + 2,start + 1,start + 5);
	tris[7] = vec3(start + 6,start + 2,start + 5);

	tris[8] = vec3(start + 2,start + 6,start + 3);
	tris[9] = vec3(start + 3,start + 6,start + 7);

	tris[10] = vec3(start + 5,start + 4,start + 6);
	tris[11] = vec3(start + 4,start + 7,start + 6);

	for (auto i : tris) {
		obj_faces.emplace_back(i);
	}
}

void
subdivision::generate_geometry(vector<vec3>& obj_vertices,
                          vector<uvec3>& obj_faces) const
{
    obj_vertices.clear();
    obj_faces.clear();
    vec3 min = vec3(-1.0f,-1.0f,-1.0f);
    vec3 max = vec3(1.0f,1.0f,1.0f);

    generate_cube(obj_vertices, obj_faces, min, max);
}

void generate_quad_cube(vector<vec3>& obj_vertices,
				   vector<uvec4>& obj_faces,
				   vec3 min, vec3 max) {
	vec3 vecs[8];
	vecs[0] = vec3(min.x, max.y, min.z);
	vecs[1] = min;
	vecs[2] = vec3(min.x, min.y, max.z);
	vecs[3] = vec3(min.x, max.y, max.z);
	vecs[4] = vec3(max.x, max.y, min.z);
	vecs[5] = vec3(max.x, min.y, min.z);
	vecs[6] = vec3(max.x, min.y, max.z);
	vecs[7] = max;

	for (auto i : vecs) {
		obj_vertices.push_back(i);
	}

	uvec4 quads[6];
	quads[0] = uvec4(0,1,2,3);
	quads[1] = uvec4(4,5,6,7);
	quads[2] = uvec4(4,0,1,5);
	quads[3] = uvec4(7,6,2,3);
	quads[4] = uvec4(7,4,0,3);
	quads[5] = uvec4(6,5,1,2);

	for (auto i : quads) {
		obj_faces.emplace_back(i);
	}
}

void
subdivision::generate_quad_geometry(vector<vec3>& obj_vertices,
							   vector<uvec4>& obj_faces) const
{
	obj_vertices.clear();
	obj_faces.clear();
	vec3 min = vec3(-1.0f,-1.0f,-1.0f);
	vec3 max = vec3(1.0f,1.0f,1.0f);

	generate_quad_cube(obj_vertices, obj_faces, min, max);

	// changing the geometry up a bit
	obj_faces.erase(obj_faces.begin() + 0);
	uint x = obj_vertices.size();
	float xpos = -3.0f;
	float size = 3.0f;
	obj_vertices.emplace_back(vec3(xpos,size,-size));
	obj_vertices.emplace_back(vec3(xpos,-size,-size));
	obj_vertices.emplace_back(vec3(xpos,-size,size));
	obj_vertices.emplace_back(vec3(xpos,size,size));

	obj_faces.emplace_back(uvec4(x, x+1, x+2, x+3));
	obj_faces.emplace_back(uvec4(0, 1, x+1, x));
	obj_faces.emplace_back(uvec4(1, 2, x+2, x + 1));
	obj_faces.emplace_back(uvec4(2, 3, x+3, x + 2));
	obj_faces.emplace_back(uvec4(3, 0, x, x + 3));
}

void subdivision::catmull(vector<vec3> &obj_vertices, vector<uvec4> &obj_faces) {
	// preprocessing structures for speed
	unordered_map<uvec2, unordered_set<uvec4>> edge_to_face;
	unordered_map<uint, unordered_set<uvec4>> vertex_to_face;

	// face points
	uint j = (uint) obj_vertices.size();
	uint original_vertices_number = j;
	unordered_map<uvec4, uint> face_to_point;
	for (auto i : obj_faces) {
		face_to_point[i] = j;
		vec3 sum = vec3(0);
		sum += obj_vertices.at(i.x);
		sum += obj_vertices.at(i.y);
		sum += obj_vertices.at(i.z);
		sum += obj_vertices.at(i.w);
		sum /= 4.0f; // for quads
		obj_vertices.emplace_back(sum);
		j++;
	}

    // get all edges and fill adjacency structures
    unordered_set<uvec2> edges;
    for (auto f : obj_faces) {
        for (uvec2 e : get_quad_edges(f)) {
            edges.insert(e);
			edge_to_face[e].insert(f);
        }
		vertex_to_face[f.x].insert(f);
		vertex_to_face[f.y].insert(f);
		vertex_to_face[f.z].insert(f);
		vertex_to_face[f.w].insert(f);
    }

    // edge points
	unordered_map<uvec2, uint> edge_to_mid;
	for (auto e : edges) {
		edge_to_mid[e] = j;
		vec3 edgemid = 0.5f * obj_vertices.at(e.x) + 0.5f * obj_vertices.at(e.y);
		vec3 facemid = vec3(0);
		int n = 0;
		for (auto adj_face : edge_to_face[e]) {
			facemid += 0.5f * obj_vertices.at(face_to_point.at(adj_face)); // assuming each edge is part of at most two faces
			n++;
		}
		if (n == 2)
			obj_vertices.emplace_back(0.5f * edgemid + 0.5f * facemid);
		else
			obj_vertices.emplace_back(edgemid);
		j++;
	}

	// update vertex points
	vector<vec3> new_vertices(original_vertices_number);

	for (uint i = 0; i < original_vertices_number; i++) {
		auto adj_faces = vertex_to_face[i];
		vector<uvec2> adj_edges;
		for (auto face : adj_faces) {
			for (auto e : get_quad_edges(face)) {
				if (e.x == i || e.y == i)
					adj_edges.emplace_back(e);
			}
		}
		auto n = (float) adj_faces.size();
		vec3 faceavg = vec3(0);
		for (auto face : adj_faces)
			faceavg += obj_vertices.at(face_to_point.at(face));
		faceavg /= n;
		vec3 edgeavg = vec3(0);
		for (auto edge : adj_edges)
			edgeavg += obj_vertices.at(edge_to_mid.at(edge));
		edgeavg /= adj_edges.size();
		new_vertices[i] = ((n - 3) / n) * obj_vertices.at(i) + (1.0f/n) * faceavg + (2.0f/n) * edgeavg;
	}

	for (int i = 0; i < original_vertices_number; i++) {
		obj_vertices[i] = new_vertices[i];
	}

	// subdivide to create new quads
	vector<uvec4> new_faces;
	for (auto k : obj_faces) {
		new_faces.emplace_back(uvec4(k.x, edge_to_mid.at(sort(uvec2(k.x,k.y))),face_to_point.at(k),edge_to_mid.at(sort(uvec2(k.x,k.w)))));
		new_faces.emplace_back(uvec4(k.y,edge_to_mid.at(sort(uvec2(k.y,k.z))),face_to_point.at(k),edge_to_mid.at(sort(uvec2(k.y,k.x)))));
		new_faces.emplace_back(uvec4(k.z,edge_to_mid.at(sort(uvec2(k.z,k.w))),face_to_point.at(k),edge_to_mid.at(sort(uvec2(k.z,k.y)))));
		new_faces.emplace_back(uvec4(k.w,edge_to_mid.at(sort(uvec2(k.w,k.z))),face_to_point.at(k),edge_to_mid.at(sort(uvec2(k.w,k.x)))));
	}

	obj_faces.clear();
	for (auto k : new_faces)
		obj_faces.emplace_back(k);
}

void subdivision::loop(vector<vec3> &obj_vertices, vector<uvec3> &obj_faces) {
	// preprocessing structures for speed
	unordered_map<uvec2, unordered_set<uvec3>> edge_to_face;
	unordered_map<uint, unordered_set<uvec3>> vertex_to_face;

	// get all edges and fill adjacency structures
	unordered_set<uvec2> edges;
	for (auto f : obj_faces) {
	    for (uvec2 e : get_tri_edges(f)) {
	        edges.insert(e);
			edge_to_face[e].insert(f);
	    }
	    vertex_to_face[f.x].insert(f);
		vertex_to_face[f.y].insert(f);
		vertex_to_face[f.z].insert(f);
	}

	// create and append edge vertices
	unordered_map<uvec2, uint> edge_to_mid;
	uint j = (uint) obj_vertices.size();
	uint original_vertices_number = j;
	for (auto e : edges) {
	    edge_to_mid[e] = j;
	    vec3 other_vertex_weights = vec3(0);
	    int n = 0;
		for (auto f : edge_to_face[e]) {
			other_vertex_weights += (1.0f / 8.0f) * obj_vertices.at(get_other_vertex(e, f));
			n++;
		}
	    if (n == 2)
	        obj_vertices.emplace_back((3.0f/8.0f) * obj_vertices.at(e.x) + (3.0f/8.0f) * obj_vertices.at(e.y) + other_vertex_weights);
        else
            obj_vertices.emplace_back(0.5f * obj_vertices.at(e.x) + 0.5f * obj_vertices.at(e.y));
	    j++;
	}

	// update vertex points
	vector<vec3> new_vertices(original_vertices_number);

	for (uint i = 0; i < original_vertices_number; i++) {
		unordered_set<uint> adj_points;
		for (auto f : vertex_to_face[i]) {
			adj_points.insert(f.x);
			adj_points.insert(f.y);
			adj_points.insert(f.z);
		}
		adj_points.erase(i);
		uint n = (uint) adj_points.size();
		double s = 3.0/16.0;
		if (n > 3) {
			s = (1.0/n) * (5.0/8.0 - pow((3.0/8.0 + .25 * cos((2.0 * M_PI)/n)),2.0));
		}
		vec3 sum = vec3(0);
		for (auto v : adj_points) {
			sum += obj_vertices.at(v);
		}
		new_vertices[i] = (float)(1 - n * s) * obj_vertices.at(i) + (float) s * sum;
	}

	for (int i = 0; i < original_vertices_number; i++) {
		obj_vertices[i] = new_vertices[i];
	}

	// subdivide to create the 4 new triangles
	vector<uvec3> new_faces;
	for (auto k : obj_faces) {
		new_faces.emplace_back(uvec3(k.x, edge_to_mid.at(sort(uvec2(k.x,k.y))), edge_to_mid.at(sort(uvec2(k.x,k.z)))));
		new_faces.emplace_back(uvec3(k.y, edge_to_mid.at(sort(uvec2(k.y,k.x))), edge_to_mid.at(sort(uvec2(k.y,k.z)))));
		new_faces.emplace_back(uvec3(k.z, edge_to_mid.at(sort(uvec2(k.z,k.x))), edge_to_mid.at(sort(uvec2(k.z,k.y)))));
		new_faces.emplace_back(uvec3(edge_to_mid.at(sort(uvec2(k.x,k.y))), edge_to_mid.at(sort(uvec2(k.z,k.x))), edge_to_mid.at(sort(uvec2(k.z,k.y)))));
	}

	obj_faces.clear();
	for (auto k : new_faces)
		obj_faces.emplace_back(k);
}

vector<uvec2> subdivision::get_tri_edges(uvec3 face) {
	face = sort(face);
	vector<uvec2> vec;
	vec.emplace_back(face.x,face.y);
	vec.emplace_back(face.y,face.z);
	vec.emplace_back(face.x,face.z);
	return vec;
}

vector<uvec2> subdivision::get_quad_edges(uvec4 face) {
    vector<uvec2> vec;
    vec.emplace_back(sort(uvec2(face.x, face.y)));
	vec.emplace_back(sort(uvec2(face.y, face.z)));
	vec.emplace_back(sort(uvec2(face.z, face.w)));
	vec.emplace_back(sort(uvec2(face.w, face.x)));
	return vec;
}

uvec2 subdivision::sort(uvec2 edge) {
	if (edge.x > edge.y)
		return uvec2(edge.y, edge.x);
	return edge;
}

uvec3 subdivision::sort(uvec3 face) {
	if (face.x > face.y) {
		auto temp = face.x;
		face.x = face.y;
		face.y = temp;
	}
	if (face.y > face.z) {
		auto temp = face.y;
		face.y = face.z;
		face.z = temp;
		if (face.x > face.y) {
			auto temp = face.x;
			face.x = face.y;
			face.y = temp;
		}
	}
	return face;
}

vector<uvec3> subdivision::get_adj_faces(uvec2 edge, vector<uvec3> &obj_faces) {
	vector<uvec3> vec;
	for (auto obj_face : obj_faces) {
		vector<uvec2> curr_edges = get_tri_edges(obj_face);
		for (auto curr_edge : curr_edges) {
			if (curr_edge == edge)
				vec.push_back(obj_face);
		}
	}
	return vec;
}

vector<uvec4> subdivision::get_adj_faces(uvec2 edge, vector<uvec4> &obj_faces) {
	vector<uvec4> vec;
	for (auto obj_face : obj_faces) {
		vector<uvec2> curr_edges = get_quad_edges(obj_face);
		for (auto curr_edge : curr_edges) {
			if (curr_edge == edge)
				vec.push_back(obj_face);
		}
	}
	return vec;
}

vector<uvec3> subdivision::get_adj_faces(uint vertex, vector<uvec3> &obj_faces) {
	vector<uvec3> vec;
	for (auto obj_face : obj_faces) {
			if (vertex == obj_face.x || vertex == obj_face.y || vertex == obj_face.z)
				vec.push_back(obj_face);
	}
	return vec;
}

vector<uvec4> subdivision::get_adj_faces(uint vertex, vector<uvec4> &obj_faces) {
	vector<uvec4> vec;
	for (auto obj_face : obj_faces) {
		if (vertex == obj_face.x || vertex == obj_face.y || vertex == obj_face.z || vertex == obj_face.w)
			vec.push_back(obj_face);
	}
	return vec;
}

uint subdivision::get_other_vertex(uvec2 edge, uvec3 face) {
	unordered_set<uint> set;
	set.insert(face.x);
	set.insert(face.y);
	set.insert(face.z);
	set.erase(edge.x);
	set.erase(edge.y);
	return *set.begin();
}







