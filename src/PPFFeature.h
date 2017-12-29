#pragma once
#include <vector>
#include <stdint.h>
#include "common.h"
#include "pose_cluster.h"
#include "Voxel_grid.h"

#include "boost/serialization/serialization.hpp"  
#include "boost/archive/binary_oarchive.hpp"  
#include "boost/archive/binary_iarchive.hpp"  
#include <boost/serialization/export.hpp> 
namespace zyk
{
	struct ZYK_EXPORTS PPF
	{
		int first_index;
		int second_index;
		pcl::PPFSignature ppf;
		float weight = 1;
	};

	struct PPF_Accumulator
	{
		PPF_Accumulator(int pnt_number, int rot_angle_div){ acumulator = Eigen::MatrixXf(pnt_number, rot_angle_div); acumulator.setConstant(0); };
		Eigen::MatrixXf acumulator;
	};

	class ZYK_EXPORTS PPF_Space
	{
	public:
		PPF_Space();
		~PPF_Space();
	public:
		//construct
		bool init(std::string Name, pcl::PointCloud<PointType>::Ptr pointcloud, pcl::PointCloud<NormalType>::Ptr pointNormal, int32_t angle_div, int32_t distance_div, bool ignore_plane=false);
		//bool init(pcl::PointCloud<PointType>::Ptr pointcloud, pcl::PointCloud<NormalType>::Ptr pointNormal, float angle_step, float distance_step,bool ignore_plane=false);
		void clear();
		////////////////////
		////// property access
		////////////////////
		const string getName() const { return mName; };
		const double getMaxD() const { return max_p[3]; };
		const double getSampleRatio() const { return model_res / (norm(model_size, 3)); };
	public:
		////////////////////
		//////methods
		////////////////////

		static float computeAlpha(const PointType& first_pnt, const NormalType& first_normal, const PointType& second_pnt);
		static float computeAlpha(const Eigen::Vector3f& first_pnt, const Eigen::Vector3f& first_normal, const Eigen::Vector3f& second_pnt);
		static void computeSinglePPF(const PointType& first_pnt, const NormalType& first_normal, const PointType& second_pnt, const NormalType& second_normal, zyk::PPF& ppf);
		static void computeSinglePPF(const Eigen::Vector3f& first_pnt, const Eigen::Vector3f& first_normal, const Eigen::Vector3f& second_pnt, const Eigen::Vector3f& second_normal,zyk::PPF& ppf);
		// for train only, in match step, use the above two.
		static void computeSinglePPF(pcl::PointCloud<PointType>::Ptr pointcloud, pcl::PointCloud<NormalType>::Ptr pointNormalType, int32_t index1, int32_t index2, PPF& ppf);
		static bool getPoseFromPPFCorresspondence(PointType& model_point, NormalType& model_normal, PointType& scene_point, NormalType&scene_normal, float alpha, Eigen::Affine3f& transformation);
		//test speed
		void getppfBoxCoord(PPF& ppf, Eigen::Vector4i& ijk);
		void getppfBoxCoord(PPF& ppf, int* ijk);
		int getppfBoxIndex(PPF& ppf);
		vector<box*>* getBoxVector() { return &ppf_box_vector; };
		vector<PPF>* getPPFVector() { return &ppf_vector; };

		//in order to spread discretized ppf, use this to get neighboring ppf box
		void getNeighboringPPFBoxIndex(int currentIndex,vector<int>&out_vec);
		void getModelPointCloud(pcl::PointCloud<PointType>::Ptr&pointcloud){ pointcloud = input_point_cloud; };
		void getCenteredPointCloud(pcl::PointCloud<PointType>::Ptr&pointcloud){ pointcloud = centered_point_cloud; };
		void getPointNormalCloud(pcl::PointCloud<NormalType>::Ptr&pointNormals){ pointNormals = input_point_normal; };

		friend class boost::serialization::access;
		
		//for test only
		float computeClusterScore(pcl::PointCloud<PointType>::Ptr& scene, pcl::PointCloud<NormalType>::Ptr& scene_normals, float dis_thresh, float ang_thresh, zyk::pose_cluster &pose_clusters);
		
		////////match
		void match(pcl::PointCloud<PointType>::Ptr scene, pcl::PointCloud<NormalType>::Ptr scene_normals, bool spread_ppf_switch, bool two_ball_switch, float relativeReferencePointsNumber, float max_vote_thresh, float max_vote_percentage, float angle_thresh, float first_dis_thresh, float recompute_score_dis_thresh, float recompute_score_ang_thresh, float second_dis_thresh, int num_clusters_per_group, vector<zyk::pose_cluster, Eigen::aligned_allocator<zyk::pose_cluster>>& pose_clusters);
		///////USER IO
		bool save(std::string file_name);
		bool load(std::string fine_name);

		//bool model_x_centrosymmetric = false;
		//bool model_y_centrosymmetric = false;
		//bool model_z_centrosymmetric = false;

		//
		//model property
		//
		float model_size[3];
		float model_res;
		bool ignore_plane_switch;

	protected:
		///////////////IO
		template<class Archive>
		void save(Archive& ar, const unsigned int version);
		template<class Archive>
		void load(Archive& ar, const unsigned int version);
		BOOST_SERIALIZATION_SPLIT_MEMBER()

		/////////////METHODS
		void recomputeClusterScore(zyk::CVoxel_grid& grid, pcl::PointCloud<NormalType>& scene_normals, float dis_thresh, float ang_thresh, vector<zyk::pose_cluster, Eigen::aligned_allocator<zyk::pose_cluster>> &pose_clusters);
		bool computeAllPPF();
		bool findBoundingBox();
		bool constructGrid();
	protected:
		////////////////////
		//////basic property
		////////////////////
		std::string mName;
		//for fast scene point access
		//CVoxel_grid scene_grid;

		//ptr to data
		pcl::PointCloud<PointType>::Ptr input_point_cloud;
		pcl::PointCloud<PointType>::Ptr centered_point_cloud;
		pcl::PointCloud<NormalType>::Ptr input_point_normal;
		//grid property
		int grid_f1_div;
		int grid_f2_div;
		int grid_f3_div;
		int grid_f4_div;
		//grids two limits
		Eigen::Array4f min_p;
		Eigen::Array4f max_p;
		//PPF container
		vector<PPF> ppf_vector;
		//ppf_box container
		vector<box*> ppf_box_vector;

		///////////////////////////////////////////
		////////////// other property
		///////////////////////////////////////////
		int32_t total_box_num;
		//size
		Eigen::Array4f leaf_size;
		Eigen::Array4f inv_leaf_size;
		//multiplier
		Eigen::Vector4i grid_div_mul;
		//Eigen::Vector3f point_cloud_center;
		//model cloud center
		double cx, cy, cz;
	};



}

template<class Archive>
void zyk::PPF_Space::save(Archive& ar, const unsigned int version)
{
	ar & mName;
	ar & grid_f1_div;
	ar & grid_f2_div;
	ar & grid_f3_div;
	ar & grid_f4_div;
	ar & min_p(0);
	ar & min_p(1);
	ar & min_p(2);
	ar & min_p(3);
	ar & max_p(0);
	ar & max_p(1);
	ar & max_p(2);
	ar & max_p(3);
	ar & model_size[0];
	ar & model_size[1];
	ar & model_size[2];
	ar & model_res;
	int ppf_vector_size=ppf_vector.size();
	ar & ppf_vector_size;
	for (int32_t i = 0; i < ppf_vector.size(); i++)
	{
		ar & ppf_vector[i].first_index;
		ar & ppf_vector[i].second_index;
		ar & ppf_vector[i].weight;
		ar & ppf_vector[i].ppf.alpha_m;
		ar & ppf_vector[i].ppf.f1;
		ar & ppf_vector[i].ppf.f2;
		ar & ppf_vector[i].ppf.f3;
		ar & ppf_vector[i].ppf.f4;
	}
	int point_num= input_point_cloud->size();
	ar & point_num;
	for (int32_t i = 0; i < input_point_cloud->size(); i++)
	{
		ar & input_point_cloud->at(i).x;
		ar & input_point_cloud->at(i).y;
		ar & input_point_cloud->at(i).z;
		ar & input_point_normal->at(i).normal_x;
		ar & input_point_normal->at(i).normal_y;
		ar & input_point_normal->at(i).normal_z;
	}
}

template<class Archive>
void zyk::PPF_Space::load(Archive& ar, const unsigned int version)
{
	int32_t ppf_vector_size = 0;
	ar & mName;
	ar & grid_f1_div;
	ar & grid_f2_div;
	ar & grid_f3_div;
	ar & grid_f4_div;
	ar & min_p(0);
	ar & min_p(1);
	ar & min_p(2);
	ar & min_p(3);
	ar & max_p(0);
	ar & max_p(1);
	ar & max_p(2);
	ar & max_p(3);
	ar & model_size[0];
	ar & model_size[1];
	ar & model_size[2];
	ar & model_res;
	ar & ppf_vector_size;
	ppf_vector.resize(ppf_vector_size);
	for (int32_t i = 0; i < ppf_vector_size; i++)
	{
		ar & ppf_vector[i].first_index;
		ar & ppf_vector[i].second_index;
		ar & ppf_vector[i].weight;
		ar & ppf_vector[i].ppf.alpha_m;
		ar & ppf_vector[i].ppf.f1;
		ar & ppf_vector[i].ppf.f2;
		ar & ppf_vector[i].ppf.f3;
		ar & ppf_vector[i].ppf.f4;
	}
	int32_t points_num = 0;
	ar & points_num;
	input_point_cloud = pcl::PointCloud<PointType>::Ptr(new pcl::PointCloud<PointType>());
	input_point_normal = pcl::PointCloud<NormalType>::Ptr(new pcl::PointCloud<NormalType>());
	for (int32_t i = 0; i < points_num; i++)
	{
		PointType _tem;
		NormalType _nor;
		ar & _tem.x;
		ar & _tem.y;
		ar & _tem.z;
		ar & _nor.normal_x;
		ar & _nor.normal_y;
		ar & _nor.normal_z;
		input_point_cloud->push_back(_tem);
		input_point_normal->push_back(_nor);
	}

	float minx, miny, minz, maxx, maxy, maxz;
	for (size_t i = 0; i < input_point_cloud->size(); ++i) {
		if (i == 0) {
			maxx = minx = input_point_cloud->at(i).x;
			maxy = miny = input_point_cloud->at(i).y;
			maxz = minz = input_point_cloud->at(i).z;
		}
		else {
			maxx = std::max(maxx, input_point_cloud->at(i).x); minx = std::min(minx, input_point_cloud->at(i).x);
			maxy = std::max(maxy, input_point_cloud->at(i).y); miny = std::min(miny, input_point_cloud->at(i).y);
			maxz = std::max(maxz, input_point_cloud->at(i).z); minz = std::min(minz, input_point_cloud->at(i).z);
		}
	}
	cx = (minx + maxx) / 2;
	cy = (miny + maxy) / 2;
	cz = (minz + maxz) / 2;
	centered_point_cloud->clear();
	for (size_t i = 0; i < input_point_cloud->size(); ++i) {
		PointType _tem;
		_tem.x = input_point_cloud->at(i).x - cx;
		_tem.y = input_point_cloud->at(i).y - cy;
		_tem.z = input_point_cloud->at(i).z - cz;
		centered_point_cloud->push_back(_tem);
	}

	grid_div_mul(0) = 1;
	grid_div_mul(1) = grid_div_mul(0)*grid_f1_div;
	grid_div_mul(2) = grid_div_mul(1)*grid_f2_div;
	grid_div_mul(3) = grid_div_mul(2)*grid_f3_div;
	total_box_num = grid_div_mul(3)*grid_f4_div;
	assert(total_box_num > 0);

	leaf_size(0) = M_PI/ grid_f1_div;
	leaf_size(1) = M_PI / grid_f2_div;
	leaf_size(2) = M_PI / grid_f3_div;
	leaf_size(3) = 1.1*max_p(3) / grid_f4_div;

	inv_leaf_size(0) = 1 / leaf_size(0);
	inv_leaf_size(1) = 1 / leaf_size(1);
	inv_leaf_size(2) = 1 / leaf_size(2);
	inv_leaf_size(3) = 1 / leaf_size(3);

	//grid
	ppf_box_vector.resize(total_box_num);
	constructGrid();
}
