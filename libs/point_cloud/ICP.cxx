#include "point_cloud.h"
#include <fstream>
#include "ICP.h"

ICP::ICP() {
	sourceCloud = nullptr;
	targetCloud = nullptr;
	tree = nullptr;
	crspd_source = nullptr;
	crspd_target = nullptr;
	this->maxIterations = 400;
	this->numRandomSamples = 400;
	this->eps = 1e-8;
}

ICP::~ICP() {
	delete tree;
}

void ICP::initialize()
{
	if (targetCloud==nullptr) {
		std::cerr << "ICP::initialize: target cloud missing, can't build ann tree\n";
		return;
	}
	if (tree) {
		delete tree;
	}

	tree = new ann_tree();
	tree->build(*targetCloud);
}

void ICP::clear()
{
	delete tree;
	tree = nullptr;
}

void ICP::set_source_cloud(const point_cloud &inputCloud) {
	sourceCloud = &inputCloud;
}

void ICP::set_target_cloud(const point_cloud &inputCloud) {
	targetCloud = &inputCloud;
}

void ICP::set_iterations(int Iter) {
	this->maxIterations = Iter;
}

void ICP::set_num_random(int NR) {
	this->numRandomSamples = NR;
}

void ICP::set_eps(float e) {
	this->eps = e;
}

///output the rotation matrix and translation vector
void ICP::reg_icp(Mat& rotation_mat, Dir& translation_vec) {
	if (!tree) {
		initialize();
		//std::cerr << "ICP::reg_icp: called reg_icp before initialization!\n";
		return;
	}
	if (!(sourceCloud && targetCloud)) {
		std::cerr << "ICP::reg_icp: source or target cloud not set!\n";
		return;
	}
	//point_cloud* output = new point_cloud();
	Pnt source_center;
	Pnt target_center;
	source_center.zeros();
	target_center.zeros();
	/// create the ann tree
	ann_tree* tree = new ann_tree();
	tree->build(*targetCloud);
	size_t num_source_points = sourceCloud->get_nr_points();

	get_center_point(*targetCloud, target_center);
	get_center_point(*sourceCloud, source_center);
	Pnt p;

	float cost = 1.0;
	///define min as Infinity
	float min = DBL_MAX;
	std::srand(std::time(0));

	Mat fA(0.0f);             // this initializes fA to matrix filled with zeros

	cgv::math::mat<float> U, V;
	cgv::math::diag_mat<float> Sigma;
	U.zeros();
	V.zeros();
	Sigma.zeros();
	for (int iter = 0; iter < maxIterations && abs(cost) > eps; iter++)
	{
		cost = 0.0;
		point_cloud Q,S;
		Q.resize(sourceCloud->get_nr_points());
		S.resize(sourceCloud->get_nr_points());
		source_center = rotation_mat * source_center + translation_vec;
		fA.zeros();
		for (int i = 0; i < sourceCloud->get_nr_points(); i++)
		{
			int randSample = std::rand() % sourceCloud->get_nr_points();
			/// sample the source point cloud
			S.pnt(i) = sourceCloud->pnt(randSample);
			Q.pnt(i) = targetCloud->pnt(tree->find_closest(S.pnt(i)));
			/// get the closest point in the target point cloud
			//Q.pnt(i) = targetCloud->pnt(i);
			fA += Mat(Q.pnt(i) - target_center, rotation_mat * S.pnt(i) + translation_vec - source_center);
		}
		///cast fA to A
		cgv::math::mat<float> A(3, 3, &fA(0, 0));
		cgv::math::svd(A, U, Sigma, V);
		Mat fU(3, 3, &U(0, 0)), fV(3, 3, &V(0, 0));
		///get new R and t
		Mat rotation_update_mat = fU * cgv::math::transpose(fV);
		Dir translation_update_vec = target_center - rotation_update_mat * source_center;
		///calculate error function E(R,t)
		for (int i = 0; i < S.get_nr_points(); i++){
			///transform Pi to R*Pi + t
			S.pnt(i) = rotation_mat * S.pnt(i) + translation_vec;
			///the new rotation matrix: rotation_update_mat
			float tempcost = error(Q.pnt(i), S.pnt(i), rotation_update_mat, translation_update_vec);
			cost += error(Q.pnt(i), S.pnt(i), rotation_update_mat, translation_update_vec);
		}
		cost /= sourceCloud->get_nr_points();
		///judge if cost is decreasing, and is larger than eps. If so, update the R and t, otherwise stop and output R and t
		if (min >= abs(cost)) {
			///update the R and t
			rotation_mat = rotation_update_mat * rotation_mat;
			translation_vec = rotation_update_mat * translation_vec + translation_update_vec;
			min = abs(cost);
		}
	}
	std::cout << "rotate_mat: " << rotation_mat << std::endl;
	std::cout << "translation_vec: " << translation_vec << std::endl;
	//print_rotation(rotation_mat);
	//print_translation(translation_vec);
	delete tree;
}

void ICP::get_center_point(const point_cloud &input, Pnt &center_point) {
	center_point.zeros();
	for (unsigned int i = 0; i < input.get_nr_points(); i++)
		center_point += input.pnt(i);
	center_point /= (float)input.get_nr_points();
}

float ICP::error(Pnt &ps, Pnt &pd, Mat &r, Dir &t)
{
	Pnt res;
	res = r * pd;
	float err = pow(ps.x() - res.x() - t[0], 2.0) + pow(ps.y() - res.y() - t[1], 2.0) + pow(ps.z() - res.z() - t[2], 2.0);
	return err;
}

void ICP::get_crspd(Mat& rotation_mat, Dir& translation_vec, point_cloud& pc1, point_cloud& pc2)
{
	Pnt source_center;
	Pnt target_center;
	source_center.zeros();
	target_center.zeros();
	/// create the ann tree
	ann_tree* tree = new ann_tree();
	tree->build(*targetCloud);
	size_t num_source_points = sourceCloud->get_nr_points();

	get_center_point(*targetCloud, target_center);
	get_center_point(*sourceCloud, source_center);
	Pnt p;

	float cost = 1.0;
	///define min as Infinity
	float min = DBL_MAX;
	std::srand(std::time(0));

	Mat fA(0.0f);             // this initializes fA to matrix filled with zeros

	cgv::math::mat<float> U, V;
	cgv::math::diag_mat<float> Sigma;
	U.zeros();
	V.zeros();
	Sigma.zeros();
	for (int iter = 0; iter < maxIterations && abs(cost) > eps; iter++)
	{
		cost = 0.0;
		point_cloud Q, S;
		Q.resize(sourceCloud->get_nr_points());
		S.resize(sourceCloud->get_nr_points());
		source_center = rotation_mat * source_center + translation_vec;
		fA.zeros();
		for (int i = 0; i < sourceCloud->get_nr_points(); i++)
		{
			int randSample = std::rand() % sourceCloud->get_nr_points();
			/// sample the source point cloud
			S.pnt(i) = sourceCloud->pnt(randSample);
			Q.pnt(i) = targetCloud->pnt(tree->find_closest(S.pnt(i)));
			/// get the closest point in the target point cloud
			//Q.pnt(i) = targetCloud->pnt(i);
			fA += Mat(Q.pnt(i) - target_center, rotation_mat * S.pnt(i) + translation_vec - source_center);
		}
		///cast fA to A
		cgv::math::mat<float> A(3, 3, &fA(0, 0));
		cgv::math::svd(A, U, Sigma, V);
		Mat fU(3, 3, &U(0, 0)), fV(3, 3, &V(0, 0));
		///get new R and t
		Mat rotation_update_mat = fU * cgv::math::transpose(fV);
		Dir translation_update_vec = target_center - rotation_update_mat * source_center;
		///calculate error function E(R,t)
		for (int i = 0; i < S.get_nr_points(); i++) {
			///transform Pi to R*Pi + t
			S.pnt(i) = rotation_mat * S.pnt(i) + translation_vec;
			///the new rotation matrix: rotation_update_mat
			//float tempcost = error(Q.pnt(i), S.pnt(i), rotation_update_mat, translation_update_vec);
			cost += error(Q.pnt(i), S.pnt(i), rotation_update_mat, translation_update_vec);
		}
		cost /= sourceCloud->get_nr_points();
		///judge if cost is decreasing, and is larger than eps. If so, update the R and t, otherwise stop and output R and t
		if (min >= abs(cost)) {
			///update the R and t
			rotation_mat = rotation_update_mat * rotation_mat;
			translation_vec = rotation_update_mat * translation_vec + translation_update_vec;
			min = abs(cost);
		}
		pc1.clear();
		pc2.clear();
		for (int i = 0; i < S.get_nr_points(); i++)
			pc1.add_point(S.pnt(i));
		for (int i = 0; i < Q.get_nr_points(); i++)
			pc2.add_point(Q.pnt(i));
	}
	std::cout << "rotate_mat: " << rotation_mat << std::endl;
	std::cout << "translation_vec: " << translation_vec << std::endl;
	delete tree;
}
///print rotation matrix
void ICP::print_rotation(float *rotation) {
	std::cout << "rotation" << std::endl;
	for (int i = 0; i < 9; i = i+3) {
		std::cout << rotation[i] << " " << rotation[i + 1] << " " << rotation[i + 2] << std::endl;
	}
}
///print translation vector
void ICP::print_translation(float *translation) {
	std::cout << "translation" << std::endl;
	std::cout << translation[0] << " " << translation[1] << " " << translation[2] << std::endl;
}