#include "age_gender_classify.h"

using namespace tensorflow;

int main(int argc, char* argv[]) {

	cv::Mat image_mat = cv::imread("image/101.jpg", CV_LOAD_IMAGE_COLOR);
	ClassifyAgeUsingCvMat(image_mat);
	ClassifyGenderUsingCvMat(image_mat);

	return 0;
}