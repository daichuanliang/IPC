#include "face_recognition.h"

#include "meta.h"

Eigen::ArrayXf GetEmbeddings(string image, string graph) {
	// These are the command-line flags the program can understand.
	// They define where the graph and input data is located, and what kind of
	// input the model expects. If you train your own model, or use something
	// other than inception_v3, then you'll need to update these.
	int32 input_width = 160;
	int32 input_height = 160;
	float input_mean = 0;
	float input_std = 255;
	string input_layer = "input:0";
	string embeddings_layer = "embeddings:0";
	string phase_train_layer = "phase_train:0";
	string root_dir = "";

	// First we load and initialize the model.
	std::unique_ptr<tensorflow::Session> session;
	string graph_path = tensorflow::io::JoinPath(root_dir, graph);
	Status load_graph_status = LoadGraph(graph_path, &session);
	if (!load_graph_status.ok()) {
		LOG(ERROR) << load_graph_status;
	}

	// Get the image from disk as a float array of numbers, resized and normalized
	// to the specifications the main graph expects.
	std::vector<Tensor> resized_tensors;
	string image_path = tensorflow::io::JoinPath(root_dir, image);
	Status read_tensor_status =
		ReadTensorFromImageFile(image_path, input_height, input_width, input_mean,
			input_std, &resized_tensors);
	if (!read_tensor_status.ok()) {
		LOG(ERROR) << read_tensor_status;
	}
	const Tensor& resized_tensor = resized_tensors[0];

	Tensor phase_train(tensorflow::DT_BOOL, tensorflow::TensorShape());
	phase_train.scalar<bool>()() = false;

	// Actually run the image through the model.
	std::vector<Tensor> outputs;
	Status run_status = session->Run({ { input_layer, resized_tensor },{ phase_train_layer, phase_train } },
	{ embeddings_layer }, {}, &outputs);
	if (!run_status.ok()) {
		LOG(ERROR) << "Running model failed: " << run_status;
	}
	Tensor result = outputs[0];
	auto embs = result.flat<float>();
	Eigen::ArrayXf embeddings((Eigen::ArrayXf::Map(embs.data(), embs.size())));
	//std::cout << "Using image path" << embeddings << std::endl << std::endl;
	return embeddings;

}

void PreWhiten(cv::Mat src) {
	int height = src.rows;
	int width = src.cols;

	cv::Mat means, stddevs;
	meanStdDev(src, means, stddevs);
	double mean = (means.at<double>(0) + means.at<double>(1) + means.at<double>(2)) / 3;
	double stddev = sqrt((pow(means.at<double>(0), 2) + pow(means.at<double>(1), 2) + pow(means.at<double>(2), 2)
		+ pow(stddevs.at<double>(0), 2) + pow(stddevs.at<double>(1), 2) + pow(stddevs.at<double>(2), 2)) / 3 - pow(mean, 2));
	double std_adj = (sqrt(1.0 / (height * width * 3)) < stddev) ? stddev : sqrt(1.0 / (height * width));

	float* src_data = (float*)src.data;
	for (int y = 0; y < height; ++y) {
		float* src_row = src_data + (y * width * 3);
		for (int x = 0; x < width; ++x) {
			float* src_pixel = src_row + (x * 3);
			src_pixel[0] = (src_pixel[0] - mean) / std_adj;
			src_pixel[1] = (src_pixel[1] - mean) / std_adj;
			src_pixel[2] = (src_pixel[2] - mean) / std_adj;
		}
	}
}

Eigen::ArrayXf GetEmbeddingsUsingCvMat(cv::Mat image_mat, string graph) {
	// These are the command-line flags the program can understand.
	// They define where the graph and input data is located, and what kind of
	// input the model expects. If you train your own model, or use something
	// other than inception_v3, then you'll need to update these.
	int32 input_width = 160;
	int32 input_height = 160;
	float input_mean = 0;
	float input_std = 255;
	string input_layer = "input:0";
	string embeddings_layer = "embeddings:0";
	string phase_train_layer = "phase_train:0";
	string root_dir = "";

	// First we load and initialize the model.
	std::unique_ptr<tensorflow::Session> session;
	string graph_path = tensorflow::io::JoinPath(root_dir, graph);
	Status load_graph_status = LoadGraph(graph_path, &session);
	if (!load_graph_status.ok()) {
		LOG(ERROR) << load_graph_status;
	}

	image_mat.convertTo(image_mat, CV_32FC3);
	if (image_mat.empty()) {
		LOG(ERROR) << "Failed to read image by opencv";
	}
	cv::resize(image_mat, image_mat, cv::Size(input_height, input_width));
	PreWhiten(image_mat);
	const Tensor image_tensor = ReadTensorFromCvMat(image_mat, input_height, input_width);

	Tensor phase_train(tensorflow::DT_BOOL, tensorflow::TensorShape());
	phase_train.scalar<bool>()() = false;

	// Actually run the image through the model.
	std::vector<Tensor> outputs;
	Status run_status = session->Run({ { input_layer, image_tensor },{ phase_train_layer, phase_train } },
	{ embeddings_layer }, {}, &outputs);
	if (!run_status.ok()) {
		LOG(ERROR) << "Running model failed: " << run_status;
	}
	Tensor result = outputs[0];
	auto embs = result.flat<float>();
	Eigen::ArrayXf embeddings((Eigen::ArrayXf::Map(embs.data(), embs.size())));
	//std::cout << "Using cv::Mat" << embeddings << std::endl << std::endl;
	return embeddings;

}

float arrayXfDistance(Eigen::ArrayXf x, Eigen::ArrayXf y) {
	return sqrt((x - y).square().sum());
}

int IsKnown(Eigen::ArrayXf embeddings, std::vector<Eigen::ArrayXf> id_dataset) {
	float threshold = 0.60;
	for (int i = 0; i < id_dataset.size(); i++) {
		float distance = arrayXfDistance(embeddings, id_dataset[i]);
        LOG(INFO) << "================= current threshold is " << threshold << " =========== distance is" << distance ;

        if (distance < threshold) {
			return i;
		}
	}
	return -1;
}

void updateDataset(Meta_t meta) {
    // std::vector<Eigen::ArrayXf> feature_dataset;
    // std::vector<int> customer_id_dataset;
    float feature_data[128];
    // memcpy(feature_data,meta.feature,sizeof(meta.feature));
    for(int i=0;i<128;i++)
    {
        feature_data[i] = (float) meta.feature[i];
    }

    Eigen::Map<RowMatrixXf> v(feature_data,128,1);
    Eigen::ArrayXf arrayData(128);
    arrayData << v.col(0).array();
    global_id_dataset.push_back(arrayData);
    // Eigen::ArrayXf emb = convertArrayToEigenArrayXf(meta.feature);
    // Eigen::ArrayXf emb(meta.feature);
    // feature_dataset.push_back(emb);
    // customer_id_dataset.push_back(meta.customerID);
}
