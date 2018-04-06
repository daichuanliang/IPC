#ifndef FACE_RECOGNITION_H
#define FACE_RECOGNITION_H
#include "tensor_util.h"
#include <math.h>
#include <stdio.h>
#include <iostream>
#include <Eigen/Dense>
#include <Eigen/Core>

extern std::vector<Eigen::ArrayXf> global_id_dataset;
Eigen::ArrayXf GetEmbeddingsUsingCvMat(cv::Mat image_mat, string graph);

int IsKnown(Eigen::ArrayXf embeddings, std::vector<Eigen::ArrayXf> id_dataset);
typedef Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> RowMatrixXf;

#endif
