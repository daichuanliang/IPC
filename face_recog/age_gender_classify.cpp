#include "age_gender_classify.h"

static const std::vector<string> AGE_LIST = { "(0, 2)","(4, 6)","(8, 12)" ,"(15, 20)" ,"(25, 32)" ,"(38, 43)" ,"(48, 53)" ,"(60, 100)" };
static const std::vector<string> GENDER_LIST = { "M","F" };

void Classify(string image, string graph, std::vector<string> labels) {
	// These are the command-line flags the program can understand.
	// They define where the graph and input data is located, and what kind of
	// input the model expects. If you train your own model, or use something
	// other than inception_v3, then you'll need to update these.
	int32 input_width = 227;
	int32 input_height = 227;
	float input_mean = 0;
	float input_std = 255;
	string input_layer = "inputdata:0";
	string output_layer = "outputdata:0";
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

	// Actually run the image through the model.
	std::vector<Tensor> outputs;
	Status run_status = session->Run({ { input_layer, resized_tensor } },
	{ output_layer }, {}, &outputs);
	if (!run_status.ok()) {
		LOG(ERROR) << "Running model failed: " << run_status;
	}
	//Tensor result = outputs[0];
	//auto probs = result.flat<float>();
	//for (size_t i = 0; i < probs.size(); i++)
	//{
	//	LOG(INFO) << probs(i);
	//}

	// Do something interesting with the results we've generated.
	Status print_status = PrintFirstLabel(outputs, labels);
	if (!print_status.ok()) {
		LOG(ERROR) << "Running print failed: " << print_status;
	}

}

void ClassifyAge(string image) {
	LOG(INFO) << "Guess age...";
	Classify(image, "model/age_inception_model.pb", AGE_LIST);
}

void ClassifyGender(string image) {
	LOG(INFO) << "Guess gender...";
	Classify(image, "model/gender_inception_model.pb", GENDER_LIST);
}



int ClassifyUsingCvMat(cv::Mat image_mat, string graph, std::unique_ptr<tensorflow::Session> *session) {
	// These are the command-line flags the program can understand.
	// They define where the graph and input data is located, and what kind of
	// input the model expects. If you train your own model, or use something
	// other than inception_v3, then you'll need to update these.
	int32 input_width = 227;
	int32 input_height = 227;
	float input_mean = 0;
	float input_std = 255;
	string input_layer = "inputdata:0";
	string output_layer = "outputdata:0";
	string root_dir = "";


	// First we load and initialize the model.
	// std::unique_ptr<tensorflow::Session> session;
	string graph_path = tensorflow::io::JoinPath(root_dir, graph);
	Status load_graph_status = LoadGraph(graph_path, session);
	if (!load_graph_status.ok()) {
		LOG(ERROR) << load_graph_status;
	}

	image_mat.convertTo(image_mat, CV_32FC3);
	if (image_mat.empty()) {
		LOG(ERROR) << "Failed to read image by opencv";
	}
	cv::resize(image_mat, image_mat, cv::Size(input_height, input_width));
	const Tensor image_tensor = ReadTensorFromCvMat(image_mat, input_height, input_width);

	// Actually run the image through the model.
	std::vector<Tensor> outputs;
	Status run_status = (*session)->Run({ { input_layer, image_tensor } },
	{ output_layer }, {}, &outputs);
	if (!run_status.ok()) {
		LOG(ERROR) << "Running model failed: " << run_status;
	}
	Tensor result = outputs[0];
	auto probs = result.flat<float>();
	float max_prob = 0.0;
	int index = 0;
	for (size_t i = 0; i < probs.size(); i++)
	{
		if (max_prob < probs(i))
		{
			max_prob = probs(i);
			index = i;
		}
	}
	return index;
}

int ClassifyAgeUsingCvMat(cv::Mat image_mat, std::unique_ptr<tensorflow::Session> *session) {
	int index = ClassifyUsingCvMat(image_mat, "face_recog/model/age_inception_model.pb", session);
	LOG(INFO) << "Guess age: " << AGE_LIST[index];
    if (index == 0) return 1;
    else return 0;
}

int ClassifyGenderUsingCvMat(cv::Mat image_mat, std::unique_ptr<tensorflow::Session> *session) {
	int index = ClassifyUsingCvMat(image_mat, "face_recog/model/gender_inception_model.pb", session);
	LOG(INFO) << "Guess gender: " << GENDER_LIST[index];
    if (index == 0 || index == 1 || index == 2) return 0;
    else if (index == 3 || index == 4) return 1;
    else if (index == 5 || index == 6) return 2;
    else return 3;
}
