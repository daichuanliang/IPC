#include "tensor_util.h"

void ClassifyAge(string image);

void ClassifyGender(string image);

int ClassifyAgeUsingCvMat(cv::Mat image_mat, std::unique_ptr<tensorflow::Session> *session);

int ClassifyGenderUsingCvMat(cv::Mat image_mat, std::unique_ptr<tensorflow::Session> *session);

