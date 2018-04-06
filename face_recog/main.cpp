#include "age_gender_classify.h"
#include "face_recognition.h"

int main(int argc, char* argv[]) {

	std::vector<string> image_paths = { "image/unknown.jpg","image/jobs1.jpg","image/jobs2.jpg" };

	std::vector<Eigen::ArrayXf> id_dataset;
	std::vector<string> id_name;

	//GetEmbeddings("image/unknown.jpg", "model/20170512-110547.pb");

	for (auto image_path : image_paths)
	{
		cv::Mat image_mat = cv::imread(image_path, CV_LOAD_IMAGE_COLOR);
		Eigen::ArrayXf embs_1 = GetEmbeddingsUsingCvMat(image_mat, "model/20170512-110547.pb");
		int is_known = IsKnown(embs_1, id_dataset);
		if (is_known != -1)
		{
			LOG(INFO) << "Customer: " << id_name[is_known];
		}
		else
		{
			string name = "id_" + std::to_string(id_name.size());
			id_dataset.push_back(embs_1);
			id_name.push_back(name);
			LOG(INFO) << "Unknown customer. Add to dataset, name: " << name;
			ClassifyAgeUsingCvMat(image_mat);
			ClassifyGenderUsingCvMat(image_mat);
		}
	}

	return 0;
}